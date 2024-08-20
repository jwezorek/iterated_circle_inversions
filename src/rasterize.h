#pragma once

#include <vector>
#include <string>

/*------------------------------------------------------------------------------------------------*/

namespace ici {

    struct raster_output_settings;
    struct circle;

    void rasterize(const std::string& outp, const std::vector<circle>& circles, 
        const ici::raster_output_settings& settings);

}
