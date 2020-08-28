#ifndef JOY_ASSEMBLER__UTIL_CPP
#define JOY_ASSEMBLER__UTIL_CPP

#include "UTF8.cpp"
#include "types.hh"

namespace Util {
    std::string to_upper(std::string const&);
    std::string UInt32AsPaddedHex(uint32_t const n);

    namespace std20 {
        /* a custom std::map::contains (C++20) implementation */
        template<typename K, typename V>
        inline bool contains(std::map<K, V> const&map, K const&key) {
            return map.count(key) != 0; }

        /* a custom std::set::contains (C++20) implementation */
        template<typename V>
        inline bool contains(std::set<V> const&set, V const&value) {
            return set.count(value) != 0; }
    }
}

namespace Util {
    namespace ANSI_COLORS {
        const std::string CLEAR = "\33[0m";

        const std::string INSTRUCTION_NAME = "\33[38;5;119m";
        const std::string INSTRUCTION_ARGUMENT = "\33[38;5;121m";
        const std::string STACK = "\33[38;5;127m";
        const std::string STACK_FAINT = "\33[38;5;53m";
        const std::string MEMORY_LOCATION_USED = "\33[1m";

        std::string paint(std::string const&ansi, std::string const&text) {
            return ansi + text + CLEAR; }
    }


    template<typename T>
    class Stream {
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

        private:
            std::size_t p;
            std::vector<T> values;
            T zeroValue;
    };

    std::optional<std::vector<UTF8::rune_t>> parseString(std::string const&s) {
        UTF8::Decoder decoder{};
        for (byte_t b : s)
            decoder.decode(b);
        if (!decoder.finish())
            return std::nullopt;

        std::map<UTF8::rune_t, UTF8::rune_t> oneRuneEscapes = {
            {static_cast<UTF8::rune_t>('0'), static_cast<UTF8::rune_t>('\0')},
            {static_cast<UTF8::rune_t>('a'), static_cast<UTF8::rune_t>('\a')},
            {static_cast<UTF8::rune_t>('b'), static_cast<UTF8::rune_t>('\b')},
            {static_cast<UTF8::rune_t>('e'), static_cast<UTF8::rune_t>(0x0b)},
            {static_cast<UTF8::rune_t>('f'), static_cast<UTF8::rune_t>('\f')},
            {static_cast<UTF8::rune_t>('n'), static_cast<UTF8::rune_t>('\n')},
            {static_cast<UTF8::rune_t>('r'), static_cast<UTF8::rune_t>('\r')},
            {static_cast<UTF8::rune_t>('t'), static_cast<UTF8::rune_t>('\t')},
            {static_cast<UTF8::rune_t>('v'), static_cast<UTF8::rune_t>('\v')},
            {static_cast<UTF8::rune_t>('"'), static_cast<UTF8::rune_t>('"')}
        };
        std::map<UTF8::rune_t, uint8_t> nibbleEscapes = {
            {static_cast<UTF8::rune_t>('0'), 0x0},
            {static_cast<UTF8::rune_t>('1'), 0x1},
            {static_cast<UTF8::rune_t>('2'), 0x2},
            {static_cast<UTF8::rune_t>('3'), 0x3},
            {static_cast<UTF8::rune_t>('4'), 0x4},
            {static_cast<UTF8::rune_t>('5'), 0x5},
            {static_cast<UTF8::rune_t>('6'), 0x6},
            {static_cast<UTF8::rune_t>('7'), 0x7},
            {static_cast<UTF8::rune_t>('8'), 0x8},
            {static_cast<UTF8::rune_t>('9'), 0x9},
            {static_cast<UTF8::rune_t>('a'), 0xa},
            {static_cast<UTF8::rune_t>('b'), 0xb},
            {static_cast<UTF8::rune_t>('c'), 0xc},
            {static_cast<UTF8::rune_t>('d'), 0xd},
            {static_cast<UTF8::rune_t>('e'), 0xe},
            {static_cast<UTF8::rune_t>('f'), 0xf},
            {static_cast<UTF8::rune_t>('A'), 0xa},
            {static_cast<UTF8::rune_t>('B'), 0xb},
            {static_cast<UTF8::rune_t>('C'), 0xc},
            {static_cast<UTF8::rune_t>('D'), 0xd},
            {static_cast<UTF8::rune_t>('E'), 0xe},
            {static_cast<UTF8::rune_t>('F'), 0xf},
        };

        std::vector<UTF8::rune_t> unescaped{};
        Stream<UTF8::rune_t> stream{decoder.runes, static_cast<UTF8::rune_t>(0x00000000)};
        while (!stream.exhausted()) {
            UTF8::rune_t rune{stream.read()};
            if (rune != static_cast<UTF8::rune_t>('\\')) {
                unescaped.push_back(rune);
                continue; }
            if (stream.exhausted())
                return std::nullopt;

            UTF8::rune_t emprisonedRune{stream.read()};
            if (std20::contains(oneRuneEscapes, emprisonedRune)) {
                unescaped.push_back(oneRuneEscapes[emprisonedRune]);
                continue; }
            if (emprisonedRune == static_cast<UTF8::rune_t>('u') || emprisonedRune == static_cast<UTF8::rune_t>('U')) {
                uint8_t escapeLength = emprisonedRune == static_cast<UTF8::rune_t>('u') ? 4 : 8;
                UTF8::rune_t escapedRune = static_cast<UTF8::rune_t>(0x00000000);
                for (uint8_t j = 0; j < escapeLength; j++) {
                    if (stream.exhausted())
                        return std::nullopt;
                    UTF8::rune_t emprisonedNibble{stream.read()};
                    if (!std20::contains(nibbleEscapes, emprisonedNibble))
                        return std::nullopt;
                    escapedRune <<= 4;
                    escapedRune |= 0xf & nibbleEscapes[emprisonedNibble];
                }
                unescaped.push_back(escapedRune);
                continue; }

            return std::nullopt;
        }

        if (unescaped.size() <= 2)
            return std::nullopt;
        if (unescaped.front() != static_cast<UTF8::rune_t>('"'))
            return std::nullopt;
        unescaped.erase(unescaped.begin());
        if (unescaped.back() != static_cast<UTF8::rune_t>('"'))
            return std::nullopt;
        unescaped.pop_back();

        return std::make_optional(unescaped);
    }

