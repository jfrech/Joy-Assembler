/* Jonathan Frech, August and October 2020 */
/* A C++17 UTF-8 decoding and encoding implementation. */

#ifndef UTF8_CPP
#define UTF8_CPP

#include <iostream>
#include <optional>
#include <tuple>
#include <vector>

namespace UTF8 {
    typedef uint32_t rune_t;
    typedef uint8_t byte_t;

    constexpr rune_t const NULL_RUNE = static_cast<rune_t>(0x00000000);
    constexpr rune_t const ERROR_RUNE = static_cast<rune_t>(0x0000fffd);

    class Encoder {
        private:
            std::vector<byte_t> bytes;
            bool ok;

        public: Encoder() :
            bytes{},
            ok{true}
        { ; }

        public: bool encode(rune_t const rune) {
            if (rune <= 0x7f) {
                bytes.push_back(static_cast<byte_t>(
                    0b0'0000000 | ( rune        & 0b0'1111111)));
                return true; }

            else if (rune <= 0x07ff) {
                bytes.push_back(static_cast<byte_t>(
                    0b110'00000 | ((rune >>  6) & 0b000'11111)));
                bytes.push_back(static_cast<byte_t>(
                    0b10'000000 | ( rune        & 0b00'111111)));
                return true; }

            else if (rune <= 0xffff) {
                bytes.push_back(static_cast<byte_t>(
                    0b1110'0000 | ((rune >> 12) & 0b0000'1111)));
                bytes.push_back(static_cast<byte_t>(
                    0b10'000000 | ((rune >>  6) & 0b00'111111)));
                bytes.push_back(static_cast<byte_t>(
                    0b10'000000 | ( rune        & 0b00'111111)));
                return true; }

            else if (rune <= 0x10ffff) {
                bytes.push_back(static_cast<byte_t>(
                    0b11110'000 | ((rune >> 18) & 0b00000'111)));
                bytes.push_back(static_cast<byte_t>(
                    0b10'000000 | ((rune >> 12) & 0b00'111111)));
                bytes.push_back(static_cast<byte_t>(
                    0b10'000000 | ((rune >>  6) & 0b00'111111)));
                bytes.push_back(static_cast<byte_t>(
                    0b10'000000 | ( rune        & 0b00'111111)));
                return true; }

            else
                return err();
        }

        public: std::tuple<std::vector<byte_t>, bool> finish() {
            std::tuple<std::vector<byte_t>, bool> bytesOk{std::make_tuple(
                bytes, ok)};
            bytes.clear();
            ok = true;
            return bytesOk; }

        private: bool err() {
            return ok = false; }
    };

    class Decoder {
        private:
            std::vector<rune_t> runes;
            std::vector<byte_t> buf;
            bool ok;

        public: Decoder() :
            runes{},
            buf{},
            ok{true}
        {
            buf.reserve(4); }

        /* return value signals if another byte is required */
        public: bool decode(byte_t const b) {
            bool invalid = false;
            for (std::size_t j = 1; j < 4; ++j) {
                if (j >= buf.size())
                    break;
                invalid |= ((0b11'000000 & buf[j]) != 0b10'000000); }
            if (invalid)
                return err();

            auto const checkRune{[&](
                std::size_t const min, std::size_t const max
            ) {
                return !runes.empty()
                    && min <= runes.back() && runes.back() <= max;
            }};

            buf.push_back(b);

            if ((buf[0] & 0b1'0000000) == 0b0'0000000) {
                if (buf.size() < 1)
                    return true;
                if (buf.size() != 1)
                    return err();
                runes.push_back(static_cast<rune_t>(
                       0b0'1111111 & buf[0]));
                buf.clear();
                if (!checkRune(0x00, 0x7f))
                    return err();
                return false; }

            else if ((buf[0] & 0b111'00000) == 0b110'00000) {
                if (buf.size() < 2)
                    return true;
                if (buf.size() != 2 || invalid)
                    return err();
                runes.push_back(static_cast<rune_t>(
                      (0b000'11111 & buf[0]) <<  6
                    | (0b00'111111 & buf[1])));
                buf.clear();
                if (!checkRune(0x80, 0x07ff))
                    return err();
                return false; }

            else if ((buf[0] & 0b1111'0000) == 0b1110'0000) {
                if (buf.size() < 3)
                    return true;
                if (buf.size() != 3 || invalid)
                    return err();
                runes.push_back(static_cast<rune_t>(
                      (0b0000'1111 & buf[0]) << 12
                    | (0b00'111111 & buf[1]) <<  6
                    | (0b00'111111 & buf[2])));
                buf.clear();
                if (!checkRune(0x0800, 0xffff))
                    return err();
                return false; }

            else if ((buf[0] & 0b11111'000) == 0b11110'000) {
                if (buf.size() < 4)
                    return true;
                if (buf.size() != 4 || invalid)
                    return err();
                runes.push_back(static_cast<rune_t>(
                      (0b00000'111 & buf[0]) << 18
                    | (0b00'111111 & buf[1]) << 12
                    | (0b00'111111 & buf[2]) <<  6
                    | (0b00'111111 & buf[3])));
                buf.clear();
                if (!checkRune(0x10000, 0x10ffff))
                    return err();
                return false; }

            else
                return err();
        }

        public: std::tuple<std::vector<rune_t>, bool> finish() {
            if (!buf.empty())
                err();

            std::tuple<std::vector<rune_t>, bool> runesOk{std::make_tuple(
                runes, ok)};
            runes.clear();
            buf.clear();
            ok = true;
            return runesOk; }

        private: bool err() {
            runes.push_back(ERROR_RUNE);
            buf.clear();
            return ok = false; }
    };

    std::optional<std::string> utf8string(std::vector<rune_t> const&runes) {
        Encoder encoder{};
        for (rune_t const&rune : runes)
            encoder.encode(rune);
        auto const&[bytes, ok]{encoder.finish()};
        if (!ok)
            return std::nullopt;

        std::string str{};
        for (byte_t const&byte : bytes)
            str += static_cast<std::string::value_type>(byte);
        return std::make_optional(str);
    }
}

namespace UTF8IO {
    void putByte(UTF8::byte_t const b) {
        std::cout.put(b); }

    UTF8::byte_t getByte() {
        return std::cin.get(); }

    void putRune(UTF8::rune_t const rune) {
        UTF8::Encoder encoder{};
        encoder.encode(rune);
        auto [bytes, ok] = encoder.finish();
        if (!ok)
            return;
        for (UTF8::byte_t b : bytes)
            putByte(b); }

    UTF8::rune_t getRune() {
        UTF8::Decoder decoder{};
        while (decoder.decode(getByte()))
            ;
        auto [runes, ok] = decoder.finish();
        if (!ok || runes.size() != 1)
            return UTF8::ERROR_RUNE;
        return runes.front(); }
}

#endif
