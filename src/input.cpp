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
    constexpr auto k_default_out_fname = "circle_inv.svg"; 
    constexpr auto k_eps_field = "eps";
    constexpr auto k_iters_field = "iterations";
    constexpr auto k_output_res_field = "resolution";
    constexpr auto k_antialias_field = "antialiasing_level";

    constexpr auto k_out_file = "out-file";

    std::optional<json> file_to_json(const std::string& inp_file) {
        std::ifstream file(inp_file);

        if (!file.is_open()) {
            return {};
        }

        try {
            json json_data;
            file >> json_data;
            return json_data;
        }
        catch (const json::parse_error& e) {
            return {};
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

    std::optional<ici::raster_output_settings> get_raster_output_settings(
            std::string& outfile, const json& json) {
        if (fs::path(outfile).extension() != ".png") {
            return {};
        }
        /*
        std::vector<ici::color> colors = {
            {0,0,0},
            {255,255,255}
        };
        */

        std::vector<ici::color> colors = {
            {255,   0,   0},
            {0,   255,   0},
            {0,     0, 255}
        };

        return ici::raster_output_settings{
            json.contains(k_output_res_field) ?
                json[k_output_res_field].get<int>() :
                k_default_res,
            json.contains(k_antialias_field) ?
                json[k_antialias_field].get<int>() :
                k_default_aa_level,
            colors
        };
    }

    std::optional<const ici::input> json_to_input(
            const std::string& inp_file, const json& json) {

        if (!json.is_object()) {
            return {};
        }

        if (!json.contains("circles")) {
            return {};
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
            .raster = get_raster_output_settings(outp, json)
        };
    }
}

std::optional<const ici::input> ici::parse_input(const std::string& inp_file)
{
    try {
        auto maybe_json = file_to_json(inp_file);
        if (!maybe_json) {
            return {};
        }
        return json_to_input(inp_file, *maybe_json);
    } catch (...) {
        return {};
    }
}
