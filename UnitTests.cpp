#include "Includes.hh"

std::function<void(bool const, std::string const&)> asserterFactory(bool &testStatus) {
    return [&testStatus](bool const microTest, std::string const&errorMessage) -> void {
        if (microTest)
            return;
        std::cerr << "MICROTEST FAILURE: " << errorMessage << std::endl;
    };
}

bool unitTest_LevenshteinDistance() {
    bool testStatus{true};
    auto asserter{asserterFactory(testStatus)};

    auto test = [](std::string s, std::string t, uint_t d) {
        return Util::LevenshteinDistance(s, t) == d
            && Util::LevenshteinDistance(t, s) == d;
    };

    std::vector<std::tuple<std::string, std::string, uint_t>> const cases{
        {"", "", 0},
        {"GUMBO", "GAMBOL", 2},
        {"a", "b", 1},
        /* TODO */
    };
    for (auto const&[s, t, d] : cases)
        asserter(test(s, t, d), "incorrect Levenshtein distance on inputs '"
            + s + "' and '" + t + "'.");

    return testStatus;
}

bool unitTest_Two_sComplement() {
    bool testStatus{true};
    auto asserter{asserterFactory(testStatus)};

    Util::rng_t rng{};
    for (uint_t j{0}; j < 0xfff; ++j) {
        uint32_t bits{rng.unif(0xff'ff'ff'ff)};
        asserter(Util::toTwo_sComplement<int32_t, uint32_t, 32>(
            Util::fromTwo_sComplement<uint32_t, int32_t, 32>(bits)) == bits,
            "incorrect 2's complement behaviour on the following bits: "
                + std::to_string(bits));

        int32_t small{static_cast<int32_t>(rng.unif(0xffffff))};
        uint32_t sign{rng.unif(1)};
        int32_t n{sign == 0 ? small : -small};
        asserter(Util::fromTwo_sComplement<uint32_t, int32_t, 32>(
            Util::toTwo_sComplement<int32_t, uint32_t, 32>(n)) == n,
            "incorrect 2's complement behaviour on the following integer: "
                + std::to_string(n));
    }

    return testStatus;
}

int main() {
    #define NameTheIdentifier(IDENTIFIER) \
        std::make_tuple(std::string{#IDENTIFIER}, IDENTIFIER)
    auto const&unitTests{std::vector{
        NameTheIdentifier(unitTest_LevenshteinDistance),
        NameTheIdentifier(unitTest_Two_sComplement),
    }};
    #undef NameTheIdentifier

    for (auto const&[unitTestName, unitTest] : unitTests)
        if (!unitTest()) {
            std::cerr << "\33[38;5;124m[ERR]\33[0m at least one unit test "
                "failed: " << unitTestName << std::endl;
            return EXIT_FAILURE; }
        else
            std::clog << "\33[38;5;154m[SUC]\33[0m unit test passed: "
                << unitTestName << std::endl;

    std::clog << "\33[38;5;154m[SUC]\33[0m every unit test succeeded ("
              << unitTests.size() << " in total)" << std::endl;

    return EXIT_SUCCESS;
}
