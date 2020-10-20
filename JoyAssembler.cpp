/* Jonathan Frech, August to October 2020 */
/* A minimalistic toy assembler written in C++17. */

#include "Includes.hpp"

static_assert(sizeof (int64_t) == sizeof (long long int));
static_assert(sizeof (std::size_t) >= sizeof (uint32_t));
static_assert(sizeof (uint_t) >= sizeof (std::size_t));

int main(int const argc, char const*argv[]) {
    std::ios_base::sync_with_stdio(false);

    if (argc < 2) {
        std::cerr << "please provide an input joy assembly file" << std::endl;
        return EXIT_FAILURE;
    }

    Parser parser{};
    std::optional<ComputationState> oCS{parser.parse(
        std::filesystem::current_path() / std::string{argv[1]})};

    if (!oCS.has_value()) {
        std::cerr << "parsing failed" << std::endl;
        return EXIT_FAILURE;
    }
    ComputationState cs{std::move(oCS.value())};

    if (argc > 2 && std::string{argv[2]} != "memory-dump") {
        if (!parser.commandlineArg(cs, std::string{argv[2]})) {
            std::cerr << "unknown commandline argument" << std::endl;
            return EXIT_FAILURE;
        }
    }

    if (argc > 2 && std::string{argv[2]} == "memory-dump") {
        do cs.memoryDump(); while (cs.step()); cs.memoryDump();
        return EXIT_SUCCESS;
    }

    do cs.visualize(); while (cs.step());

    return EXIT_SUCCESS;
}
