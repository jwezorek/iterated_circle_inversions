#pragma once

#include "iter_circ_inv.h"
#include <vector>
#include <optional>

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

    std::optional<const input> parse_input(const std::string& inp_file);

}