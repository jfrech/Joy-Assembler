#include <iostream>
#include <tuple>
#include <vector>

#include "Util.cpp"
#include "UTF8.cpp"

bool unitTest_LevenshteinDistance() {
    auto test = [](std::string s, std::string t, uint64_t d) {
        /*UTF8::Decoder decoder{};
        for (UTF8::byte_t b : s)
            decoder.decode(b);
        auto [runeS, okS] = decoder.finish();
        if (!okS)
            return false;
        for (UTF8::byte_t b : t)
            decoder.decode(b);
        auto [runeT, okT] = decoder.finish();
        if (!okT)
            return false;
        return Util::LevenshteinDistance(runeS, runeT) == d
            && Util::LevenshteinDistance(runeT, runeS) == d;*/
        return Util::LevenshteinDistance(s, t) == d
            && Util::LevenshteinDistance(t, s) == d;
    };

    std::vector<std::tuple<std::string, std::string, uint64_t>> const cases{
        {"", "", 0},
        {"GUMBO", "GAMBOL", 2},
        {"a", "b", 1},
        /* TODO */
    };
    for (auto const&[s, t, d] : cases)
        if (!test(s, t, d))
            return false;

    return true;
}


int main() {
    auto const&unitTests{std::vector{
        unitTest_LevenshteinDistance,
    }};

    for (auto const&unitTest : unitTests)
        if (!unitTest()) {
            std::cerr << "\33[38;5;124m[ERR]\33[0m "
                      << "at least one unit test failed" << std::endl;
            return EXIT_FAILURE; }
        else
            std::clog << "\33[38;5;154m[SUC]\33[0m "
                      << "unit test passed" << std::endl;

    std::clog << "\33[38;5;154m[SUC]\33[0m every unit test succeeded"
              << std::endl;
    return EXIT_SUCCESS;
}
