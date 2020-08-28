#ifndef JOY_ASSEMBLER__UTF8_CPP
#define JOY_ASSEMBLER__UTF8_CPP

#include <vector>

namespace UTF8 {
    typedef uint32_t rune_t;
    typedef uint8_t byte_t;

    rune_t ERROR_RUNE = static_cast<rune_t>(0xfffd);

    class Encoder {
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

    class Decoder {
        private: bool err() {
            erroneous = true;
            runes.push_back(ERROR_RUNE);
            buf.clear();
            return false; }

        /* return value signals if another byte is required */
        public: bool decode(byte_t const b) {
            bool invalid = false;
            for (size_t j = 1; j < 4; j++) {
                if (j >= buf.size())
                    break;
                invalid = invalid || ((0b11'000000 & buf[j]) != 0b10'000000); }
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

        public:
            std::vector<rune_t> runes{};
        private:
            std::vector<byte_t> buf{};
            bool erroneous{false};
    };
}

#endif