#include "geometry.h"
#include <complex>
#include <ranges>

namespace r = std::ranges;
namespace rv = std::ranges::views;

/*------------------------------------------------------------------------------------------------*/

namespace {

    std::complex<double> pt_to_z(const ici::point& pt) {
        auto [x, y] = pt;
        return { x,y };
    }
}

ici::rectangle ici::circle_bounds(const circle& c) {
    return {
        {c.loc.x - c.radius, c.loc.y - c.radius},
        {c.loc.x + c.radius, c.loc.y + c.radius}
    };
}

ici::circle ici::scale(const circle& c, double k) {
    return {
        {k * c.loc.x, k * c.loc.y},
        k * c.radius
    };
}

std::optional<ici::circle> ici::circle_through_three_points(
    const point& pt1, const point& pt2, const point& pt3) {

    using namespace std::complex_literals;
    auto z1 = pt_to_z(pt1), z2 = pt_to_z(pt2), z3 = pt_to_z(pt3);
    auto w = (z3 - z1) / (z2 - z1);

    if (std::abs(w.imag()) < std::numeric_limits<float>::epsilon()) {
        return {}; // the three points are colinear
    }

    auto magnitude_w = std::abs(w);
    auto c = (z2 - z1) * (w - magnitude_w * magnitude_w) / (2i * w.imag()) + z1;
    auto r = std::abs(z1 - c);

    return ici::circle{ {c.real(), c.imag()}, r };
}

ici::point ici::operator+(const ici::point& lhs, const ici::point& rhs) {
    return {
        lhs.x + rhs.x,
        lhs.y + rhs.y
    };
}

ici::point ici::operator-(const ici::point& lhs, const ici::point& rhs) {
    return {
        lhs.x - rhs.x,
        lhs.y - rhs.y
    };
}

ici::point ici::operator*(double lhs, const ici::point& rhs) {
    return {
        lhs * rhs.x,
        lhs * rhs.y
    };
}

double ici::distance(const point& pt1, const point& pt2)
{
    auto x_diff = pt1.x - pt2.x;
    auto y_diff = pt1.y - pt2.y;
    return std::sqrt(x_diff * x_diff + y_diff * y_diff);
}

ici::rectangle ici::pad(const ici::rectangle& r, double padding) {
    return {
        { r.min.x - padding, r.min.y - padding },
        { r.max.x + padding, r.max.y + padding }
    };
}

ici::rectangle ici::circles_bounds(const std::vector<ici::circle>& circles) {
    auto bounds = circles | rv::transform(circle_bounds) | r::to<std::vector>();

    auto x1 = r::min(bounds | rv::transform([](auto&& r) { return r.min.x; }));
    auto y1 = r::min(bounds | rv::transform([](auto&& r) { return r.min.y; }));
    auto x2 = r::max(bounds | rv::transform([](auto&& r) { return r.max.x; }));
    auto y2 = r::max(bounds | rv::transform([](auto&& r) { return r.max.y; }));

    return { {x1,y1},{x2,y2} };
}