#include <print>
#include <optional>
#include <string>
#include <ranges>
#include <numbers>
#include <filesystem>
#include "geometry.h"
#include "iterated_inversion.h"
#include "input.h"
#include "util.h"
#include <expected>
#include <stdexcept>

namespace fs = std::filesystem;
namespace r = std::ranges;
namespace rv = std::ranges::views;

/*------------------------------------------------------------------------------------------------*/

namespace {

	std::expected<const ici::input, std::runtime_error> parse_cmd_line(int argc, char* argv[]) {
		if (argc != 2) {
			return std::unexpected(
				std::runtime_error("iterated circle inversions requires a .json file.")
			);
		}
		return ici::parse_input(argv[1]);
	}
}

int main(int argc, char* argv[]) {

	try {
		auto input = parse_cmd_line(argc, argv);
		if (!input.has_value()) {
			throw input.error();
		}

		auto circles = ici::perform_inversions( *input );

		if (std::holds_alternative<ici::vector_settings>(input->output_settings)) {
			ici::to_svg(
				input->out_file, 
				circles, 
				std::get<ici::vector_settings>(input->output_settings)
			);
		} else {
			ici::to_raster(input->out_file, circles, 
				std::get<ici::raster_settings>(input->output_settings)
			);
		}

		return 0;

	} catch (std::runtime_error e) {
		std::println("error : {}", e.what());
	} catch (...) {
		std::println("error : {}", "unkown");
	}

	return -1;
}
