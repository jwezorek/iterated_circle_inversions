#include "iterated_inversion.h"
#include "geometry.h"
#include "circle_set.h"
#include "circle_tree.h"
#include "input.h"
#include "util.h"
#include "image.h"
#include <print>
#include <sstream>
#include <ranges>
#include <complex>
#include <filesystem>
#include <bit>

namespace fs = std::filesystem;
namespace r = std::ranges;
namespace rv = std::ranges::views;

/*------------------------------------------------------------------------------------------------*/
namespace {

    using rect = ici::rect_type<int>;

    std::optional<rect> intersection(const rect& r1, const rect& r2) {

        ici::point_type<int> min_point{
            std::max(r1.min.x, r2.min.x),
            std::max(r1.min.y, r2.min.y)
        };

        ici::point_type<int> max_point{
            std::min(r1.max.x, r2.max.x),
            std::min(r1.max.y, r2.max.y)
        };

        if (min_point.x <= max_point.x && min_point.y <= max_point.y) {
            return rect{ min_point, max_point };
        } else {
            return std::nullopt; 
        }
    }

    int get_scale(int antialias_mode) {
        if (antialias_mode == 0) {
            return 1;
        }

        if (antialias_mode > 3) {
            antialias_mode = 3;
        }
        int scale = 1;
        for (int i = 0; i < antialias_mode; ++i) {
            scale *= 2;
        }
        return scale;
    }

    std::tuple<int, int, double> image_metrics(
        ici::point min_pt, ici::point max_pt, int resolution) {

        double wd = max_pt.x - min_pt.x;
        double hgt = max_pt.y - min_pt.y;
        double cols = 0.0;
        double rows = 0.0;
        double image_to_log = 0.0;

        if (wd > hgt) {
            cols = resolution;
            rows = std::ceil((hgt * static_cast<double>(cols)) / wd);
            image_to_log = wd / static_cast<double>(cols);
        } else {
            rows = resolution;
            cols = std::ceil((wd * static_cast<double>(rows)) / hgt);
            image_to_log = hgt / static_cast<double>(rows);
        }

        return {
            static_cast<int>(cols),
            static_cast<int>(rows),
            image_to_log 
        };
    }

    struct raster_context {
        ici::circle_tree circles;
        ici::rectangle view;
        double img_to_log;
        int canvas_sz;
        int antialiasing_level;
        std::vector<ici::color> colors;
    };

    ici::rectangle canvas_rect_to_logical_rect(const raster_context& ctxt, const rect& r) {
        auto origin = ctxt.view.min;
        auto x1 = origin.x + r.min.x * ctxt.img_to_log;
        auto y1 = origin.y + r.min.y * ctxt.img_to_log;
        auto x2 = origin.x + (r.max.x + 1) * ctxt.img_to_log;
        auto y2 = origin.y + (r.max.y + 1) * ctxt.img_to_log;
        return { {x1,y1},{x2,y2} };
    }

    int two_to_the_nth(int n) {
        int v = 1;
        for (int i = 0; i < n; ++i) {
            v += 2;
        }
        return v;
    }

    uint32_t to_pixel(const ici::color& color) {
        return (0xFF << 24) | (color.r << 16) | (color.g << 8) | color.b;
    }

    void rasterize_pixel(const raster_context& ctxt, ici::image& img, int col, int row) {
        if (col < 0 || row < 0 || col >= img.cols() || row >= img.rows()) {
            return;
        }
        auto rect = canvas_rect_to_logical_rect(ctxt, { {col,row},{col,row} });
        auto dimension = two_to_the_nth(ctxt.antialiasing_level);
        auto spacing = (rect.max.x - rect.min.x) / dimension;
        auto marg = spacing / 2.0;

        double red = 0;
        double green = 0;
        double blue = 0;

        for (int j = 0; j < dimension; ++j) {
            for (int i = 0; i < dimension; ++i) {
                auto pt = rect.min + ici::point{ i * spacing + marg, j * spacing + marg };
                auto circles = ctxt.circles.contains(pt);
                auto color = ctxt.colors.at(circles.size() % ctxt.colors.size());
                red += color.r;
                green += color.g;
                blue += color.b;
            }
        }
        auto area = dimension * dimension;
        ici::color pixel{
                static_cast<uint8_t>(std::round(red / area)),
                static_cast<uint8_t>(std::round(green / area)),
                static_cast<uint8_t>(std::round(blue / area))
        };
        img(col, row) = to_pixel(pixel);
    }

