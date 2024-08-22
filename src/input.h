#pragma once

#include "geometry.h"
#include <vector>
#include <variant>
#include <expected>
#include <stdexcept>

/*------------------------------------------------------------------------------------------------*/

namespace ici {

    struct color {
        uint8_t r;
        uint8_t g;
        uint8_t b;
    };

    struct raster_settings {
        int resolution;
        int antialiasing_level;
        std::vector<color> color_tbl;
        std::optional<rectangle> view;
    };

    struct vector_settings {
        double scale;
        double padding;
        std::string bkgd_color;
        std::string color;
        std::string blend_mode;
    };

    struct input {
        std::string fname;
        std::vector<circle> circles;
        double eps;
        int iterations;
        std::string out_file;
        std::variant<vector_settings, raster_settings> output_settings;
    };

    std::expected<const input, std::runtime_error> parse_input(const std::string& inp_file);

}