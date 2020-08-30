#ifndef JOY_ASSEMBLER__UTIL_CPP
#define JOY_ASSEMBLER__UTIL_CPP

#include "UTF8.cpp"
#include "types.hh"

#include <chrono>
#include <thread>

namespace Util {
    std::string to_upper(std::string const&);
    std::string UInt32AsPaddedHex(uint32_t const n);

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
        std::string const&CLEAR = "\33[0m";

        std::string const&INSTRUCTION_NAME = "\33[38;5;119m";
        std::string const&INSTRUCTION_ARGUMENT = "\33[38;5;121m";
        std::string const&STACK = "\33[38;5;127m";
        std::string const&STACK_FAINT = "\33[38;5;53m";
        std::string const&MEMORY_LOCATION_USED = "\33[1m";

        std::string paint(std::string const&ansi, std::string const&text) {
            return ansi + text + CLEAR; }
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

        public: bool exhausted() {
            return p >= values.size(); }
    };

    std::optional<std::vector<UTF8::rune_t>> parseString(std::string const&s) {
        UTF8::Decoder decoder{};
        for (byte_t b : s)
            decoder.decode(b);
        if (!decoder.finish())
            return std::nullopt;

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
            {static_cast<UTF8::rune_t>('\"'), static_cast<UTF8::rune_t>('\"')},
            {static_cast<UTF8::rune_t>('\''), static_cast<UTF8::rune_t>('\'')},
        };
        std::map<UTF8::rune_t, uint8_t> const&nibbleEscapes{[](){
            std::map<UTF8::rune_t, uint8_t> nibbleEscapes{};
            for (uint8_t j = 0; j < 10; j++)
                nibbleEscapes.insert({static_cast<UTF8::rune_t>('0'+j), j});
            for (uint8_t j = 0; j < 6; j++)
                nibbleEscapes.insert({static_cast<UTF8::rune_t>('a'+j), 10+j});
            for (uint8_t j = 0; j < 6; j++)
                nibbleEscapes.insert({static_cast<UTF8::rune_t>('A'+j), 10+j});
            return nibbleEscapes;
        }()};

        std::vector<UTF8::rune_t> unescaped{};
        Stream<UTF8::rune_t> stream{decoder.runes, UTF8::ERROR_RUNE};
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

            UTF8::rune_t constexpr shortU = static_cast<UTF8::rune_t>('u');
            UTF8::rune_t constexpr longU = static_cast<UTF8::rune_t>('U');
            if (emprisonedRune == shortU || emprisonedRune == longU) {
                uint8_t const escapeLength = emprisonedRune == shortU ? 4 : 8;
                UTF8::rune_t escapedRune = UTF8::NULL_RUNE;
                for (uint8_t j = 0; j < escapeLength; j++) {
                    if (stream.exhausted())
                        return std::nullopt;
                    UTF8::rune_t const emprisonedNibble{stream.read()};
                    if (!std20::contains(nibbleEscapes, emprisonedNibble))
                        return std::nullopt;
                    escapedRune <<= 4;
                    escapedRune |= 0xf & nibbleEscapes.at(emprisonedNibble); }
                unescaped.push_back(escapedRune);
                continue; }

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

    std::optional<word_t> stringToUInt32(std::string const&s) {
        std::optional<long long int> oN{std::nullopt};
        std::smatch smatch{};
        #define ON_MATCH(REGEX, LAMBDA, BASE) \
            if (!oN.has_value()) \
                if (std::regex_match(s, smatch, std::regex{(REGEX)})) \
                    try { oN = std::make_optional( \
                        std::stoll((LAMBDA)(smatch[1]), nullptr, (BASE))); } \
                    catch (std::invalid_argument const&_) { \
                        oN = std::nullopt; }
        ON_MATCH("^\\s*([+-]?0[xX][0-9a-fA-F]+)\\s*$", std20::identity, 16)
        ON_MATCH("^\\s*([+-]?[0-9]+)\\s*$", std20::identity, 10)
        ON_MATCH(std::regex{"^\\s*([+-]?0[bB][01]+)\\s*$"}, (
            [](std::string const&s) {
                return std::string{std::regex_replace(s,
                    std::regex{"0[bB]"}, "")}; }), 2)
        #undef ON_MATCH

        long long int constexpr min32 = -(1LL << 31);
        long long int constexpr max32 = +(1LL << 32)-1;
        if (oN.has_value() && ((oN.value() < min32 || oN.value() > max32)))
            oN = std::nullopt;

        return static_cast<std::optional<word_t>>(oN);
    }

    std::string UInt32AsPaddedHex(uint32_t const n) {
        char buf[11];
        std::snprintf(buf, 11, "0x%08x", n);
        return std::string{buf}; }

    std::string UInt8AsPaddedHex(uint8_t const n) {
        char buf[3];
        std::snprintf(buf, 3, "%02X", n);
        return std::string{buf}; }

    std::string to_upper(std::string const&_str) {
        std::string str{_str};
        for (auto &c : str)
            c = std::toupper(c);
        return str; }

    std::optional<std::filesystem::path>
    startingFilepathFilepathToOptionalResolvedFilepath(
        std::filesystem::path const&start, std::filesystem::path const&further
    ) {
        std::string startStr{start}, furtherStr{further};
        if (furtherStr == "")
            return std::nullopt;
        if (furtherStr.front() == '/')
            return std::make_optional(further);

        auto extendSplit = [](
            std::vector<std::string> &parts, std::string const&s
        ) {
            std::string buf{};
            for (auto const&c : s) {
                if (c == '/') {
                    if (!buf.empty())
                        parts.push_back(buf);
                    buf = std::string{};
                    continue; }
                buf += std::string{c}; }
            if (!buf.empty())
                parts.push_back(buf);
        };

        std::vector<std::string> parts{};
        extendSplit(parts, startStr);
        extendSplit(parts, furtherStr);

        for (std::size_t j = 0; j < parts.size(); j++) {
            if (parts[j] == "..") {
                if (j < 1)
                    return std::nullopt;
                parts.erase(parts.begin() + j--);
                parts.erase(parts.begin() + j--); }
            else if (parts[j] == ".")
                parts.erase(parts.begin() + j--);
        }
        std::filesystem::path path{"/"};
        for (std::string const&part : parts)
            path /= part;

        return path;
    }
}

#endif
