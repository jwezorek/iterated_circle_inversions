#include "iterated_inversion.h"
#include "geometry.h"
#include "circle_set.h"
#include "circle_tree.h"
#include "input.h"
#include "util.h"
#include "image.h"
#include <opencv2/opencv.hpp>
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

    cv::Vec3b to_cv_color(const ici::color& color) {
        return {
            color.r,
            color.g,
            color.b
        };
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

    void modulo_in_place(cv::Mat& mat, uchar modulus) {
        for (int y = 0; y < mat.rows; y++) {
            for (int x = 0; x < mat.cols; x++) {
                mat.at<uchar>(y, x) %= modulus;
            }
        }
    }

    cv::Mat apply_color_table(const cv::Mat& indexMat, const std::vector<cv::Vec3b>& colors) {
        cv::Mat colorMat(indexMat.size(), CV_8UC3);

        for (int y = 0; y < indexMat.rows; y++) {
            for (int x = 0; x < indexMat.cols; x++) {
                uchar index = indexMat.at<uchar>(y, x);
                colorMat.at<cv::Vec3b>(y, x) = colors[index];
            }
        }

        return colorMat;
    }

    std::tuple<int, int, double> image_metrics(
        ici::point min_pt, ici::point max_pt, int scale, int resolution) {

        double wd = max_pt.x - min_pt.x;
        double hgt = max_pt.y - min_pt.y;
        double cols = 0.0;
        double rows = 0.0;
        double log_to_image = 0.0;

        if (wd > hgt) {
            cols = resolution;
            rows = std::ceil((hgt * static_cast<double>(cols)) / wd);
            log_to_image = static_cast<double>(cols) / wd;
        }
        else {
            rows = resolution;
            cols = std::ceil((wd * static_cast<double>(rows)) / hgt);
            log_to_image = static_cast<double>(rows) / hgt;
        }

        return {
            scale * static_cast<int>(cols),
            scale * static_cast<int>(rows),
            scale * log_to_image };
    }


    class modulo_canvas {
        cv::Mat img_;
        cv::Mat mask_;
        uchar modulus_;

    public:
        modulo_canvas(int cols, int rows, int modulus) :
            modulus_(static_cast<uchar>(modulus)) {
            img_ = cv::Mat::zeros(rows, cols, CV_8UC1);
            mask_ = cv::Mat::zeros(rows, cols, CV_8UC1);
        }

        void add_circle(int x, int y, int radius) {
            cv::circle(mask_, cv::Point(x, y), radius, cv::Scalar(1), -1);

            int x_start = std::max(x - radius, 0);
            int y_start = std::max(y - radius, 0);

            if (y_start >= mask_.rows || x_start >= mask_.cols) {
                return;
            }

            int x_end = std::min(x + radius, mask_.cols);
            int y_end = std::min(y + radius, mask_.rows);

            if (y_end < 0 || x_end < 0) {
                return;
            }

            cv::Rect roi(x_start, y_start, x_end - x_start, y_end - y_start);
            cv::Mat mat_roi = img_(roi);
            cv::Mat temp_roi = mask_(roi);
            cv::add(mat_roi, temp_roi, mat_roi);
            modulo_in_place(mat_roi, modulus_);

            cv::circle(mask_, cv::Point(x, y), radius, cv::Scalar(0), -1);

        }

        cv::Mat image() const {
            return img_;
        }
    };
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

    std::println("complete.\ngenerating {} ...\n", 
        fs::path(inp.out_file).filename().string()
    );

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

void ici::to_raster(const std::string& outp,
        const std::vector<circle>& circles, const ici::raster_settings& settings) {
    rectangle view_rect = settings.view ? *settings.view : bounds(circles);

    std::println("rasterizing...");
    std::println("  view rect: [({} , {}) - ({} , {})]\n",
        view_rect.min.x, view_rect.min.y,
        view_rect.max.x, view_rect.max.y
    );

    int scale = get_scale(settings.antialiasing_level);
    auto [cols, rows, logical_to_image] = image_metrics(
        view_rect.min, view_rect.max, scale, settings.resolution
    );

    int num_colors = static_cast<int>(settings.color_tbl.size());
    modulo_canvas canvas(cols, rows, num_colors);
    int count = 0;
    for (auto&& circle : circles) {
        if (circles.size() >= 10000) {
            if (count % 10000 == 0) {
                std::println("  rasterized {} circles of {}...",
                    count, circles.size()
                );
            }
        }
        int x = static_cast<int>(std::round((circle.loc.x - view_rect.min.x) * logical_to_image));
        int y = static_cast<int>(std::round((circle.loc.y - view_rect.min.y) * logical_to_image));
        int r = static_cast<int>(std::round(circle.radius * logical_to_image));
        canvas.add_circle(x, y, r);
        count++;
    }
    auto palette = settings.color_tbl | rv::transform(to_cv_color) | r::to<std::vector>();
    auto image = apply_color_table(canvas.image(), palette);

    if (scale > 1) {
        cv::Mat resized;
        cv::Size sz(image.cols / scale, image.rows / scale);
        cv::resize(image, resized, sz, 0, 0, cv::INTER_AREA);
        image = resized.clone();
    }

    cv::imwrite(outp, image);
}

struct raster_context {
    ici::circle_tree circles;
    ici::rectangle view;
    double img_to_log;
    int canvas_sz;
    int antialiasing_level;
    std::vector<ici::color> colors;
};

uint32_t to_uint32(const ici::color& color) {
    return (0xFF << 24) | (color.r << 16) | (color.g << 8) | color.b;
}

ici::rectangle canvas_rect_to_logical_rect(const raster_context& ctxt, const rect& r) {
    auto origin = ctxt.view.min;
    auto x1 = origin.x + r.min.x * ctxt.img_to_log;
    auto y1 = origin.y + r.min.y * ctxt.img_to_log;
    auto x2 = origin.x + (r.max.x+1) * ctxt.img_to_log;
    auto y2 = origin.y + (r.max.y+1) * ctxt.img_to_log;
    return { {x1,y1},{x2,y2} };
}

/*
void rasterize_pixel(const raster_context& ctxt, ici::image& img, int col, int row) {
    if (col < 0 || row < 0 || col >= img.cols() || row >= img.rows()) {
        return;
    }
    //TODO: handle antialiasing...
    auto pt = ici::centroid(
        canvas_rect_to_logical_rect(ctxt, { {col,row},{col,row} })
    );
    auto circles = ctxt.circles.contains(pt);
    img(col, row) = ctxt.colors.at(circles.size() % ctxt.colors.size());
}
*/

int two_to_the_nth(int n) {
    int v = 1;
    for (int i = 0; i < n; ++i) {
        v += 2;
    }
    return v;
}

void rasterize_pixel(const raster_context& ctxt, ici::image& img, int col, int row) {
    if (col < 0 || row < 0 || col >= img.cols() || row >= img.rows()) {
        return;
    }
    auto rect = canvas_rect_to_logical_rect(ctxt, {{col,row},{col,row}} );
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
    img(col, row) = to_uint32(pixel);
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
    auto img_rect = rect{ {0,0},{img.cols()-1,img.rows()-1} };
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

void rasterize_rect(const raster_context& ctxt, ici::image& img, const rect& rect) { 

    auto log_rect = canvas_rect_to_logical_rect(ctxt, rect);
    if (! ici::intersects(log_rect, ctxt.view)) {
        return;
    }

    if (rect.min.x == rect.max.x && rect.min.y == rect.max.y) {
        rasterize_pixel(ctxt, img, rect.min.x, rect.min.y);
        return;
    }

    // if the only circles the rectangle intersects completely contain the rectangle
    // then fill in this rectangle.
    auto count = containing_circle_count(ctxt.circles, log_rect);
    if (count) {
        fill_rect(  img, rect,  to_uint32(ctxt.colors.at(*count % ctxt.colors.size())) );
        return;
    }

    // otherwise, recurse...
    int sz = (rect.max.x - rect.min.x + 1)/2;
    int x1 = rect.min.x;
    int y1 = rect.min.y;
    int x2 = rect.max.x;
    int y2 = rect.max.y;

    std::array<::rect, 4> quadrants = {{
        {{x1 , y1 + sz} , {x1 + sz - 1,y2}}, // northwest
        {{x1 + sz, y1 + sz} , {x2 , y2}}, // northeast
        {{x1 + sz, y1}, {x2, y1 + sz - 1}}, // southeast
        {{x1,y1},{x1 + sz - 1, y1 + sz - 1}} // southwest
    }};

    for (const auto& quadrant : quadrants) {
        rasterize_rect(ctxt, img, quadrant);
    }
}

void ici::to_raster2( const std::string& outp, 
        const std::vector<circle>& inp, const raster_settings& settings) {

    rectangle view_rect = settings.view ? *settings.view : bounds(inp);
    auto [cols, rows, logical_to_image] = image_metrics(
        view_rect.min, view_rect.max, 1.0, settings.resolution
    );

    raster_context ctxt = {
        .circles = {inp},
        .view = view_rect,
        .img_to_log = 1.0 / logical_to_image,
        .canvas_sz = static_cast<int>(
                std::bit_ceil(static_cast<unsigned>(std::max(cols, rows)))
            ),
        .antialiasing_level = settings.antialiasing_level,
        .colors = settings.color_tbl
    };

    ici::image img(cols, rows);
    rasterize_rect(ctxt, img, {{0,0},{ctxt.canvas_sz - 1, ctxt.canvas_sz - 1}} );
    
    ici::write_to_file(outp, img);
}
