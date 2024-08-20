#include "input.h"
#include "third-party/json.hpp"
#include <fstream>
#include <filesystem>
#include <ranges>

namespace fs = std::filesystem;
namespace r = std::ranges;
namespace rv = std::ranges::views;

/*------------------------------------------------------------------------------------------------*/

namespace {
    using json = nlohmann::json;

    constexpr auto k_epsilon = 0.0001;
    constexpr auto k_num_iterations = 2;
    constexpr auto k_default_res = 2048;
    constexpr auto k_default_aa_level = 0;
    constexpr auto k_default_scale = 100.0;
    constexpr auto k_default_padding = 10.0;
    constexpr auto k_default_out_fname = "circle_inv.svg"; 
    constexpr auto k_eps_field = "eps";
    constexpr auto k_iters_field = "iterations";
    constexpr auto k_output_res_field = "resolution";
    constexpr auto k_antialias_field = "antialiasing_level";
    constexpr auto k_colors_field = "colors";
    constexpr auto k_scale_field = "scale";
    constexpr auto k_padding_field = "padding";

    constexpr auto k_out_file = "out-file";

    std::expected<json, std::runtime_error> file_to_json(const std::string& inp_file) {
        std::ifstream file(inp_file);

        if (!file.is_open()) {
            return std::unexpected(
                std::runtime_error(
                    std::format("'{}' not found / cannot be opened", inp_file)
                )
            );
        }

        try {
            json json_data;
            file >> json_data;
            return json_data;
        }
        catch (const json::parse_error& e) {
            return std::unexpected(std::runtime_error(e.what()));
        }
    }

    ici::circle json_ary_to_circle(const json& json) {
        if (!json.is_array()) {
            throw std::runtime_error("incorrect json");
        }
        return {
            {json.at(0).get<double>(), json.at(1).get<double>()},
            json.at(2).get<double>()
        };
    }

    std::vector<ici::circle> json_to_circles(const std::vector<json>& ary) {
        return ary | rv::transform(json_ary_to_circle) | r::to<std::vector>();
    }

    double get_eps(const json& json) {
        if (!json.contains(k_eps_field)) {
            return k_epsilon;
        }
        return json[k_eps_field].get<double>();
    }

    int get_num_iterations(const json& json) {
        if (!json.contains(k_iters_field)) {
            return k_epsilon;
        }
        return json[k_iters_field].get<int>();
    }

    std::string get_out_file(const json& json, const std::string& inp_file) {
        auto input_dir = fs::path(inp_file).parent_path();
        if (!json.contains(k_out_file)) {
            auto out_path = input_dir / k_default_out_fname;
            return out_path.string();
        }
        auto out_path = json[k_out_file].get<std::string>();
        auto dir = fs::path(out_path).parent_path();
        if (dir.empty()) {
            return (input_dir / out_path).string();
        } 
        return out_path;
    }

    ici::color str_to_color(const std::string& str) {
        auto hex = (str.size() == 7 && str.front() == '#') ?
            str.substr(1, 6) : str;
        if (hex.size() != 6) {
            throw std::runtime_error("bad color string");
        }

        std::array<uint8_t, 3> vals;
        for (int i = 0; i < 6; i += 2) {
            vals[i / 2] = static_cast<uint8_t>(std::stoi(hex.substr(i, 2), nullptr, 16));
        }
        return {
            vals[2],
            vals[1],
            vals[0]
        };
    }

    std::vector<ici::color> get_color_table(const json& json) {
        if (!json.contains(k_colors_field)) {
            return { {0,0,0},{255,255,255} };
        }
        auto color_strs = json[k_colors_field].get<std::vector<std::string>>();
        return color_strs | rv::transform(str_to_color) | r::to<std::vector>();
    }

    std::optional<ici::raster_settings> get_raster_output_settings(
            std::string& outfile, const json& json) {
        if (fs::path(outfile).extension() != ".png") {
            return {};
        }

        return ici::raster_settings{
            json.contains(k_output_res_field) ?
                json[k_output_res_field].get<int>() :
                k_default_res,
            json.contains(k_antialias_field) ?
                json[k_antialias_field].get<int>() :
                k_default_aa_level,
            get_color_table(json)
        };
    }

    std::optional<ici::vector_settings> get_vector_output_settings(
        std::string& outfile, const json& json) {
        if (fs::path(outfile).extension() != ".svg") {
            return {};
        }

        return ici::vector_settings{
            json.contains(k_scale_field) ?
                json[k_scale_field].get<double>() :
                k_default_scale,
            json.contains(k_padding_field) ?
                json[k_padding_field].get<double>() :
                k_default_padding
        };
    }

    std::variant<ici::vector_settings, ici::raster_settings> get_output_settings(
            std::string& outfile, const json& json) {
        auto raster = get_raster_output_settings(outfile, json);
        if (raster) {
            return *raster;
        }
        return *get_vector_output_settings(outfile, json);
    }

    std::expected<const ici::input, std::runtime_error> json_to_input(
            const std::string& inp_file, const json& json) {

        if (!json.is_object()) {
            return std::unexpected(
                std::runtime_error("invalid JSON, no toplevel object")
            );
        }

        if (!json.contains("circles")) {
            return std::unexpected(
                std::runtime_error("invalid JSON, no initial circles")
            );
        }

        auto outp = get_out_file(json, inp_file);

        return ici::input{
            .fname = fs::path(inp_file).filename().string(),
            .circles = json_to_circles( 
                json["circles"].get<std::vector<::json>>()
            ),
            .eps = get_eps( json ),
            .iterations = get_num_iterations( json ),
            .out_file = outp,
            .output_settings = get_output_settings(outp, json)
        };
    }
}

std::expected<const ici::input, std::runtime_error> ici::parse_input(const std::string& inp_file)
{
    try {
        auto maybe_json = file_to_json(inp_file);
        if (!maybe_json.has_value()) {
            return std::unexpected(maybe_json.error());
        }
        return json_to_input(inp_file, *maybe_json);
    } catch (std::runtime_error e) {
        return std::unexpected(e);
    } catch (...) {
        return std::unexpected(std::runtime_error("unknown error while parsing input"));
    }
}
