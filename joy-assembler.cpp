// Jonathan Frech, August 2020

#include <bitset>
#include <chrono>
#include <fstream>
#include <iostream>
#include <regex>
#include <thread>
#include <variant>
#include <vector>

namespace Util {
    std::string to_upper(std::string);
    std::string UInt32AsPaddedHex(uint32_t n);

    namespace std20 {
        /* a custom std::map::contains (C++20) implementation */
        template<typename K, typename V>
        bool contains(std::map<K, V> map, K key) {
            return map.count(key) != 0; }
    }
}


typedef uint8_t byte_t;
typedef uint32_t word_t;
typedef uint32_t rune_t;

enum class MemoryMode { LittleEndian, BigEndian };

word_t MEMORY_SIZE = 0x10000;
MemoryMode MEMORY_MODE = MemoryMode::LittleEndian;

bool DO_WAIT_FOR_USER = false;
bool DO_VISUALIZE_STEPS = false;


#define NO_ARG std::tuple{false, std::nullopt}
#define REQ_ARG std::tuple{true, std::nullopt}
#define OPT_ARG(ARG) std::tuple{true, std::optional<word_t>{ARG}}
#define MAP_ON_INSTRUCTION_NAMES(M) \
    M(NOP, OPT_ARG(0)), \
    \
    M(LDA, REQ_ARG), M(LDB, REQ_ARG), M(STA, REQ_ARG), M(STB, REQ_ARG), \
    M(LIA, OPT_ARG(0)), M(SIA, OPT_ARG(0)), M(LPC, NO_ARG), M(SPC, NO_ARG), \
    M(LYA, REQ_ARG), M(SYA, REQ_ARG), \
    \
    M(JMP, REQ_ARG), M(JZ, REQ_ARG), M(JNZ, REQ_ARG), M(JN, REQ_ARG), \
    M(JNN, REQ_ARG), M(JE, REQ_ARG), M(JNE, REQ_ARG), \
    \
    M(CAL, REQ_ARG), M(RET, NO_ARG), M(PSH, NO_ARG), M(POP, NO_ARG), \
    M(LSA, OPT_ARG(0)), M(SSA, OPT_ARG(0)), M(LSC, NO_ARG), M(SSC, NO_ARG), \
    \
    M(MOV, REQ_ARG), M(NOT, NO_ARG), M(SHL, OPT_ARG(1)), M(SHR, OPT_ARG(1)), \
    M(INC, OPT_ARG(1)), M(DEC, OPT_ARG(1)), M(NEG, NO_ARG), \
    \
    M(SWP, NO_ARG), M(ADD, NO_ARG), M(SUB, NO_ARG), M(AND, NO_ARG), \
    M(OR, NO_ARG), M(XOR, NO_ARG), \
    \
    M(GET, NO_ARG), M(GTC, NO_ARG), \
    M(PTU, NO_ARG), M(PTS, NO_ARG), M(PTB, NO_ARG), M(PTC, NO_ARG), \
    \
    M(HLT, NO_ARG)

enum class InstructionName {
    #define ID(ACT, _) ACT
    MAP_ON_INSTRUCTION_NAMES(ID)
    #undef ID
};

namespace InstructionNameRepresentationHandler {
    std::map<InstructionName, std::string> representation = {
        #define REPR(ACT, _) {InstructionName::ACT, #ACT}
        MAP_ON_INSTRUCTION_NAMES(REPR)
        #undef REPR
    };
    std::array<std::optional<InstructionName>, 256> instructions = {
        #define NAMESPACE_ID(ACT, _) InstructionName::ACT
        MAP_ON_INSTRUCTION_NAMES(NAMESPACE_ID)
        #undef NAMESPACE_ID
    };

    std::map<InstructionName, std::tuple<bool, std::optional<word_t>>>
        argumentType = {
            #define ARG_TYPES(ACT, CANDEF) \
                {InstructionName::ACT, CANDEF}
            MAP_ON_INSTRUCTION_NAMES(ARG_TYPES)
            #undef ARG_TYPES
    };

    std::optional<InstructionName> fromByteCode(byte_t opCode) {
        return instructions[opCode]; }

    byte_t toByteCode(InstructionName name) {
        for (uint16_t opCode = 0; opCode < 0x100; opCode++) {
            if (!instructions[opCode].has_value())
                continue;
            if (instructions[opCode].value() == name)
                return static_cast<byte_t>(opCode); }
        return 0x00; }

