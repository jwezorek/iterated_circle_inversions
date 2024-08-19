#include "iter_circ_inv.h"
#include "circle_set.h"
#include "input.h"
#include "util.h"
#include <print>
#include <sstream>
#include <ranges>
#include <complex>
#include <filesystem>

namespace fs = std::filesystem;
namespace r = std::ranges;
namespace rv = std::ranges::views;

/*------------------------------------------------------------------------------------------------*/

namespace {

    std::complex<double> pt_to_z(const ici::point& pt) {
        auto [x, y] = pt;
        return { x,y };
    }

    std::vector<ici::circle> do_one_round(const std::vector<ici::circle>& circles, double eps) {
        ici::circle_set new_circles(eps);
        for (const auto& [c1, c2] : ici::two_combinations(circles)) {
            new_circles.insert(c1);
            new_circles.insert(c2);

            auto c = invert(c1, c2);
            auto d = invert(c2, c1);

            if (c) {
                new_circles.insert(*c);
            }
            if (d) {
                new_circles.insert(*d);
            }
        }
        return new_circles.to_vector();
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

std::optional<ici::point> ici::invert(const circle& c, const point& pt) {
    auto x0 = c.loc.x;
    auto y0 = c.loc.y;
    auto k = c.radius;
    auto x_diff = pt.x - x0;
    auto y_diff = pt.y - y0;
    auto denom = x_diff * x_diff + y_diff * y_diff;
    if (std::abs(denom) < std::numeric_limits<float>::epsilon()) {
        return {};
    }

    return point{
        x0 + (k * k * x_diff) / denom,
        y0 + (k * k * y_diff) / denom
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

ici::rectangle ici::circles_bounds(const std::vector<ici::circle>& circles) {
    auto bounds = circles | rv::transform(circle_bounds) | r::to<std::vector>();

    auto x1 = r::min(bounds | rv::transform([](auto&& r) { return r.min.x; }));
    auto y1 = r::min(bounds | rv::transform([](auto&& r) { return r.min.y; }));
    auto x2 = r::max(bounds | rv::transform([](auto&& r) { return r.max.x; }));
    auto y2 = r::max(bounds | rv::transform([](auto&& r) { return r.max.y; }));

    return { {x1,y1},{x2,y2} };
}

std::vector<ici::circle> ici::perform_inversions(const ici::input& inp)
{
    std::println("inverting {}...", inp.fname);

    auto circles = inp.circles;
    for (int i : rv::iota(0, inp.iterations)) {
        std::print("  iteration {}: {} circles ->", i + 1, circles.size());
        circles = do_one_round(circles, inp.eps);
        std::println(" {} circles...", circles.size());
    }

    std::println("complete.\ngenerating {} ...\n", fs::path(inp.out_file).filename().string());
    return circles;
}


