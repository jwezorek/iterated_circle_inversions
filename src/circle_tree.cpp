#include "circle_tree.h"
#include <ranges>
#include <iterator>

namespace r = std::ranges;
namespace rv = std::ranges::views;

/*------------------------------------------------------------------------------------------------*/
namespace {
    namespace bg = boost::geometry;
    namespace bgi = boost::geometry::index;
    using box = bg::model::box<bg::model::point<double, 2, bg::cs::cartesian>>;

    box to_box(const ici::rectangle& r) {
        return {
            {r.min.x, r.min.y},
            {r.max.x, r.max.y}
        };
    }

}

ici::circle_tree::circle_tree() {
}

void ici::circle_tree::insert(const ici::circle& c)
{
    impl_.insert(rtree_value(to_box(bounds(c)), c));
}

std::vector<ici::circle> ici::circle_tree::intersects(const ici::rectangle& r) const {
    std::vector<rtree_value> results;
    impl_.query(bgi::intersects(to_box(r)), std::back_inserter(results));
    return results | rv::values | rv::filter(
            [&](const circle& c) {
                return ici::circle_rectangle_intersection(c, r);
            }
        ) | r::to<std::vector>();
}

std::vector<ici::circle> ici::circle_tree::contains(const ici::point& pt) const {
    std::vector<rtree_value> results;
    impl_.query(bgi::intersects(bg::make<vec2>(pt.x,pt.y)), std::back_inserter(results));
    return results | rv::values | rv::filter(
        [&](const circle& c) {
            return ici::circle_contains_pt(c, pt);
        }
    ) | r::to<std::vector>();
}