    std::string to_string(InstructionName name) {
        if (Util::std20::contains<InstructionName, std::string>(representation, name))
            return representation[name];
        return representation[InstructionName::NOP]; }

    std::optional<InstructionName> from_string(std::string repr) {
        for (auto const&[in, insr] : representation)
            if (insr == Util::to_upper(repr))
                return std::optional<InstructionName>{in};
        return std::nullopt; }
}
#undef NO_ARG
#undef REQ_ARG
#undef OPT_ARG
#undef MAP_ON_INSTRUCTION_NAMES

struct Instruction { InstructionName name; word_t argument; };

namespace InstructionRepresentationHandler {
    std::string to_string(Instruction instruction) {
        return InstructionNameRepresentationHandler::to_string(instruction.name)
               + " " + Util::UInt32AsPaddedHex(instruction.argument); }
}

namespace Util {
    namespace ANSI_COLORS {
        const std::string CLEAR = "\33[0m";

        const std::string INSTRUCTION_NAME = "\33[38;5;119m";
        const std::string INSTRUCTION_ARGUMENT = "\33[38;5;121m";
        const std::string STACK = "\33[38;5;127m";
        const std::string STACK_FAINT = "\33[38;5;53m";
        const std::string MEMORY_LOCATION_USED = "\33[1m";

        std::string paint(std::string ansi, std::string text) {
            return ansi + text + CLEAR; }
    }

