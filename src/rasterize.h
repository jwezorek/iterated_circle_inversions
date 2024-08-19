#pragma once

#include "iter_circ_inv.h"
#include <vector>

/*------------------------------------------------------------------------------------------------*/

namespace ici {

    struct raster_output_settings;

    void rasterize(const std::string& outp, const std::vector<circle>& circles, 
        const ici::raster_output_settings& settings);

}
