#pragma once

#include <vector>
#include <string>
#include <ranges>
#include <tuple>

/*------------------------------------------------------------------------------------------------*/

namespace ici {

    void string_to_file(const std::string& fname, const std::string& contents);

    template <std::ranges::random_access_range R>
    auto two_combinations(R&& rng) {
        namespace r = std::ranges;
        namespace rv = std::ranges::views;
        size_t n = r::size(rng);
        return rv::iota(0u, n - 1) |
            rv::transform(
                [n](auto i) {
                    return rv::iota(i + 1, n) | rv::transform(
                        [i](auto j) {
                            return std::tuple<size_t, size_t>(i, j);
                        }
                    );
                }
            ) | rv::join | rv::transform(
                [&rng](auto&& tup) {
                    auto [i, j] = tup;
                    return std::tuple(rng.at(i), rng.at(j));
                }
            );
    }

}