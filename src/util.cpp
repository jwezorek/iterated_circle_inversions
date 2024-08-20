#include "util.h"
#include <fstream>
#include <stdexcept>
#include <ranges>
#include <sstream>
#include <format>

namespace r = std::ranges;
namespace rv = std::ranges::views;


/*------------------------------------------------------------------------------------------------*/

void ici::string_to_file(const std::string& fname, const std::string& contents) {
    std::ofstream file(fname);
    file << contents;
}

