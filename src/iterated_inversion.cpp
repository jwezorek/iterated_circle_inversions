#include "iterated_inversion.h"
#include "geometry.h"
#include "circle_set.h"
#include "input.h"
#include "util.h"
#include <opencv2/opencv.hpp>
#include <print>
#include <sstream>
#include <ranges>
#include <complex>
#include <filesystem>

namespace fs = std::filesystem;
namespace r = std::ranges;
namespace rv = std::ranges::views;

/*------------------------------------------------------------------------------------------------*/
namespace {

    std::vector<ici::circle> do_one_round(const std::vector<ici::circle>& circles, double eps) {
        ici::circle_set new_circles(eps);
        for (const auto& [c1, c2] : ici::two_combinations(circles)) {
            new_circles.insert(c1);
            new_circles.insert(c2);

            auto c = invert(c1, c2);
            auto d = invert(c2, c1);

            if (c) {
                new_circles.insert(*c);
            }
            if (d) {
                new_circles.insert(*d);
            }
        }
        return new_circles.to_vector();
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
            int x_end = std::min(x + radius, mask_.cols);
            int y_end = std::min(y + radius, mask_.rows);

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

std::vector<ici::circle> ici::perform_inversions(const ici::input& inp)
{
    std::println("inverting {}...", inp.fname);

    auto circles = inp.circles;
    for (int i : rv::iota(0, inp.iterations)) {
        std::print("  iteration {}: {} circles ->", i + 1, circles.size());
        circles = do_one_round(circles, inp.eps);
        std::println(" {} circles...", circles.size());
    }

    std::println("complete.\ngenerating {} ...\n", fs::path(inp.out_file).filename().string());
    return circles;
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
        "<svg style=\"{}\" viewBox=\"{} {} {} {}\" xmlns=\"http://www.w3.org/2000/svg\">\n",
        "stroke-width: 1px; background-color: white;",
        dimensions.min.x,
        dimensions.min.y,
        dimensions.max.x - dimensions.min.x,
        dimensions.max.y - dimensions.min.y
    );

    for (const auto& c : circles) {
        ss << std::format(
            "    <circle cx=\"{}\" cy=\"{}\" r=\"{}\" fill=\"none\" stroke=\"black\" />\n",
            c.loc.x, c.loc.y, c.radius
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
        if (circles.size() > 10000) {
            if (count % 1000 == 0) {
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