    rune_t ERROR_RUNE = static_cast<rune_t>(0xfffd);
    class UTF8Encoder {
        public: bool encode(rune_t rune) {
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

        public: bool decode(byte_t b) {
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
        public: Stream(std::vector<T> values, T zeroValue) :
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

    std::optional<std::vector<rune_t>> parseString(std::string s) {
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

    std::optional<word_t> stringToUInt32(std::string s) {
        try {
            if (std::regex_match(s, std::regex{"\\s*[+-]?0[xX][0-9a-fA-F]+\\s*"}))
                return std::optional{static_cast<word_t>(std::stoll(s, nullptr, 16))};
            if (std::regex_match(s, std::regex{"\\s*[+-]?0[bB][01]+\\s*"})) {
                s = std::regex_replace(s, std::regex{"0[bB]"}, "");
                return std::optional{static_cast<word_t>(std::stoll(s, nullptr, 2))}; }
            if (std::regex_match(s, std::regex{"\\s*[+-]?[0-9]+\\s*"}))
                return std::optional{static_cast<word_t>(std::stoll(s, nullptr, 10))};
            return std::nullopt;
        } catch (std::invalid_argument const&_) {
            return std::nullopt; }}

    std::string UInt32AsPaddedHex(uint32_t n) {
        char buf[11];
        std::snprintf(buf, 11, "0x%08x", n);
        return std::string{buf}; }
    std::string UInt8AsPaddedHex(uint8_t n) {
        char buf[3];
        std::snprintf(buf, 3, "%02X", n);
        return std::string{buf}; }

    std::string to_upper(std::string _str) {
        std::string str{_str};
        for (auto &c : str)
            c = std::toupper(c);
        return str; }

    void put_byte(byte_t b) {
        std::cout.put(b); }

    byte_t get_byte() {
        return std::cin.get(); }

    void put_utf8_char(rune_t rune) {
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

class ComputationState {
    private:
        std::vector<byte_t> memory;
        word_t registerA, registerB, registerPC, registerSC;
        word_t memorySize;

        bool flagAZero, flagANegative, flagAEven;

        word_t debugProgramTextSize;
        word_t debugHighestUsedMemoryLocation;
        uint64_t debugExecutionCycles;

        bool mock;

    public: ComputationState() :
        memory{std::vector<byte_t>(MEMORY_SIZE)},
        registerA{0}, registerB{0}, registerPC{0}, registerSC{0},
        memorySize{MEMORY_SIZE},

        debugProgramTextSize{0},
        debugHighestUsedMemoryLocation{0},
        debugExecutionCycles{0},

        mock{false}
    {
        updateFlags(); }

    public: Instruction nextInstruction() {
        byte_t opCode{loadMemory(static_cast<word_t>(registerPC++))};
        word_t arg{loadMemory4(static_cast<word_t>((registerPC += 4) - 4))};

        auto oInstructionName = InstructionNameRepresentationHandler
                                ::fromByteCode(opCode);
        if (!oInstructionName.has_value())
            return Instruction{InstructionName::NOP, 0x00000000};

        InstructionName name = oInstructionName.value();
        word_t argument = arg;

        return Instruction{name, argument};
    }

    public: bool step() {
        auto instruction = nextInstruction();
        auto jmp = [&](bool cnd) {
            if (cnd)
                registerPC = static_cast<word_t>(instruction.argument); };

        switch (instruction.name) {
            case InstructionName::NOP: break;

            case InstructionName::LDA:
                registerA = loadMemory4(instruction.argument);
                break;
            case InstructionName::LDB:
                registerB = loadMemory4(instruction.argument);
                break;
            case InstructionName::STA:
                storeMemory4(instruction.argument, registerA);
                break;
            case InstructionName::STB:
                storeMemory4(instruction.argument, registerB);
                break;
            case InstructionName::LIA:
                registerA = loadMemory4(registerB + instruction.argument);
                break;
            case InstructionName::SIA:
                storeMemory4(registerB + instruction.argument, registerA);
                break;
            case InstructionName::LPC: registerPC = registerA; break;
            case InstructionName::SPC: registerA = registerPC; break;
            case InstructionName::LYA:
                registerA = (registerA                        & 0xffffff00)
                          | (loadMemory(instruction.argument) & 0x000000ff);
                break;
            case InstructionName::SYA:
                storeMemory(instruction.argument, static_cast<byte_t>(
                    registerA & 0xff));
                break;

            case InstructionName::JMP: jmp(true); break;
            case InstructionName::JZ: jmp(flagAZero); break;
            case InstructionName::JNZ: jmp(!flagAZero); break;
            case InstructionName::JN: jmp(flagANegative); break;
            case InstructionName::JNN: jmp(!flagANegative); break;
            case InstructionName::JE: jmp(flagAEven); break;
            case InstructionName::JNE: jmp(!flagAEven); break;

            case InstructionName::CAL:
                storeMemory4(registerSC, registerPC);
                registerSC += 4;
                registerPC = instruction.argument;
                break;
            case InstructionName::RET:
                registerSC -= 4;
                registerPC = loadMemory4(registerSC);
                break;
            case InstructionName::PSH:
                storeMemory4(registerSC, registerA);
                registerSC += 4;
                break;
            case InstructionName::POP:
                registerA = loadMemory4(registerSC -= 4);
                break;
            case InstructionName::LSA:
                registerA = loadMemory4(registerSC + instruction.argument);
                break;
            case InstructionName::SSA:
                storeMemory4(registerSC + instruction.argument, registerA);
                break;
            case InstructionName::LSC: registerSC = registerA; break;
            case InstructionName::SSC: registerA = registerSC; break;

            case InstructionName::MOV: registerA = instruction.argument; break;
            case InstructionName::NOT: registerA = ~registerA; break;
            case InstructionName::SHL:
                registerA = static_cast<uint32_t>(registerA)
                          << static_cast<uint8_t>(instruction.argument);
                break;
            case InstructionName::SHR:
                registerA = static_cast<uint32_t>(registerA)
                          >> static_cast<uint8_t>(instruction.argument);
                break;
            case InstructionName::INC:
                registerA = static_cast<word_t>(static_cast<int32_t>(registerA)
                    + static_cast<int32_t>(instruction.argument));
                break;
            case InstructionName::DEC:
                registerA = static_cast<word_t>(static_cast<int32_t>(registerA)
                    - static_cast<int32_t>(instruction.argument));
                break;
            case InstructionName::NEG:
                registerA = static_cast<word_t>(
                    -static_cast<int32_t>(registerA));
                break;

            case InstructionName::SWP: std::swap(registerA, registerB); break;
            case InstructionName::AND: registerA &= registerB; break;
            case InstructionName::OR: registerA |= registerB; break;
            case InstructionName::XOR: registerA ^= registerB; break;
            case InstructionName::ADD:
                registerA = static_cast<word_t>(
                    static_cast<int32_t>(registerA)
                    + static_cast<int32_t>(registerB));
                break;
            case InstructionName::SUB:
                registerA = static_cast<word_t>(
                    static_cast<int32_t>(registerA)
                    - static_cast<int32_t>(registerB));
                break;

            case InstructionName::PTU:
                if (mock)
                    break;
                std::cout << static_cast<uint32_t>(registerA) << "\n";
                break;
            case InstructionName::PTS:
                if (mock)
                    break;
                std::cout << static_cast<int32_t>(registerA) << "\n";
                break;
            case InstructionName::PTB:
                if (mock)
                    break;
                std::cout << "0b" << std::bitset<32>(registerA) << "\n";
                break;
            case InstructionName::PTC:
                if (mock)
                    break;
                Util::put_utf8_char(registerA);
                break;
            case InstructionName::GET: {
                if (mock) {
                    registerA = 0;
                    break; }
                std::optional<uint32_t> oN{std::nullopt};
                while (!oN.has_value()) {
                    std::cout << "enter a number: ";
                    std::string get;
                    std::getline(std::cin, get);
                    oN = Util::stringToUInt32(get);
                }
                registerA = static_cast<word_t>(oN.value());
            }; break;
            case InstructionName::GTC:
                std::cout << "enter a character: ";
                registerA = Util::get_utf8_char();
                break;

            case InstructionName::HLT: return false;
        }

        updateFlags();
        debugExecutionCycles++;

        std::flush(std::cout);
        return true;
    }

    void visualize() {
        std::printf("\n\n\n");

        std::printf("\n=== MEMORY ===\n");
        word_t pc = 0, rPC = registerPC;
        std::size_t w = 16;
        std::printf("       ");
        for (std::size_t x = 0; x < w; x++)
            std::printf("_%01X ", (int) x);
        for (std::size_t y = 0; true; y++) {
            std::printf("\n    %02X_", (int) y);
            for (std::size_t x = 0; x < w; x++) {
                word_t m = y *w+ x;
                if (registerSC <= pc+4 && pc+4 < registerSC + 4)
                    std::cout << Util::ANSI_COLORS::STACK_FAINT;
                if (registerSC <= pc && pc < registerSC + 4)
                    std::cout << Util::ANSI_COLORS::STACK;
                if (rPC <= pc && pc < rPC + 5)
                    std::cout << (pc == rPC ? Util::ANSI_COLORS::INSTRUCTION_NAME : Util::ANSI_COLORS::INSTRUCTION_ARGUMENT);
                else if (m <= debugHighestUsedMemoryLocation)
                    std::cout << Util::ANSI_COLORS::MEMORY_LOCATION_USED;
                std::printf(" %02X", memory[y *w+ x]);
                std::cout << Util::ANSI_COLORS::CLEAR;
                pc++;
            }

            if ((y+1)*w >= 0x100 && debugHighestUsedMemoryLocation+1 < (y+1)*w)
                break;
        }
        std::printf("\n");

        std::printf("\n=== CURRENT INSTRUCTION ===\n");
        byte_t opCode = loadMemory(static_cast<word_t>(registerPC));
        std::string opCodeName = "(err. NOP)";
        auto oInstructionName = InstructionNameRepresentationHandler
                                ::fromByteCode(opCode);
        if (oInstructionName.has_value())
            opCodeName = InstructionNameRepresentationHandler
                         ::to_string(oInstructionName.value());
        word_t argument{loadMemory4(static_cast<word_t>(registerPC) + 1)};
        std::cout << "    " << Util::ANSI_COLORS::paint(Util::ANSI_COLORS::INSTRUCTION_NAME, opCodeName) << " " << Util::ANSI_COLORS::paint(Util::ANSI_COLORS::INSTRUCTION_ARGUMENT, Util::UInt32AsPaddedHex(argument)) << "\n";

        std::printf("\n=== REGISTERS ===\n");
        std::printf("    A:  0x%08x,    B:  0x%08x,\n    PC: 0x%08x,"
                    "    SC: 0x%08x\n"
                   , registerA, registerB, registerPC, registerSC);

        std::printf("\n=== FLAGS ===\n");
        std::printf("    flagAZero: %d,    flagANegative: %d,\n    "
                    "flagAEven: %d\n", flagAZero, flagANegative
                   , flagAEven);

        std::cout << "\n";
        printExecutionCycles();
    }

    void printExecutionCycles() {
        std::cout << "Execution cycles: " << debugExecutionCycles << "\n"; }

    public: void memoryDump() {
        mock = true;

        std::string dump{""};
        dump += "A: " + Util::UInt32AsPaddedHex(registerA);
        dump += ", B: " + Util::UInt32AsPaddedHex(registerB);
        dump += ", PC: " + Util::UInt32AsPaddedHex(registerPC);
        dump += ", SC: " + Util::UInt32AsPaddedHex(registerSC);
        dump += "; memory (" + std::to_string(memorySize) + "B):";
        word_t mx = memorySize;
        while (--mx != 0 && memory[mx] == 0)
            ;
        mx++;
        for (word_t m = 0; m < mx; m++)
            dump += " " + Util::UInt8AsPaddedHex(memory[m]);

        std::cout << dump << "\n";
    }

    private:

    void updateFlags() {
        flagAZero = registerA == 0;
        flagANegative = static_cast<int32_t>(registerA) < 0;
        flagAEven = registerA % 2 == 0;
    }

    byte_t loadMemory(word_t m) {
        debugHighestUsedMemoryLocation = std::max(
            debugHighestUsedMemoryLocation, m);
        return m < MEMORY_SIZE ? memory[m] : 0;
    }

    void storeMemory(word_t m, byte_t b) {
        debugHighestUsedMemoryLocation = std::max(
            debugHighestUsedMemoryLocation, m);
        if (m < MEMORY_SIZE)
            memory[m] = b;
    }

    word_t loadMemory4(word_t m) {
        byte_t b3, b2, b1, b0;
        switch (MEMORY_MODE) {
            case MemoryMode::LittleEndian:
                b3 = loadMemory(m+3);
                b2 = loadMemory(m+2);
                b1 = loadMemory(m+1);
                b0 = loadMemory(m+0);
                break;
            case MemoryMode::BigEndian:
                b3 = loadMemory(m+0);
                b2 = loadMemory(m+1);
                b1 = loadMemory(m+2);
                b0 = loadMemory(m+3);
                break;
        }
        return (b3 << 24) | (b2 << 16) | (b1 << 8) | b0;
    }

    void storeMemory4(word_t m, word_t w) {
        byte_t b3 = static_cast<byte_t>((w >> 24) & 0xff);
        byte_t b2 = static_cast<byte_t>((w >> 16) & 0xff);
        byte_t b1 = static_cast<byte_t>((w >>  8) & 0xff);
        byte_t b0 = static_cast<byte_t>( w        & 0xff);
        switch (MEMORY_MODE) {
            case MemoryMode::LittleEndian:
                storeMemory(m+3, b3);
                storeMemory(m+2, b2);
                storeMemory(m+1, b1);
                storeMemory(m+0, b0);
                break;
            case MemoryMode::BigEndian:
                storeMemory(m+0, b3);
                storeMemory(m+1, b2);
                storeMemory(m+2, b1);
                storeMemory(m+3, b0);
                break;
        }
    }

    public: word_t storeInstruction(word_t m, Instruction instruction) {
        storeMemory(m, InstructionNameRepresentationHandler
                       ::toByteCode(instruction.name));
        storeMemory4(1+m, instruction.argument);
        debugHighestUsedMemoryLocation = 0;
        return 5;
    }

    public: word_t storeData(word_t m, word_t data) {
        storeMemory4(m, data);
        debugHighestUsedMemoryLocation = 0;
        return 4;
    }
};

bool parse(std::string filename, ComputationState &cs) {
    bool dbg{false};

    std::ifstream is{filename};
    if (!is.is_open()) {
        std::cerr << "could not read input joy assembly file\n";
        return false; }

    typedef std::tuple<InstructionName, std::optional<std::string>> parsingInstruction;
    typedef word_t parsingData;
    typedef uint64_t line_number_t;
    std::vector<std::tuple<line_number_t, std::variant<parsingInstruction, parsingData>>> parsing;
    std::map<std::string, std::string> definitions{};

    word_t memPtr{0};

    auto parseError = [filename](line_number_t lineNumber, std::string msg){
        std::cerr << filename << ", ln " << lineNumber << ": " << msg << "\n"; };

    for (auto [lineNumber, ln] = std::tuple<line_number_t, std::string>{1, ""}; std::getline(is, ln); lineNumber++) {
        ln = std::regex_replace(ln, std::regex{";.*"}, "");
        ln = std::regex_replace(ln, std::regex{"\\s\\s+"}, " ");
        ln = std::regex_replace(ln, std::regex{"^\\s+"}, "");
        ln = std::regex_replace(ln, std::regex{"\\s+$"}, "");

        if (ln == "")
            continue;

        if (dbg)
            std::cout << "ln: " << ln << "\n";

        auto define = [parseError, lineNumber, &definitions, dbg]
            (std::string k, std::string v)
        {
            if (dbg)
                std::cout << "defining " << k << " to be " << v << "\n";
            if (Util::std20::contains<std::string, std::string>(definitions, k)) {
                parseError(lineNumber, "duplicate definition: " + k);
                return false; }
            definitions[k] = v;
            return true;
        };

        {
            std::smatch _def;
            std::regex_match(ln, _def, std::regex{"^(\\S+)\\s+:=\\s+(\\S+)$"});
            if (_def.size() == 3) {
                if (!define(_def[1], _def[2]))
                    return false;
                continue;
            }
        }

        {
            std::smatch _label;
            std::regex_match(ln, _label, std::regex{"^(\\S+):$"});
            if (_label.size() == 2) {
                if (!define("@" + std::string{_label[1]}, std::to_string(memPtr)))
                    return false;
                continue;
            }
        }

        {
            std::smatch _data;
            std::regex_match(ln, _data, std::regex{"^data\\s*(.+)$"});
            if (_data.size() == 2) {
                std::string data{_data[1]};
                while (!data.empty()) {
                    std::smatch _item;
                    std::regex_match(data, _item, std::regex{"(\"(?:[^\"\\\\]|\\\\.)*\"|[^\",]*)(,\\s*(.*))?"});
                    if (_item.size() != 4) {
                        parseError(lineNumber, "invalid data: " + data);
                        return false; }
                    std::string item{_item[1]};
                    if (item[0] == '"') {
                        std::optional<std::vector<rune_t>> oRunes = Util::parseString(item);
                        if (!oRunes.has_value()) {
                            parseError(lineNumber, "invalid data: " + data);
                            return false; }
                        for (rune_t rune : oRunes.value()) {
                            parsing.push_back(std::make_tuple(lineNumber,
                                parsingData{static_cast<word_t>(rune)}));
                            memPtr += 4; }
                        data = _item[3];
                        continue; }

                    {
                        std::smatch _match;
                        if (std::regex_match(item, _match, std::regex{"\\[(.+)\\]\\s*(.+)"})) {
                            auto oSize = Util::stringToUInt32(_match[1]);
                            if (!oSize.has_value()) {
                                parseError(lineNumber, "invalid data size: " + item + ", i.e. " + std::string{_match[1]});
                                return false; }
                            auto oValue = Util::stringToUInt32(_match[2]);
                            if (!oValue.has_value()) {
                                parseError(lineNumber, "invalid data value: " + item + ", i.e. " + std::string{_match[2]});
                                return false; }
                            for (std::size_t j = 0; j < oSize.value(); j++) {
                                parsing.push_back(std::make_tuple(lineNumber,
                                    parsingData{oValue.value()}));
                                memPtr += 4; }
                            data = _item[3];
                            continue; }
                    }

                    auto oValue = Util::stringToUInt32(item);
                    if (!oValue.has_value()) {
                        parseError(lineNumber, "invalid data value: " + item);
                        return false; }
                    parsing.push_back(std::make_tuple(lineNumber,
                        parsingData{oValue.value()}));
                    memPtr += 4;
                    data = _item[3];
                }
                continue;
            }
        }

        {
            std::smatch _int;
            std::regex_match(ln, _int, std::regex{"^int\\s*\\[(.+?)\\]\\s*(.+?)$"});
            if (_int.size() == 3) {
                auto oSize = Util::stringToUInt32(_int[1]);
                if (!oSize.has_value() || oSize.value() <= 0) {
                    parseError(lineNumber, "invalid int data size: " + std::string{_int[1]});
                    return false; }
                std::optional<word_t> oValue{Util::stringToUInt32(_int[2])};
                if (!oValue.has_value()) {
                    parseError(lineNumber, "invalid int data value: " + std::string{_int[2]});
                    return false; }
                for (std::size_t j = 0; j < oSize.value(); j++) {
                    parsing.push_back(std::make_tuple(lineNumber,
                        parsingData{static_cast<word_t>(oValue.value())}));
                    memPtr += 4;
                }
                continue;
            }
        }

        {
            std::smatch _instr;
            std::regex_match(ln, _instr, std::regex{"^(\\S+)(\\s+(\\S+))?$"});
            if (_instr.size() == 4) {
                auto oName = InstructionNameRepresentationHandler::from_string(_instr[1]);
                if (!oName.has_value()) {
                    parseError(lineNumber, "invalid instruction name: " + std::string{_instr[1]});
                    return false; }
                std::optional<std::string> oArg{std::nullopt};
                if (_instr[3] != "")
                    oArg = std::optional{_instr[3]};
                parsing.push_back(std::make_tuple(lineNumber,
                    parsingInstruction{std::make_tuple(oName.value(), oArg)}));
                memPtr += 5;
                continue;
            }
        }

        parseError(lineNumber, "incomprehensible");
        return false;
    }

    memPtr = 0;
    for (auto [lineNumber, p] : parsing) {
        if (std::holds_alternative<parsingData>(p)) {
            word_t data{std::get<parsingData>(p)};
            if (dbg)
                std::printf("data value 0x%08x\n", data);
            memPtr += cs.storeData(memPtr, data);
        }
        else if (std::holds_alternative<parsingInstruction>(p)) {
            parsingInstruction instruction{std::get<parsingInstruction>(p)};
            InstructionName name = std::get<0>(instruction);
            auto oArg = std::get<1>(instruction);
            std::optional<word_t> oValue{std::nullopt};
            if (oArg.has_value()) {
                word_t value = 0;
                if (Util::std20::contains<std::string, std::string>(definitions, oArg.value()))
                    oArg = std::make_optional(definitions[oArg.value()]);

                oValue = static_cast<std::optional<word_t>>(Util::stringToUInt32(oArg.value()));
                if (!oValue.has_value()) {
                    parseError(lineNumber, "invalid value: " + oArg.value());
                    return false; }
                value += oValue.value();

                oValue = std::make_optional(value);
            }



            auto [hasArgument, optionalValue] = InstructionNameRepresentationHandler
                                                ::argumentType[name];

            if (!hasArgument && oValue.has_value()) {
                parseError(lineNumber, "superfluous argument: " + InstructionNameRepresentationHandler::to_string(name) + " " + std::to_string(oValue.value()));
                return false; }
            if (hasArgument) {
                if (!optionalValue.has_value() && !oValue.has_value()) {
                    parseError(lineNumber, "requiring argument: " + InstructionNameRepresentationHandler::to_string(name));
                    continue; }
                if (!oValue.has_value())
                    oValue = optionalValue;
            }

            if (oValue.has_value()) {
                if (dbg)
                    std::cout << "instruction " << InstructionNameRepresentationHandler::to_string(name) << " " << oValue.value() << "\n";
                auto argument = oValue.value();
                memPtr += cs.storeInstruction(memPtr, Instruction{name, argument});
            } else {
                if (dbg)
                    std::cout << "instruction " << InstructionNameRepresentationHandler::to_string(name) << " (none)\n";
                memPtr += cs.storeInstruction(memPtr, Instruction{name, 0x00000000});
            }
        }

        if (dbg) {
            cs.visualize();
            std::cin.get(); }
    }

    return true;
}


int main(int argc, char const*argv[]) {
    if (argc < 2) {
        std::cerr << "please provide an input joy assembly file\n";
        return EXIT_FAILURE; }
    if (argc > 2 && std::string{argv[2]} == "visualize")
        DO_VISUALIZE_STEPS = true;
    if (argc > 2 && std::string{argv[2]} == "step")
        DO_VISUALIZE_STEPS = DO_WAIT_FOR_USER = true;

    ComputationState cs{};

    std::string filename{argv[1]};
    if (!parse(filename, cs)) {
        std::cerr << "faulty joy assembly file\n";
        return EXIT_FAILURE; }

    if (argc > 2 && std::string{argv[2]} == "memory-dump") {
        do {
            cs.memoryDump();
        } while (cs.step());
        cs.memoryDump();
        return EXIT_SUCCESS; }

    do {
        if (DO_VISUALIZE_STEPS)
            cs.visualize();
        if (DO_WAIT_FOR_USER)
            std::cin.get();
        else if (DO_VISUALIZE_STEPS)
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
    } while (cs.step());

    if (argc > 2 && std::string{argv[2]} == "cycles")
        cs.printExecutionCycles();
}
