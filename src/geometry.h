#pragma once

#include <vector>
#include <optional>

namespace ici {

    struct point {
        double x;
        double y;
    };

    struct circle {
        point loc;
        double radius;
    };

    struct rectangle {
        point min;
        point max;
    };

    point operator+(const point& lhs, const point& rhs);
    point operator-(const point& lhs, const point& rhs);
    point operator*(double lhs, const point& rhs);

    rectangle bounds(const circle& c);
    rectangle bounds(const std::vector<circle>& circles);
    rectangle pad(const rectangle& r, double padding);
    circle scale(const circle& c, double k);
    std::optional<circle> circle_through_three_points(
        const point& pt1, const point& pt2, const point& pt3);

    double distance(const point& pt1, const point& pt2);

}