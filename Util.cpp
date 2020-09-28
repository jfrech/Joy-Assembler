#ifndef JOY_ASSEMBLER__UTIL_CPP
#define JOY_ASSEMBLER__UTIL_CPP

#include <chrono>
#include <random>
#include <thread>

#include <iostream>
#include <optional>
#include <map>
#include <set>

#include "Types.hh"
#include "UTF8.cpp"

namespace Util {
    /* custom C++20 implementations */
    namespace std20 {
        /* a custom std::map::contains (C++20) implementation */
        template<typename K, typename V>
        inline constexpr bool contains(std::map<K, V> const&map, K const&key) {
            return map.count(key) != 0; }

        /* a custom std::set::contains (C++20) implementation */
        template<typename V>
        inline constexpr bool contains(std::set<V> const&set, V const&value) {
            return set.count(value) != 0; }

        /* a custom std::identity (C++20) implementation */
        template<typename T>
        inline constexpr T identity(T const&x) {
            return x; }
    }

    namespace ANSI_COLORS {
        auto esc{[](std::string const&code) {
#ifdef NO_ANSI_COLORS
                return "";
#else
                return "\33[" + code + "m";
#endif
        }};

        std::string const CLEAR{esc("0")};
        std::string const NONE{""};

        std::string const INSTRUCTION_NAME{esc("38;5;119")};
        std::string const INSTRUCTION_ARGUMENT{esc("38;5;121")};
        std::string const STACK{esc("38;5;127")};
        std::string const STACK_FAINT{esc("38;5;53")};
        std::string const MEMORY_LOCATION_USED{esc("1")};
        std::string const FAINT{esc("2")};
        std::string const REGISTER{esc("38;5;198")};

        std::string const MEMORY_SEMANTICS__INSTRUCTION_HEAD{esc("38;5;34")};
        std::string const MEMORY_SEMANTICS__INSTRUCTION{esc("38;5;70")};
        std::string const MEMORY_SEMANTICS__DATA_HEAD{esc("38;5;56")};
        std::string const MEMORY_SEMANTICS__DATA{esc("38;5;92")};

        std::string paint(std::string const&ansi, std::string const&text) {
            return ansi + text + CLEAR; }

        auto paintFactory(std::string const&ansi) {
            return [&ansi](std::string const&text) {
                return paint(ansi, text); }; }

        std::string memorySemanticColor(MemorySemantic const sem) {
            switch (sem) {
                case MemorySemantic::InstructionHead:
                    return MEMORY_SEMANTICS__INSTRUCTION_HEAD;
                case MemorySemantic::Instruction:
                    return MEMORY_SEMANTICS__INSTRUCTION;
                case MemorySemantic::DataHead:
                    return MEMORY_SEMANTICS__DATA_HEAD;
                case MemorySemantic::Data:
                    return MEMORY_SEMANTICS__DATA;
            }

            return NONE;
        }
    }

    template <typename Int, typename UInt, unsigned int BitWidth>
    inline constexpr UInt toTwo_sComplement(UInt const n) {
        using _Int = typename std::make_signed<UInt>::type;
        static_assert(std::is_same<Int, _Int>::value);
        using _UInt = typename std::make_unsigned<Int>::type;
        static_assert(std::is_same<UInt, _UInt>::value);

        static_assert(sizeof (UInt) * 8 == BitWidth);
        static_assert(sizeof (Int) * 8 == BitWidth);

        return n >= 0 ? static_cast<UInt>(n)
            : ~static_cast<UInt>(-n) + 1;
    }
    template <typename UInt, typename Int, unsigned int BitWidth>
    inline constexpr Int fromTwo_sComplement(UInt const bits) {
        using _UInt = typename std::make_unsigned<Int>::type;
        static_assert(std::is_same<UInt, _UInt>::value);
        using _Int = typename std::make_signed<UInt>::type;
        static_assert(std::is_same<Int, _Int>::value);

        static_assert(sizeof (UInt) * 8 == BitWidth);
        static_assert(sizeof (Int) * 8 == BitWidth);

        return !(bits & (UInt{1} << (BitWidth-1))) ?  static_cast<int32_t>(bits)
            : -static_cast<int32_t>(~bits + 1);
    }

    namespace IO {
        void wait() {
            std::this_thread::sleep_for(std::chrono::milliseconds(100)); }
    }

    template<typename T>
    class Stream {
        private:
            std::size_t p;
            std::vector<T> values;
            T zeroValue;

        public: Stream(std::vector<T> const&values, T const&zeroValue) :
            p{0},
            values{values},
            zeroValue{zeroValue}
        { ; }

        public: T read() {
            if (exhausted())
                return zeroValue;
            return values.at(p++); }