    std::optional<int> containing_circle_count(const ici::circle_tree& tree, const ici::rectangle& r) {
        auto intersecting_circles = tree.intersects(r);
        for (const auto& circle : intersecting_circles) {
            if (!ici::circle_contains_rectangle(circle, r)) {
                return {};
            }
        }
        return static_cast<int>(intersecting_circles.size());
    }

    void fill_rect(ici::image& img, const rect& r, uint32_t color) {
        auto img_rect = rect{ {0,0},{img.cols() - 1,img.rows() - 1} };
        auto clip_rect = intersection(img_rect, r);
        if (!clip_rect) {
            return;
        }
        for (int y = clip_rect->min.y; y <= clip_rect->max.y; ++y) {
            for (int x = clip_rect->min.x; x <= clip_rect->max.x; ++x) {
                img(x, y) = color;
            }
        }
    }

    struct progress {
        int total;
        int last_reported;
        int curr;
    };

    void update_progress(progress& prog, int n) {
        int old_pcnt = static_cast<int>(100 * static_cast<double>(prog.curr) / prog.total);
        prog.curr += n;
        int pcnt = static_cast<int>((100.0 * static_cast<double>(prog.curr)) / prog.total);
        if (pcnt - prog.last_reported > 5) {
            std::println("  {}% complete...", pcnt);
            prog.last_reported = pcnt;
        }
    }

    void finalize_progress(const progress& prog) {
        if (prog.last_reported != 100) {
            std::println("  100% complete...\n");
        }
    }

    void rasterize_rect(
            const raster_context& ctxt, ici::image& img, const rect& rect, progress& prog) {

        auto log_rect = canvas_rect_to_logical_rect(ctxt, rect);
        if (!ici::intersects(log_rect, ctxt.view)) {
            return;
        }

        if (rect.min.x == rect.max.x && rect.min.y == rect.max.y) {
            rasterize_pixel(ctxt, img, rect.min.x, rect.min.y);
            update_progress(prog, 1);
            return;
        }

        // if the only circles the rectangle intersects completely contain the rectangle
        // then fill in this rectangle.
        auto count = containing_circle_count(ctxt.circles, log_rect);
        if (count) {
            fill_rect(img, rect, to_pixel(ctxt.colors.at(*count % ctxt.colors.size())));
            update_progress(
                prog, 
                (rect.max.x - rect.min.x + 1) * (rect.max.y - rect.min.y + 1)
            );
            return;
        }

        // otherwise, recurse...
        int sz = (rect.max.x - rect.min.x + 1) / 2;
        int x1 = rect.min.x;
        int y1 = rect.min.y;
        int x2 = rect.max.x;
        int y2 = rect.max.y;

        std::array<::rect, 4> quadrants = { {
            {{x1 , y1 + sz} , {x1 + sz - 1,y2}}, // northwest
            {{x1 + sz, y1 + sz} , {x2 , y2}}, // northeast
            {{x1 + sz, y1}, {x2, y1 + sz - 1}}, // southeast
            {{x1,y1},{x1 + sz - 1, y1 + sz - 1}} // southwest
        } };

        for (const auto& quadrant : quadrants) {
            rasterize_rect(ctxt, img, quadrant, prog);
        }
    }

    void invert_and_insert(ici::circle_set& set, const ici::circle& lhs, const ici::circle& rhs) {
        auto inversion = ici::invert(lhs, rhs);
        if (inversion) {
            set.insert(*inversion);
        }
    }

    ici::circle_set all_inversions(const ici::circle_set& set) {
        ici::circle_set output(set.eps());
        auto circles = set.to_vector();
        for (const auto& [c1, c2] : ici::two_combinations(circles)) {
            invert_and_insert(output, c1, c2);
            invert_and_insert(output, c2, c1);
        }
        return output;
    }

