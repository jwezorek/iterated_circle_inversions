#pragma once

#include "geometry.h"
#include <memory>
#include <ranges>
#include <boost/geometry.hpp>
#include <boost/geometry/index/rtree.hpp>
#include <boost/geometry/geometries/box.hpp>
#include <boost/geometry/geometries/register/point.hpp>

namespace ici {

    namespace detail {
        using vec3 = boost::geometry::model::point<double, 3, boost::geometry::cs::cartesian>;
        using box = boost::geometry::model::box<vec3>;
        using rtree = boost::geometry::index::rtree<vec3, boost::geometry::index::quadratic<16>>;
    }

    class circle_set {

        detail::rtree tree_;
        double eps_;

    public:
        circle_set(double eps);
        circle_set(double eps, std::ranges::forward_range auto circles) : circle_set(eps) {
            for (auto&& c : circles) {
                insert(c);
            }
        }
        void insert(const circle& c);
        std::vector<circle> to_vector() const;
        double eps() const;
        bool empty() const;
        size_t size() const;
    };

}