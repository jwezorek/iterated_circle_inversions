#pragma once

#include "iter_circ_inv.h"
#include <vector>

namespace ici {

    struct input;

    void rasterize(const std::vector<circle>& circles, const ici::input& settings);

}
