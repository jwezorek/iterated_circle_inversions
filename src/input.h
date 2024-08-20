#pragma once

#include "geometry.h"
#include <vector>
#include <optional>
#include <expected>
#include <stdexcept>

/*------------------------------------------------------------------------------------------------*/

namespace ici {

    struct color {
        uint8_t r;
        uint8_t g;
        uint8_t b;
    };

    struct raster_output_settings {
        int resolution;
        int antialiasing_level;
        std::vector<color> color_tbl;
    };

    struct input {
        std::string fname;
        std::vector<circle> circles;
        double eps;
        int iterations;
        std::string out_file;
        std::optional<raster_output_settings> raster;
    };

    std::expected<const input, std::runtime_error> parse_input(const std::string& inp_file);

}