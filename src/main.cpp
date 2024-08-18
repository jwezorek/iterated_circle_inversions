#include <print>
#include <optional>
#include <string>
#include <ranges>
#include "iter_circ_inv.h"
#include "util.h"

namespace r = std::ranges;
namespace rv = std::ranges::views;

/*------------------------------------------------------------------------------------------------*/

namespace {

	struct options {
		std::string input;
		std::string output;
		int iterations;
	};

	std::optional<options> parse_cmd_line(int argc, char* argv[]) {
		if (argc != 4) {
			return {};
		}
		return options{
			argv[1],
			argv[2],
			std::stoi(argv[3])
		};
	}
}

int main(int argc, char* argv[]) {

	try {

		auto options = parse_cmd_line(argc, argv);
		if (!options) {
			std::println("usage: circles [inp file] [out file] [num iterations]");
			return -1;
		}

		auto circles = ici::parse(
			ici::file_to_string_vector(options->input)
		);

		for (int i : rv::iota(0, options->iterations)) {
			circles = ici::do_one_round(circles);
		}

		to_svg(options->output, circles, 10, 100);

		return 0;

	} catch (std::runtime_error e) {
		std::println("error : {}", e.what());
	} catch (...) {
		std::println("error : {}", "unkown");
	}
	return -1;
}