        public: bool exhausted() const {
            return p >= values.size(); }
    };

    std::optional<std::vector<UTF8::rune_t>> parseString(std::string const&s) {
        std::map<UTF8::rune_t, UTF8::rune_t> const&oneRuneEscapes = {
            {static_cast<UTF8::rune_t>('0'),  static_cast<UTF8::rune_t>('\0')},
            {static_cast<UTF8::rune_t>('a'),  static_cast<UTF8::rune_t>('\a')},
            {static_cast<UTF8::rune_t>('b'),  static_cast<UTF8::rune_t>('\b')},
            {static_cast<UTF8::rune_t>('e'),  static_cast<UTF8::rune_t>(0x0b)},
            {static_cast<UTF8::rune_t>('f'),  static_cast<UTF8::rune_t>('\f')},
            {static_cast<UTF8::rune_t>('n'),  static_cast<UTF8::rune_t>('\n')},
            {static_cast<UTF8::rune_t>('r'),  static_cast<UTF8::rune_t>('\r')},
            {static_cast<UTF8::rune_t>('t'),  static_cast<UTF8::rune_t>('\t')},
            {static_cast<UTF8::rune_t>('v'),  static_cast<UTF8::rune_t>('\v')},
            /* any other character is its own escape; the most useful being:
            {static_cast<UTF8::rune_t>('\"'), static_cast<UTF8::rune_t>('\"')},
            {static_cast<UTF8::rune_t>('\''), static_cast<UTF8::rune_t>('\'')},
            {static_cast<UTF8::rune_t>(';'), static_cast<UTF8::rune_t>(';')}, */
        };
        std::map<UTF8::rune_t, uint8_t> const&nibbleEscapes{[]() {
            std::map<UTF8::rune_t, uint8_t> nibbleEscapes{};
            for (uint8_t j = 0; j < 10; ++j)
                nibbleEscapes.insert({static_cast<UTF8::rune_t>('0'+j), j});
            for (uint8_t j = 0; j < 6; ++j)
                nibbleEscapes.insert({static_cast<UTF8::rune_t>('a'+j), 10+j});
            for (uint8_t j = 0; j < 6; ++j)
                nibbleEscapes.insert({static_cast<UTF8::rune_t>('A'+j), 10+j});
            return nibbleEscapes;
        }()};

        UTF8::Decoder decoder{};
        for (byte_t b : s)
            decoder.decode(b);
        auto [runes, ok] = decoder.finish();
        if (!ok)
            return std::nullopt;
        Stream<UTF8::rune_t> stream{runes, UTF8::ERROR_RUNE};

        std::vector<UTF8::rune_t> unescaped{};
        while (!stream.exhausted()) {
            UTF8::rune_t rune{stream.read()};
            if (rune != static_cast<UTF8::rune_t>('\\')) {
                unescaped.push_back(rune);
                continue; }
            if (stream.exhausted())
                return std::nullopt;

            UTF8::rune_t const emprisonedRune{stream.read()};
            if (std20::contains(oneRuneEscapes, emprisonedRune)) {
                unescaped.push_back(oneRuneEscapes.at(emprisonedRune));
                continue; }

            UTF8::rune_t const shortU = static_cast<UTF8::rune_t>('u');
            UTF8::rune_t const longU = static_cast<UTF8::rune_t>('U');
            if (emprisonedRune == shortU || emprisonedRune == longU) {
                uint8_t const escapeLength = emprisonedRune == shortU ? 4 : 8;
                UTF8::rune_t escapedRune = UTF8::NULL_RUNE;
                for (uint8_t j = 0; j < escapeLength; ++j) {
                    if (stream.exhausted())
                        return std::nullopt;
                    UTF8::rune_t const emprisonedNibble{stream.read()};
                    if (!std20::contains(nibbleEscapes, emprisonedNibble))
                        return std::nullopt;
                    escapedRune <<= 4;
                    escapedRune |= 0xf & nibbleEscapes.at(emprisonedNibble); }
                unescaped.push_back(escapedRune);
                continue; }

            // e.g. `\1` is the escape code for `1`
            unescaped.push_back(emprisonedRune);

            return std::nullopt; }

        if (unescaped.size() < 1 || unescaped.front()
                                 != static_cast<UTF8::rune_t>('"'))
            return std::nullopt;
        unescaped.erase(unescaped.begin());
        if (unescaped.size() < 1 || unescaped.back()
                                 != static_cast<UTF8::rune_t>('"'))
            return std::nullopt;
        unescaped.erase(unescaped.end()-1);

        return std::make_optional(unescaped);
    }

