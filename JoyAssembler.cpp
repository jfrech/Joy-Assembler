/* Jonathan Frech, August 2020 */
/* A minimalistic toy assembler written in C++17. */

#include "Types.hh"
#include "Log.cpp"

#include "Util.cpp"
#include "Computation.cpp"
#include "Parser.cpp"

int main(int const argc, char const*argv[]) {
    if (argc < 2) {
        std::cerr << "please provide an input joy assembly file" << std::endl;
        return EXIT_FAILURE; }

    /*ComputationState cs{0x10000};

    if (argc > 2 && std::string{argv[2]} == "visualize")
        cs.enableVisualization();
    if (argc > 2 && std::string{argv[2]} == "step")
        cs.enableStepping();
    if (argc > 2 && std::string{argv[2]} == "cycles")
        cs.enableFinalCycles();
    */

    Parser parser{
        std::filesystem::current_path() / std::filesystem::path(argv[1])};

    if (argc > 2 && std::string{argv[2]} != "memory-dump")
        if (!parser.commandlineArg(std::string{argv[2]})) {
            std::cerr << "unknown commandline argument" << std::endl;
            return EXIT_FAILURE; }

    parser.parse();
    auto [cs, ok] = parser.finish();
    if (!ok) {
        std::cerr << "parsing failed" << std::endl;
        return EXIT_FAILURE; }

    if (argc > 2 && std::string{argv[2]} == "memory-dump") {
        do cs.memoryDump(); while (cs.step()); cs.memoryDump();
        return EXIT_SUCCESS; }

    do cs.visualize(); while (cs.step()); cs.finalCycles();

    return EXIT_SUCCESS;
}
