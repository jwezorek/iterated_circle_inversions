#include "circle_set.h"
#include <boost/functional/hash.hpp>
#include <ranges>

namespace r = std::ranges;
namespace rv = std::ranges::views;
namespace bg = boost::geometry;
namespace bgi = boost::geometry::index;

/*------------------------------------------------------------------------------------------------*/

bool ici::circle_set::discretized_circle::operator==(const discretized_circle& c) const
{
    return x == c.x && y == c.y && r == c.r;
}

size_t ici::circle_set::hash_discretized_circle::operator()(const discretized_circle& c) const
{
    size_t seed = 0;
    boost::hash_combine(seed, c.x);
    boost::hash_combine(seed, c.y);
    boost::hash_combine(seed, c.r);
    return seed;
}

ici::circle_set::discretized_circle ici::circle_set::discretize(
        const circle& c) const {

    auto k = 1.0 / eps_;

    return {
        static_cast<int64_t>(std::round(k * c.loc.x)),
        static_cast<int64_t>(std::round(k * c.loc.y)),
        static_cast<int64_t>(std::round(k * c.radius))
    };
}

ici::circle_set::circle_set(double eps) : eps_(eps) {
}

void ici::circle_set::insert(const circle& c) {
    auto key = discretize(c);
    if (impl_.contains(key)) {
        return;
    }
    impl_[key] = c;
}

std::vector<ici::circle> ici::circle_set::to_vector() const {
    return impl_ | rv::values | r::to<std::vector>();
}

double ici::circle_set::eps() const {
    return eps_;
}

bool ici::circle_set::empty() const {
    return impl_.empty();
}

size_t ici::circle_set::size() const {
    return impl_.size();
}
