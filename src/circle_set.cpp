#include "circle_set.h"
#include <ranges>

namespace r = std::ranges;
namespace rv = std::ranges::views;
namespace bg = boost::geometry;
namespace bgi = boost::geometry::index;

namespace {
    using box = cir::detail::box;
    using vec3 = cir::detail::vec3;

    using rtree = cir::detail::rtree;

    vec3 to_vec3(const cir::circle& c) {
        return {
            c.loc.x,
            c.loc.y,
            c.radius
        };
    }

    box pad_vec3(const vec3& v, double k) {
        auto x = bg::get<0>(v);
        auto y = bg::get<1>(v);
        auto z = bg::get<2>(v);
        return {
            {x - k, y - k, z - k},
            {x + k, y + k, z + k}
        };
    }

    double dist(const vec3& v1, const vec3& v2) {

        auto x1 = bg::get<0>(v1);
        auto y1 = bg::get<1>(v1);
        auto z1 = bg::get<2>(v1);

        auto x2 = bg::get<0>(v2);
        auto y2 = bg::get<1>(v2);
        auto z2 = bg::get<2>(v2);

        auto x_diff = x1 - x2;
        auto y_diff = y1 - y2;
        auto z_diff = z1 - z2;

        return std::sqrt(
            x_diff * x_diff + y_diff * y_diff + z_diff * z_diff
        );
    }

    cir::circle to_circle(const vec3& v) {
        return {
            {bg::get<0>(v), bg::get<1>(v)},
            bg::get<2>(v)
        };
    }
}

cir::circle_set::circle_set(double eps) : eps_(eps)
{
}

void cir::circle_set::insert(const circle& c) {
    std::vector<vec3> results;
    auto v = to_vec3(c);
    tree_.query(bgi::intersects(pad_vec3(v, eps_)), std::back_inserter(results));
    if (results.empty()) {
        tree_.insert(v);
        return;
    }
    auto is_in_set = [&](auto&& res) {return dist(res, v) <= eps_; };
    if (r::find_if(results, is_in_set) != results.end()) {
        return;
    }
    tree_.insert(v);
}

std::vector<cir::circle> cir::circle_set::to_vector() const {
    return tree_ | rv::transform(to_circle) | r::to<std::vector>();
}
