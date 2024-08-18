#include "iter_circ_inv.h"
#include "circle_set.h"
#include "util.h"
#include <print>
#include <sstream>
#include <ranges>
#include <complex>

namespace r = std::ranges;
namespace rv = std::ranges::views;

/*------------------------------------------------------------------------------------------------*/

namespace {

    std::complex<double> pt_to_z(const cir::point& pt) {
        auto [x, y] = pt;
        return { x,y };
    }

}

cir::rectangle cir::bounds(const circle& c) {
    return {
        {c.loc.x - c.radius, c.loc.y - c.radius},
        {c.loc.x + c.radius, c.loc.y + c.radius}
    };
}

cir::circle cir::scale(const circle& c, double k) {
    return {
        {k * c.loc.x, k * c.loc.y},
        k * c.radius
    };
}

std::optional<cir::circle> cir::circle_through_three_points(
        const point& pt1, const point& pt2, const point& pt3) {

    using namespace std::complex_literals;
    auto z1 = pt_to_z(pt1), z2 = pt_to_z(pt2), z3 = pt_to_z(pt3);
    auto w = (z3 - z1) / (z2 - z1);

    if (w.imag() == 0) {
        return {}; // the three points are colinear
    }

    auto magnitude_w = std::abs(w);
    auto c = (z2 - z1) * (w - magnitude_w * magnitude_w) / (2i * w.imag()) + z1;
    auto r = std::abs(z1 - c);

    return cir::circle{ {c.real(), c.imag()}, r };
}

cir::point cir::operator+(const cir::point& lhs, const cir::point& rhs) {
    return {
        lhs.x + rhs.x,
        lhs.y + rhs.y
    };
}

cir::point cir::operator-(const cir::point& lhs, const cir::point& rhs) {
    return {
        lhs.x - rhs.x,
        lhs.y - rhs.y
    };
}

cir::point cir::operator*(double lhs, const cir::point& rhs) {
    return {
        lhs * rhs.x,
        lhs * rhs.y
    };
}

cir::point cir::invert(const circle& c, const point& pt) {
    auto x0 = c.loc.x;
    auto y0 = c.loc.y;
    auto k = c.radius;
    auto x_diff = pt.x - x0;
    auto y_diff = pt.y - y0;
    auto denom = x_diff * x_diff + y_diff * y_diff;

    return {
        x0 + (k * k * x_diff) / denom,
        y0 + (k * k * y_diff) / denom
    };
}

cir::circle cir::invert(const circle& c, const circle& invertee)
{
    auto north = invert(c, invertee.loc + point{0, invertee.radius});
    auto west = invert(c, invertee.loc + point{ -invertee.radius, 0 });
    auto east = invert(c, invertee.loc + point{ invertee.radius, 0 });
    
    return *circle_through_three_points(north, west, east);
}

double cir::distance(const point& pt1, const point& pt2)
{
    auto x_diff = pt1.x - pt2.x;
    auto y_diff = pt1.y - pt2.y;
    return std::sqrt(x_diff * x_diff + y_diff * y_diff);
}

cir::rectangle cir::pad(const cir::rectangle& r, double padding) {
    return {
        { r.min.x - padding, r.min.y - padding },
        { r.max.x + padding, r.max.y + padding }
    };
}

std::vector<cir::circle> cir::do_one_round(const std::vector<circle>& circles) {
    circle_set new_circles(0.001);
	for (const auto& [c1, c2] : two_combinations(circles)) {
        new_circles.insert(c1);
        new_circles.insert(c2);
        new_circles.insert( invert(c1, c2) );
        new_circles.insert( invert(c2, c1) );
	}
    return new_circles.to_vector();
}


