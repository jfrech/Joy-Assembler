#ifndef JOY_ASSEMBLER__UTIL_CPP
#define JOY_ASSEMBLER__UTIL_CPP

#include "types.hh"

namespace Util {
    std::string to_upper(std::string const&);
    std::string UInt32AsPaddedHex(uint32_t const n);

    namespace std20 {
        /* a custom std::map::contains (C++20) implementation */
        template<typename K, typename V>
        inline bool contains(std::map<K, V> const&map, K const&key) {
            return map.count(key) != 0; }
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

    rune_t ERROR_RUNE = static_cast<rune_t>(0xfffd);
    class UTF8Encoder {
        public: bool encode(rune_t const rune) {
            if (rune <= 0x7f) {
                bytes.push_back(static_cast<byte_t>(
                    0b0'0000000 | ( rune        & 0b0'1111111)));
                return true; }
            if (rune <= 0x07ff) {
                bytes.push_back(static_cast<byte_t>(
                    0b110'00000 | ((rune >>  6) & 0b000'11111)));
                bytes.push_back(static_cast<byte_t>(
                    0b10'000000 | ( rune        & 0b00'111111)));
                return true; }
            if (rune <= 0xffff) {
                bytes.push_back(static_cast<byte_t>(
                    0b1110'0000 | ((rune >> 12) & 0b0000'1111)));
                bytes.push_back(static_cast<byte_t>(
                    0b10'000000 | ((rune >>  6) & 0b00'111111)));
                bytes.push_back(static_cast<byte_t>(
                    0b10'000000 | ( rune        & 0b00'111111)));
                return true; }
            if (rune <= 0x10ffff) {
                bytes.push_back(static_cast<byte_t>(
                    0b11110'000 | ((rune >> 18) & 0b00000'111)));
                bytes.push_back(static_cast<byte_t>(
                    0b10'000000 | ((rune >> 12) & 0b00'111111)));
                bytes.push_back(static_cast<byte_t>(
                    0b10'000000 | ((rune >>  6) & 0b00'111111)));
                bytes.push_back(static_cast<byte_t>(
                    0b10'000000 | ( rune        & 0b00'111111)));
                return true; }
            return erroneous = false;
        }

        public: bool finish() {
            return !erroneous; }

        public:
            std::vector<byte_t> bytes{};
        private:
            bool erroneous{false};
    };
    class UTF8Decoder {
        private: bool err() {
            erroneous = true;
            runes.push_back(ERROR_RUNE);
            buf.clear();
            return false; }

        public: bool decode(byte_t const b) {
            /* return value signals if another byte is required */

            auto invalid = [](byte_t b){
                return (0b11'000000 & b) != 0b10'000000; };

            buf.push_back(b);

            if ((buf[0] & 0b1'0000000) == 0b0'0000000) {
                if (buf.size() < 1)
                    return true;
                if (buf.size() != 1)
                    return err();

                runes.push_back(static_cast<rune_t>(
                       0b0'1111111 & buf[0]));
                buf.clear();
                return false;
            }

            if ((buf[0] & 0b111'00000) == 0b110'00000) {
                if (buf.size() < 2)
                    return true;
                if (buf.size() != 2 || invalid(buf[1]))
                    return err();

                runes.push_back(static_cast<rune_t>(
                      (0b000'11111 & buf[0]) <<  6
                    | (0b00'111111 & buf[1])));
                buf.clear();
                return false;
            }

            if ((buf[0] & 0b1111'0000) == 0b1110'0000) {
                if (buf.size() < 3)
                    return true;
                if (buf.size() != 3 || invalid(buf[1]) || invalid(buf[2]))
                    return err();

                runes.push_back(static_cast<rune_t>(
                      (0b0000'1111 & buf[0]) << 12
                    | (0b00'111111 & buf[1]) <<  6
                    | (0b00'111111 & buf[2])));
                buf.clear();
                return false;
            }

            if ((buf[0] & 0b11111'000) == 0b11110'000) {
                if (buf.size() < 4)
                    return true;
                if (buf.size() != 4 || invalid(buf[1]) || invalid(buf[2]) || invalid(buf[3]))
                    return err();

                runes.push_back(static_cast<rune_t>(
                      (0b00000'111 & buf[0]) << 18
                    | (0b00'111111 & buf[1]) << 12
                    | (0b00'111111 & buf[2]) <<  6
                    | (0b00'111111 & buf[3])));
                buf.clear();
                return false;
            }

            return err();
        }

        public: bool finish() {
            if (buf.empty())
                return !erroneous;
            err();
            return false; }

        public:
            std::vector<rune_t> runes{};
        private:
            std::vector<byte_t> buf{};
            bool erroneous{false};
    };

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

    std::optional<std::vector<rune_t>> parseString(std::string const&s) {
        UTF8Decoder decoder{};
        for (byte_t b : s)
            decoder.decode(b);
        if (!decoder.finish())
            return std::nullopt;

        std::map<rune_t, rune_t> oneRuneEscapes = {
            {static_cast<rune_t>('0'), static_cast<rune_t>('\0')},
            {static_cast<rune_t>('a'), static_cast<rune_t>('\a')},
            {static_cast<rune_t>('b'), static_cast<rune_t>('\b')},
            {static_cast<rune_t>('e'), static_cast<rune_t>(0x0b)},
            {static_cast<rune_t>('f'), static_cast<rune_t>('\f')},
            {static_cast<rune_t>('n'), static_cast<rune_t>('\n')},
            {static_cast<rune_t>('r'), static_cast<rune_t>('\r')},
            {static_cast<rune_t>('t'), static_cast<rune_t>('\t')},
            {static_cast<rune_t>('v'), static_cast<rune_t>('\v')},
            {static_cast<rune_t>('"'), static_cast<rune_t>('"')}
        };
        std::map<rune_t, uint8_t> nibbleEscapes = {
            {static_cast<rune_t>('0'), 0x0},
            {static_cast<rune_t>('1'), 0x1},
            {static_cast<rune_t>('2'), 0x2},
            {static_cast<rune_t>('3'), 0x3},
            {static_cast<rune_t>('4'), 0x4},
            {static_cast<rune_t>('5'), 0x5},
            {static_cast<rune_t>('6'), 0x6},
            {static_cast<rune_t>('7'), 0x7},
            {static_cast<rune_t>('8'), 0x8},
            {static_cast<rune_t>('9'), 0x9},
            {static_cast<rune_t>('a'), 0xa},
            {static_cast<rune_t>('b'), 0xb},
            {static_cast<rune_t>('c'), 0xc},
            {static_cast<rune_t>('d'), 0xd},
            {static_cast<rune_t>('e'), 0xe},
            {static_cast<rune_t>('f'), 0xf},
            {static_cast<rune_t>('A'), 0xa},
            {static_cast<rune_t>('B'), 0xb},
            {static_cast<rune_t>('C'), 0xc},
            {static_cast<rune_t>('D'), 0xd},
            {static_cast<rune_t>('E'), 0xe},
            {static_cast<rune_t>('F'), 0xf},
        };

        std::vector<rune_t> unescaped{};
        Stream<rune_t> stream{decoder.runes, static_cast<rune_t>(0x00000000)};
        while (!stream.exhausted()) {
            rune_t rune{stream.read()};
            if (rune != static_cast<rune_t>('\\')) {
                unescaped.push_back(rune);
                continue; }
            if (stream.exhausted())
                return std::nullopt;

            rune_t emprisonedRune{stream.read()};
            if (std20::contains(oneRuneEscapes, emprisonedRune)) {
                unescaped.push_back(oneRuneEscapes[emprisonedRune]);
                continue; }
            if (emprisonedRune == static_cast<rune_t>('u') || emprisonedRune == static_cast<rune_t>('U')) {
                uint8_t escapeLength = emprisonedRune == static_cast<rune_t>('u') ? 4 : 8;
                rune_t escapedRune = static_cast<rune_t>(0x00000000);
                for (uint8_t j = 0; j < escapeLength; j++) {
                    if (stream.exhausted())
                        return std::nullopt;
                    rune_t emprisonedNibble{stream.read()};
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
        if (unescaped.front() != static_cast<rune_t>('"'))
            return std::nullopt;
        unescaped.erase(unescaped.begin());
        if (unescaped.back() != static_cast<rune_t>('"'))
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

    void put_utf8_char(rune_t const rune) {
        UTF8Encoder encoder{};
        encoder.encode(rune);
        if (!encoder.finish())
            return;
        for (byte_t b : encoder.bytes)
            put_byte(b); }

    rune_t get_utf8_char() {
        UTF8Decoder decoder{};
        while (decoder.decode(get_byte()))
            ;
        if (!decoder.finish())
            return Util::ERROR_RUNE;
        if (decoder.runes.size() != 1)
            return Util::ERROR_RUNE;
        return decoder.runes[0]; }
}

#endif
