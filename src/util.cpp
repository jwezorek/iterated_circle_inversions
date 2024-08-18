#include "util.h"
#include <fstream>
#include <stdexcept>
#include <ranges>
#include <sstream>
#include <format>

namespace r = std::ranges;
namespace rv = std::ranges::views;


/*------------------------------------------------------------------------------------------------*/
namespace {

    std::vector<std::string> split(std::string const& input) {
        std::istringstream buffer(input);
        std::vector<std::string> ret;

        std::copy(std::istream_iterator<std::string>(buffer),
            std::istream_iterator<std::string>(),
            std::back_inserter(ret));
        return ret;
    }

    ici::circle to_circle(const std::string& str) {
        auto pieces = split(str);
        if (pieces.size() != 3) {
            throw std::runtime_error("invalid input");
        }
        auto values = pieces | rv::transform(
            [](auto&& p) {
                return std::stod(p);
            }
        ) | r::to<std::vector>();
        return {
            {values[0],values[1]},
            values[2]
        };
    }

    ici::rectangle bounds(const std::vector<ici::circle>& circles) {
        auto circle_bounds = circles | rv::transform(ici::bounds) | r::to<std::vector>();

        auto x1 = r::min(circle_bounds | rv::transform([](auto&& r) { return r.min.x; }));
        auto y1 = r::min(circle_bounds | rv::transform([](auto&& r) { return r.min.y; }));
        auto x2 = r::max(circle_bounds | rv::transform([](auto&& r) { return r.max.x; }));
        auto y2 = r::max(circle_bounds | rv::transform([](auto&& r) { return r.max.y; }));

        return { {x1,y1},{x2,y2} };
    }

    void string_to_file(const std::string& fname, const std::string& contents) {
        std::ofstream file( fname );
        file << contents;
    }
}

std::vector<std::string> ici::file_to_string_vector(const std::string& filename)
{
    std::vector<std::string> v;

    std::ifstream fs(filename);
    if (!fs) {
        throw std::runtime_error("bad file");
    }

    std::string line;
    while (std::getline(fs, line)) {
        v.push_back(line);
    }
    return v;
}

void ici::to_svg(const std::string& fname, const std::vector<circle>& inp_circles, 
        double padding, double scale) {

    auto circles = inp_circles | rv::transform(
            [k = scale](auto&& c) {
                return ici::scale(c, k);
            }
        ) | r::to<std::vector>();
    auto dimensions = pad(::bounds(circles), padding);
    std::stringstream ss;

    ss << std::format(
        "<svg style=\"{}\" viewBox=\"{} {} {} {}\" xmlns=\"http://www.w3.org/2000/svg\">\n",
        "stroke-width: 1px; background-color: white;",
        dimensions.min.x,
        dimensions.min.y,
        dimensions.max.x - dimensions.min.x,
        dimensions.max.y - dimensions.min.y
    );

    for (const auto& c : circles) {
        ss << std::format(
            "    <circle cx=\"{}\" cy=\"{}\" r=\"{}\" fill=\"none\" stroke=\"black\" />\n",
            c.loc.x, c.loc.y, c.radius
        );
    }

    ss << "</svg>";

    string_to_file(fname, ss.str());
}

std::vector<ici::circle> ici::parse(const std::vector<std::string>& inp)
{
    return inp | rv::transform(
        to_circle
    ) | r::to<std::vector>();
}

