#pragma once

#include <vector>
#include <optional>
#include <string>
#include "image.h"
#include "geometry.h"

/*------------------------------------------------------------------------------------------------*/

namespace ici {

    struct input;
    struct circle;
    struct vector_settings;
    struct raster_settings;

    std::vector<circle> invert_circles(const ici::input& inp);

    void to_svg(const std::string& fname, const std::vector<circle>& circles,
        const vector_settings& settings);

    ici::image to_raster(const std::string& outp, const rectangle& view_rect,
        const std::vector<circle>& inp, const raster_settings& settings);
}