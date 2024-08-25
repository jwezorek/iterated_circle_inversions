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

    std::array<ici::point, 4> vertices(const ici::rectangle& r) {
        return {
            r.min,
            {r.max.x, r.min.y},
            r.max,
            {r.min.x, r.max.y}
        };
    }
}

ici::rectangle ici::bounds(const circle& c) {
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

bool ici::circle_rectangle_intersection(const ici::circle& c, const ici::rectangle& r) {
    // from here: https://stackoverflow.com/a/402010/1413244

    auto rect_cen = centroid(r);
    auto half_rect_width = (r.max.x - r.min.x) / 2.0;
    auto half_rect_height = (r.max.y - r.min.y) / 2.0;
    auto circle_dist_x = std::abs(rect_cen.x - c.loc.x);
    auto circle_dist_y = std::abs(rect_cen.y - c.loc.y);

    if (circle_dist_x > half_rect_width + c.radius || circle_dist_y > half_rect_height + c.radius) { 
        return false; 
    }

    if (circle_dist_x <= half_rect_width || circle_dist_y <= half_rect_height) { 
        return true; 
    }

    auto x_diff = circle_dist_x - half_rect_width;
    auto y_diff = circle_dist_y - half_rect_height;
    auto corner_dist_squ = x_diff * x_diff + y_diff * y_diff;

    return corner_dist_squ <= c.radius * c.radius;
}

bool ici::circle_contains_pt(const ici::circle& c, const ici::point& pt) {
    return distance(c.loc, pt) <= c.radius;
}

bool ici::circle_contains_rectangle(const ici::circle& c, const ici::rectangle& r) {
    auto verts = vertices(r);
    auto does_not_contain_vert = [&](auto&& vert) {
        return !circle_contains_pt(c, vert);
    };
    return r::find_if( verts, does_not_contain_vert ) == verts.end();
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

std::optional<ici::point> ici::invert(const circle& c, const point& pt) {

    auto x_diff = pt.x - c.loc.x;
    auto y_diff = pt.y - c.loc.y;
    auto denom = x_diff * x_diff + y_diff * y_diff;
    if (std::abs(denom) < std::numeric_limits<float>::epsilon()) {
        return {};
    }

    return point{
        c.loc.x + (c.radius * c.radius * x_diff) / denom,
        c.loc.y + (c.radius * c.radius * y_diff) / denom
    };

}

std::optional<ici::circle> ici::invert(const circle& c, const circle& invertee)
{
    std::array<std::optional<point>, 4> ary = { {
        invert(c, invertee.loc + point{0, invertee.radius}),    //north
        invert(c, invertee.loc + point{ -invertee.radius, 0 }), //west
        invert(c, invertee.loc + point{ invertee.radius, 0 }),  //east
        invert(c, invertee.loc + point{ 0, -invertee.radius })  //south
    } };

    auto pts = ary | rv::filter(
        [](auto&& p) { return p.has_value(); }
    ) | rv::transform(
        [](auto&& p) { return p.value(); }
    ) | r::to<std::vector>();

    if (pts.size() < 3) {
        return {};
    }

    return circle_through_three_points(pts[0], pts[1], pts[2]);
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

bool ici::intersects(const rectangle& r1, const rectangle& r2) {
    if (r1.max.x < r2.min.x || r2.max.x < r1.min.x) {
        return false;
    }

    if (r1.max.y < r2.min.y || r2.max.y < r1.min.y) {
        return false;
    }

    return true;
}

bool ici::contains(const ici::rectangle& r, const ici::point& pt) {
    return pt.x >= r.min.x && pt.x <= r.max.x &&
        pt.y >= r.min.y && pt.y <= r.max.y;
}

ici::point ici::centroid(const rectangle& r) {
    return {
        (r.min.x + r.max.x) / 2.0,
        (r.min.y + r.max.y) / 2.0
    };
}

ici::rectangle ici::bounds(const std::vector<ici::circle>& circles) {
    auto rects = circles | rv::transform(
            [](const ici::circle& c) {return bounds(c); }
        ) | r::to<std::vector>();

    auto x1 = r::min(rects | rv::transform([](auto&& r) { return r.min.x; }));
    auto y1 = r::min(rects | rv::transform([](auto&& r) { return r.min.y; }));
    auto x2 = r::max(rects | rv::transform([](auto&& r) { return r.max.x; }));
    auto y2 = r::max(rects | rv::transform([](auto&& r) { return r.max.y; }));

    return { {x1,y1},{x2,y2} };
}