    std::optional<word_t> stringToUInt32(std::string const&s) {
        try {
            /* TODO dynamic_cast */
            if (std::regex_match(s, std::regex{"\\s*[+-]?0[xX][0-9a-fA-F]+\\s*"}))
                return std::optional{static_cast<word_t>(std::stoll(s, nullptr, 16))};
            if (std::regex_match(s, std::regex{"\\s*[+-]?0[bB][01]+\\s*"})) {
                std::string s2{std::regex_replace(s, std::regex{"0[bB]"}, "")};
                return std::optional{static_cast<word_t>(std::stoll(s2, nullptr, 2))}; }
            if (std::regex_match(s, std::regex{"\\s*[+-]?[0-9]+\\s*"}))
                return std::optional{static_cast<word_t>(std::stoll(s, nullptr, 10))};
            return std::nullopt;
        } catch (std::invalid_argument const&_) {
            return std::nullopt; }}

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

    void put_byte(byte_t const b) {
        std::cout.put(b); }

    byte_t get_byte() {
        return std::cin.get(); }

    void put_utf8_char(UTF8::rune_t const rune) {
        UTF8::Encoder encoder{};
        encoder.encode(rune);
        if (!encoder.finish())
            return;
        for (byte_t b : encoder.bytes)
            put_byte(b); }

    UTF8::rune_t get_utf8_char() {
        UTF8::Decoder decoder{};
        while (decoder.decode(get_byte()))
            ;
        if (!decoder.finish())
            return UTF8::ERROR_RUNE;
        if (decoder.runes.size() != 1)
            return UTF8::ERROR_RUNE;
        return decoder.runes[0]; }
}

#endif
