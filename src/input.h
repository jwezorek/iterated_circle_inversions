#pragma once

#include "iter_circ_inv.h"
#include <vector>
#include <optional>

/*------------------------------------------------------------------------------------------------*/

namespace ici {

    struct input {
        std::string fname;
        std::vector<circle> circles;
        double eps;
        int iterations;
        std::string out_file;
        bool rasterize;
    };

    std::optional<const input> parse_input(const std::string& inp_file);

}