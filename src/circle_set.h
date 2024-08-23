#pragma once

#include "geometry.h"
#include <memory>
#include <ranges>
#include <boost/geometry.hpp>
#include <boost/geometry/index/rtree.hpp>
#include <boost/geometry/geometries/box.hpp>
#include <boost/geometry/geometries/register/point.hpp>

/*------------------------------------------------------------------------------------------------*/

namespace ici {

    class circle_set {

        struct discretized_circle {
            int64_t x;
            int64_t y;
            int64_t r;

            bool operator==(const discretized_circle& c) const;
        };

        struct hash_discretized_circle {
            size_t operator()(const discretized_circle& c) const;
        };

        discretized_circle discretize(const circle& c) const;

        std::unordered_map<discretized_circle, ici::circle, hash_discretized_circle> impl_;
        double eps_;

    public:
        circle_set(double eps);
        circle_set(double eps, std::ranges::forward_range auto circles) : 
                circle_set(eps) {
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