    std::optional<word_t> stringToOptionalUInt32(std::string const&s) {
        static_assert(sizeof (int64_t) == sizeof (long long int));
        static_assert(sizeof (long long int) > sizeof (word_t));

        std::optional<long long int> oN{std::nullopt};

        std::vector<std::tuple<std::string,
        std::function<std::string(std::string const&)>, int>> const actions{
            {"^\\s*([+-]?0[xX][0-9a-fA-F]+)\\s*$",
                [](std::string const&str) { return str; }, 16},
            {"^\\s*([+-]?[0-9]+)\\s*$",
                [](std::string const&str) { return str; }, 10},
            {"^\\s*([+-]?0[bB][01]+)\\s*$", [](std::string const&str) {
                return std::string{std::regex_replace(
                    str, std::regex{"0[bB]"}, "")}; }, 2},
        };

        for (auto const&[regexString, lambda, base] : actions)
            if (!oN.has_value()) {
                std::smatch smatch{};
                if (std::regex_match(s, smatch, std::regex{regexString}))
                    try {
                        oN = std::make_optional(std::stoll(lambda(
                            std::string{smatch[1]}), nullptr, base)); }
                    catch (std::invalid_argument const&_) {
                        oN = std::nullopt; }
            }

        constexpr long long int const min32{-(1LL << 31)}, max32{(1LL << 32)-1};
        if (oN.has_value() && (oN.value() < min32 || oN.value() > max32))
            oN = std::nullopt;

        if (oN.has_value())
            return std::make_optional(static_cast<word_t>(0xffff'ffff &
                fromTwo_sComplement<uint64_t, int64_t, 64>(oN.value())));
        return std::nullopt;
    }

    std::string UInt32AsPaddedHex(uint32_t const n) {
        char buf[9];
        std::snprintf(buf, 9, "%08x", n);
        return std::string{buf}; }

    std::string UInt8AsPaddedHex(uint8_t const n) {
        char buf[3];
        std::snprintf(buf, 3, "%02x", n);
        return std::string{buf}; }

    std::string UNibbleAsPaddedHex(uint8_t const n) {
        char buf[2];
        std::snprintf(buf, 2, "%01x", n & 0xf);
        return std::string{buf}; }

    std::string UBitAsPaddedHex(uint8_t const n) {
        char buf[2];
        std::snprintf(buf, 2, "%01x", n & 0x1);
        return std::string{buf}; }

    std::string stringToUpper(std::string const&_str) {
        std::string str{_str};
        for (auto &c : str)
            c = std::toupper(c);
        return str; }

    uint_t LevenshteinDistance(std::string const&s, std::string const&t) {
        /* see "https://people.cs.pitt.edu/~kirk/cs1501/Pruhs/Fall2006/
            Assignments/editdistance/Levenshtein%20Distance.htm" */
        std::size_t n{s.size()}, m{t.size()};
        if (n <= 0 || m <= 0)
            return std::max(n, m);

        std::vector<std::vector<uint_t>> d(m+1,
            std::vector<uint_t>(n+1, uint_t{0}));
        for (std::size_t r{0}; r <= n; ++r)
            d[0][r] = r;
        for (std::size_t c{0}; c <= m; ++c)
            d[c][0] = c;

        for (std::size_t j{1}; j <= m; ++j)
            for (std::size_t i{1}; i <= n; ++i) {
                uint_t cost{s[i-1] == t[j-1] ? uint_t{0} : uint_t{1}};
                d[j][i] = std::min(std::min(
                    d[j][i-1] + 1,
                    d[j-1][i] + 1),
                    d[j-1][i-1] + cost
                ); }

        return d[m][n]; }

    std::vector<std::string> sortByLevenshteinDistanceTo(
        std::vector<std::string> const&v, std::string const&r
    ) {
        std::vector<std::string> w{v};
        std::sort(w.begin(), w.end(), [r](auto s, auto t) {
            return LevenshteinDistance(r, s) <= LevenshteinDistance(r, t); });
        return w; }

    class rng_t {
        private:
            std::mt19937 rng;
        public: rng_t() :
            rng{std::random_device{}()}
        { ; }

        public: void seed(word_t const seed) {
            rng.seed(seed); }

        public: word_t unif(word_t const n) {
            std::uniform_int_distribution<word_t> unif{0, n};
            return unif(rng); }

        std::vector<word_t> unif(word_t const size, word_t const n) {
            std::uniform_int_distribution<word_t> unif{0, n};
            std::vector<word_t> v{std::vector<word_t>(size)};
            std::generate(v.begin(), v.end(), [&]() { return unif(rng); });
            return v; }

        std::vector<word_t> perm(word_t const size) {
            std::vector<word_t> v{std::vector<word_t>(size)};
            std::iota(v.begin(), v.end(), 0);
            std::shuffle(v.begin(), v.end(), rng);
            return v; }
    };
}

#endif
