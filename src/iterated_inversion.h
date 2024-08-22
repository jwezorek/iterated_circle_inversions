#pragma once

#include <vector>
#include <optional>
#include <string>

/*------------------------------------------------------------------------------------------------*/

namespace ici {

    struct input;
    struct circle;
    struct vector_settings;
    struct raster_settings;

    std::vector<circle> perform_inversions(const ici::input& inp);

    std::vector<circle> invert_circles(const ici::input& inp);

    void to_svg(const std::string& fname, const std::vector<circle>& circles,
        const vector_settings& settings);

    void to_raster(const std::string& outp, const std::vector<circle>& circles,
        const raster_settings& settings);
}