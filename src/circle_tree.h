#pragma once

#include "geometry.h"
#include <boost/geometry.hpp>
#include <boost/geometry/index/rtree.hpp>
#include <boost/geometry/geometries/box.hpp>
#include <boost/geometry/geometries/register/point.hpp>
#include <vector>
#include <ranges>

namespace ici {

    class circle_tree {
        using vec2 = boost::geometry::model::point<double, 2, boost::geometry::cs::cartesian>;
        using box = boost::geometry::model::box<vec2>;
        using rtree_value = std::pair<box, ici::circle>;
        using rtree = boost::geometry::index::rtree<rtree_value, boost::geometry::index::quadratic<16>>;

        rtree impl_;

    public:
        circle_tree();
        circle_tree(std::ranges::forward_range auto circles) {
            for (auto&& c : circles) {
                insert(c);
            }
        }

        void insert(const ici::circle& c);
        std::vector<ici::circle> intersects(const ici::rectangle& r) const;
        std::vector<ici::circle> contains(const ici::point& pt) const;
    };

}