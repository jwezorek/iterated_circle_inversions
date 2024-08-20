#pragma once

#include <vector>
#include <optional>

/*------------------------------------------------------------------------------------------------*/

namespace ici {

    struct input;
    struct point;
    struct circle;

    std::optional<point> invert(const circle& c, const point& pt);
    std::optional<circle> invert(const circle& c, const circle& invertee);

    std::vector<circle> perform_inversions(const ici::input& inp);

}