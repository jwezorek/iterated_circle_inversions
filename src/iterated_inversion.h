#pragma once

#include <vector>
#include <optional>
#include <string>

/*------------------------------------------------------------------------------------------------*/

namespace ici {

    struct input;
    struct circle;
    struct raster_output_settings;

    std::vector<circle> perform_inversions(const ici::input& inp);

    void to_svg(const std::string& fname, const std::vector<circle>& circles,
        double padding, double scale);

    void to_raster(const std::string& outp, const std::vector<circle>& circles,
        const ici::raster_output_settings& settings);
}