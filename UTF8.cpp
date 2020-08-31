/* Jonathan Frech, August 2020 */
/* A C++ UTF-8 decoding and encoding implementation. */

#ifndef UTF8_CPP
#define UTF8_CPP

#include <iostream>
#include <vector>

namespace UTF8 {
    typedef uint32_t rune_t;
    typedef uint8_t byte_t;

    rune_t constexpr NULL_RUNE = static_cast<rune_t>(0x00000000);
    rune_t constexpr ERROR_RUNE = static_cast<rune_t>(0x0000fffd);

    class Encoder {
        public:
            std::vector<byte_t> bytes;
        private:
            bool erroneous;

        public: Encoder() :
            bytes{},
            erroneous{false}
        { ; }

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

        public: bool finish() const {
            return !erroneous; }
    };

    class Decoder {
        public:
            std::vector<rune_t> runes;
        private:
            std::vector<byte_t> buf;
            bool erroneous;

        public: Decoder() :
            runes{},
            buf{},
            erroneous{false}
        {
            buf.reserve(4);
        }

        /* return value signals if another byte is required */
        public: bool decode(byte_t const b) {
            bool invalid = false;
            for (size_t j = 1; j < 4; j++) {
                if (j >= buf.size())
                    break;
                invalid |= ((0b11'000000 & buf[j]) != 0b10'000000); }
            if (invalid)
                return err();

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
                if (buf.size() != 2 || invalid)
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
                if (buf.size() != 3 || invalid)
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
                if (buf.size() != 4 || invalid)
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

        private: bool err() {
            erroneous = true;
            runes.push_back(ERROR_RUNE);
            buf.clear();
            return false; }
    };

    std::optional<std::string> runeVectorToOptionalString(
        std::vector<rune_t> const&runes
    ) {
        std::string s{""};
        for (rune_t rune : runes) {
            if (rune > 0x7f)
                return std::nullopt;
            char buf[2];
            buf[0] = static_cast<byte_t>(rune);
            buf[1] = '\0';
            s += std::string{buf}; }
        return std::make_optional(s);
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
        if (!encoder.finish())
            return;
        for (UTF8::byte_t b : encoder.bytes)
            putByte(b); }

    UTF8::rune_t getRune() {
        UTF8::Decoder decoder{};
        while (decoder.decode(getByte()))
            ;
        if (!decoder.finish())
            return UTF8::ERROR_RUNE;
        if (decoder.runes.size() != 1)
            return UTF8::ERROR_RUNE;
        return decoder.runes.front(); }
}

#endif