    ici::circle_set inverse_of_cartesian_product(
            const ici::circle_set& lhs, const ici::circle_set& rhs) {

        ici::circle_set output(lhs.eps());

        if (lhs.empty() || rhs.empty()) {
            return output;
        }

        auto lhs_circles = lhs.to_vector();
        auto rhs_circles = rhs.to_vector();
        for (const auto& [c1, c2] : rv::cartesian_product(lhs_circles, rhs_circles)) {
            invert_and_insert(output, c1, c2);
            invert_and_insert(output, c2, c1);
        }

        return output;
    }

    ici::circle_set circle_set_union(const ici::circle_set& lhs, const ici::circle_set& rhs) {
        ici::circle_set output(lhs.eps(), lhs.to_vector());
        auto rhs_circles = rhs.to_vector();
        for (auto&& c : rhs_circles) {
            output.insert(c);
        }
        return output;
    }
}

std::vector<ici::circle> ici::invert_circles(const ici::input& inp)
{
    std::println("inverting {}...", inp.fname);

    circle_set output(inp.eps);
    circle_set prev(inp.eps);
    circle_set curr(inp.eps, inp.circles);

    for (int i : rv::iota(0, inp.iterations)) {
        auto new_inversions = circle_set_union(
            all_inversions(curr), inverse_of_cartesian_product(prev, curr)
        );

        std::println("  iteration {}: adding {} circles...", i + 1, new_inversions.size());

        prev = curr;
        curr = new_inversions;
        output = circle_set_union(output, curr);
    }

    std::println("complete.");

    return (!output.empty()) ? output.to_vector() : inp.circles;
}

void ici::to_svg(const std::string& fname, const std::vector<circle>& inp_circles,
        const vector_settings& settings) {

    auto circles = inp_circles | rv::transform(
        [k = settings.scale](auto&& c) {
            return ici::scale(c, k);
        }
    ) | r::to<std::vector>();
    auto dimensions = pad(bounds(circles), settings.padding);
    std::stringstream ss;

    ss << std::format(
        "<svg viewBox=\"{} {} {} {}\" xmlns=\"http://www.w3.org/2000/svg\">\n",
        dimensions.min.x,
        dimensions.min.y,
        dimensions.max.x - dimensions.min.x,
        dimensions.max.y - dimensions.min.y
    );
    ss << std::format("<style> circle {{ mix-blend-mode: {}; }} </style>\n", settings.blend_mode);

    ss << std::format(
        "<rect x=\"{}\" y=\"{}\" width=\"{}\" height=\"{}\" fill=\"{}\" />\n",
        dimensions.min.x,
        dimensions.min.y,
        dimensions.max.x - dimensions.min.x,
        dimensions.max.y - dimensions.min.y,
        settings.bkgd_color
    );

    for (const auto& c : circles) {
        ss << std::format(
            "    <circle cx=\"{}\" cy=\"{}\" r=\"{}\" fill=\"{}\" stroke=\"none\" />\n",
            c.loc.x, 
            c.loc.y, 
            c.radius, 
            settings.color
        );
    }

    ss << "</svg>";

    string_to_file(fname, ss.str());
}

ici::image ici::to_raster( const std::string& outp, const rectangle& view_rect,
        const std::vector<circle>& inp, const raster_settings& settings) {

    auto [cols, rows, image_to_logical] = image_metrics(
        view_rect.min, view_rect.max, settings.resolution
    );

    raster_context ctxt = {
        .circles = {inp},
        .view = view_rect,
        .img_to_log = image_to_logical,
        .canvas_sz = static_cast<int>(
                std::bit_ceil(static_cast<unsigned>(std::max(cols, rows)))
            ),
        .antialiasing_level = settings.antialiasing_level,
        .colors = settings.color_tbl
    };
    progress prog{ ctxt.canvas_sz * ctxt.canvas_sz, 0, 0 };
    ici::image img(cols, rows);
    rasterize_rect( ctxt, img, {{0,0},{ctxt.canvas_sz - 1, ctxt.canvas_sz - 1}}, prog );
    finalize_progress(prog);
    
    return img;
}
