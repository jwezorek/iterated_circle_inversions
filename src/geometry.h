#pragma once

#include <vector>
#include <optional>

namespace ici {

    template<typename T>
    struct point_type {
        T x;
        T y;
    };

    template<typename T>
    struct rect_type {
        point_type<T> min;
        point_type<T> max;
    };

    using point = point_type<double>;
    using rectangle = rect_type<double>;

    struct circle {
        point loc;
        double radius;
    };

    point operator+(const point& lhs, const point& rhs);
    point operator-(const point& lhs, const point& rhs);
    point operator*(double lhs, const point& rhs);
    std::optional<point> invert(const circle& c, const point& pt);

    rectangle bounds(const circle& c);
    rectangle bounds(const std::vector<circle>& circles);
    rectangle pad(const rectangle& r, double padding);
    bool intersects(const rectangle& r1, const rectangle& r2);
    bool contains(const rectangle& r1, const point& pt);
    point centroid(const rectangle& r);

    circle scale(const circle& c, double k);
    bool circle_rectangle_intersection(const circle& c, const rectangle& r);
    bool circle_contains_pt(const circle& c, const point& pt);
    bool circle_contains_rectangle(const circle& c, const rectangle& r);

    std::optional<circle> invert(const circle& c, const circle& invertee);
    std::optional<circle> circle_through_three_points(
        const point& pt1, const point& pt2, const point& pt3);

    double distance(const point& pt1, const point& pt2);

}