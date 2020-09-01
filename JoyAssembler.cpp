/* Jonathan Frech, August 2020 */
/* A minimalistic toy assembler written in C++17. */

#include "Types.hh"
#include "Log.cpp"

#include "Util.cpp"
#include "Computation.cpp"
#include "Parse.cpp"

int main(int const argc, char const*argv[]) {
    if (argc < 2) {
        std::cerr << "please provide an input joy assembly file\n";
        return EXIT_FAILURE; }

    ComputationState cs{0x10000};

    if (argc > 2 && std::string{argv[2]} == "visualize")
        cs.enableVisualization();
    if (argc > 2 && std::string{argv[2]} == "step")
        cs.enableStepping();
    if (argc > 2 && std::string{argv[2]} == "cycles")
        cs.enableFinalCycles();

    ParsingState ps{
        std::filesystem::current_path() / std::filesystem::path(argv[1])};
    if (!parse1(ps))
        return ps.error(0, "parsing failed at stage 1"), EXIT_FAILURE;
    if (!parse2(ps, cs))
        return ps.error(0, "parsing failed at stage 2"), EXIT_FAILURE;

    if (argc > 2 && std::string{argv[2]} == "memory-dump") {
        do cs.memoryDump(); while (cs.step()); cs.memoryDump();
        return EXIT_SUCCESS; }

    do cs.visualize(); while (cs.step()); cs.finalCycles();

    return EXIT_SUCCESS;
}
