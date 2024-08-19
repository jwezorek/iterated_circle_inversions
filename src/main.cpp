#include <print>
#include <optional>
#include <string>
#include <ranges>
#include <numbers>
#include <filesystem>
#include "iter_circ_inv.h"
#include "input.h"
#include "util.h"
#include "rasterize.h"

namespace fs = std::filesystem;
namespace r = std::ranges;
namespace rv = std::ranges::views;

/*------------------------------------------------------------------------------------------------*/

namespace {

	std::optional<const ici::input> parse_cmd_line(int argc, char* argv[]) {
		if (argc != 2) {
			return {};
		}
		return ici::parse_input(argv[1]);
	}
}

int main(int argc, char* argv[]) {

	try {
		auto input = parse_cmd_line(argc, argv);
		if (!input) {
			std::println( 
				"iterated circle inversions requires a .json file."
			);
			return -1;
		}
		auto circles = ici::perform_inversions( *input );
		if (!input->rasterize) {
			to_svg(input->out_file, circles, 10, 100);
		} else {
			ici::rasterize( circles, *input );
		}

		return 0;

	} catch (std::runtime_error e) {
		std::println("error : {}", e.what());
	} catch (...) {
		std::println("error : {}", "unkown");
	}
	return -1;
}
