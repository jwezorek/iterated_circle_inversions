#pragma once

#include "iter_circ_inv.h"
#include <vector>
#include <optional>

/*------------------------------------------------------------------------------------------------*/

namespace ici {

    struct settings {
        std::string fname;
        std::vector<circle> circles;
        double eps;
        int iterations;
        std::string out_file;
    };

    std::optional<const settings> parse_input(const std::string& inp_file);

}