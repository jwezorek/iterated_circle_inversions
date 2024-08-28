#include <print>
#include <optional>
#include <string>
#include <ranges>
#include <numbers>
#include <filesystem>
#include "geometry.h"
#include "iterated_inversion.h"
#include "input.h"
#include "util.h"
#include <expected>
#include <stdexcept>
#include <chrono>
#include <opencv2/opencv.hpp>

namespace fs = std::filesystem;
namespace r = std::ranges;
namespace rv = std::ranges::views;

/*------------------------------------------------------------------------------------------------*/

namespace {

    std::expected<const ici::input, std::runtime_error> parse_cmd_line(int argc, char* argv[]) {
        if (argc != 2) {
            return std::unexpected(
                std::runtime_error("missing required .json file.")
            );
        }
        return ici::parse_input(argv[1]);
    }
}

double interp(double start, double end, double t) {
    return (end - start) * t + start;
}

std::vector<ici::circle> interpolate(const std::vector<ici::circle>& from, const std::vector<ici::circle>& to, double t) {
    std::vector<ici::circle> output;
    for (const auto& [c1, c2] : rv::zip(from, to)) {
        output.push_back({
            {interp(c1.loc.x, c2.loc.x, t), interp(c1.loc.y, c2.loc.y, t)},
            interp(c1.radius, c1.radius, t)
            }
        );
    }
    return output;
}

ici::rectangle interpolate_rect(const ici::rectangle& from, const ici::rectangle& to, double t) {
    return {
        {interp(from.min.x, to.min.x, t), interp(from.min.y, to.min.y, t)},
        {interp(from.max.x, to.max.x, t), interp(from.max.y, to.max.y, t)}
    };
}

cv::Mat image_to_mat(const ici::image& img) {
    return cv::Mat(img.rows(), img.cols(), CV_8UC4, img.data());
}

int main(int argc, char* argv[]) {

    auto start = ici::parse_input("D:\\test\\pentagon.json");
    auto end = ici::parse_input("D:\\test\\test.json");

    auto start_rect = ici::bounds(start->circles);
    auto end_rect = ici::bounds(end->circles);
    auto rast_settings = std::get<ici::raster_settings>(start->output_settings);
    rast_settings.resolution = 640;

    // Set the video output properties
    int frame_width = 640;
    int frame_height = 640;
    int fps = 24;

    int num_frames = fps * 30;

    // Create a VideoWriter object
    cv::VideoWriter video("D:/test/pentagon2.avi",
        cv::VideoWriter::fourcc('M', 'J', 'P', 'G'),
        fps,
        cv::Size(frame_width, frame_height));

    // Generate frames and write them to the video
    for (int i = 0; i < num_frames; ++i) {
        std::println("frame {} of {}", i, num_frames);
        auto t = static_cast<double>(i) / static_cast<double>(num_frames);
        auto circles = interpolate(start->circles, end->circles, t);
        auto rect = interpolate_rect(start_rect, end_rect, t);
        auto inp = *start;
        inp.circles = circles;
        circles = ici::invert_circles(inp);
       
        auto img = ici::to_raster("", rect, circles, rast_settings);
        auto mat = image_to_mat(img);
        if (mat.cols > 640 || mat.rows > 640) {
            int x = (mat.cols - 640) / 2;
            int y = (mat.rows - 640) / 2;
            mat = mat(cv::Rect(x, y, 640, 640)).clone();
        }
        
        video.write(mat);
    }

    // Release the video writer
    video.release();

    /*
    try {
        auto input = parse_cmd_line(argc, argv);
        if (!input.has_value()) {
            throw input.error();
        }

        auto circles = ici::invert_circles(*input);

        auto fname = fs::path(input->out_file).filename().string();
        std::println("");
        if (std::holds_alternative<ici::vector_settings>(input->output_settings)) {
            std::println("serializing circles to svg ({})...", fname);
            ici::to_svg(
                input->out_file, 
                circles, 
                std::get<ici::vector_settings>(input->output_settings)
            );
        } else {
            std::println("rasterizing {} circles...",  circles.size());

            const auto& settings = std::get<ici::raster_settings>(input->output_settings);
            ici::rectangle view_rect = settings.view ? *settings.view : ici::bounds(circles);
            std::println("  view rect: [ {}, {}, {}, {} ]",
                view_rect.min.x, view_rect.min.y, view_rect.max.x, view_rect.max.y
            );

            auto img = ici::to_raster(input->out_file, view_rect, circles, settings);
            std::println("serializing to {} format ({})...",
                fs::path(fname).extension().string(),
                fname
            );
            ici::write_to_file(input->out_file, img);
        }

        std::println("complete.");
        return 0;

    } catch (std::runtime_error e) {
        std::println("error : {}", e.what());
    } catch (...) {
        std::println("error : {}", "unknown error");
    }

    return -1;
    */
}
