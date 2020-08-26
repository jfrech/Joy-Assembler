// Jonathan Frech, August 2020

#include <bitset>
#include <chrono>
#include <fstream>
#include <iostream>
#include <regex>
#include <thread>
#include <variant>
#include <vector>

namespace Util{
    std::string to_upper(std::string); }

enum class MemoryMode { LittleEndian, BigEndian };

std::size_t MEMORY_SIZE = 0x10000;
MemoryMode MEMORY_MODE = MemoryMode::LittleEndian;

bool DO_WAIT_FOR_USER = false;
bool DO_VISUALIZE_STEPS = false;

enum class Register { RegisterA, RegisterB };
typedef uint8_t byte;
typedef uint32_t programCounter_t;

typedef int32_t reg_t;
typedef uint32_t ureg_t;
typedef uint32_t arg_t;
typedef uint32_t mem_t;


#define NO_ARG std::tuple{false, std::nullopt}
#define REQ_ARG std::tuple{true, std::nullopt}
#define OPT_ARG(ARG) std::tuple{true, std::optional<uint32_t>{ARG}}
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
    M(PTU, NO_ARG), M(GTU, NO_ARG), M(PTS, NO_ARG), M(GTS, NO_ARG), \
    M(PTB, NO_ARG), M(GTB, NO_ARG), M(PTC, NO_ARG), M(GTC, NO_ARG), \
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

    std::map<InstructionName, std::tuple<bool, std::optional<uint32_t>>>
        argumentType = {
            #define ARG_TYPES(ACT, CANDEF) \
                {InstructionName::ACT, CANDEF}
            MAP_ON_INSTRUCTION_NAMES(ARG_TYPES)
            #undef ARG_TYPES
    };

    std::optional<InstructionName> fromByteCode(byte opCode) {
        return instructions[opCode]; }

    byte toByteCode(InstructionName name) {
        for (uint16_t opCode = 0; opCode < 0x100; opCode++) {
            if (!instructions[opCode].has_value())
                continue;
            if (instructions[opCode].value() == name)
                return static_cast<byte>(opCode); }
        return 0x00; }

    std::string to_string(InstructionName name) {
        if (representation.contains(name))
            return representation[name];
        return representation[InstructionName::NOP]; }

    std::optional<InstructionName> from_string(std::string repr) {
        for (auto const&[in, insr] : representation)
            if (insr == Util::to_upper(repr))
                return std::optional<InstructionName>{in};
        return std::nullopt; }
}
#undef MAP_ON_INSTRUCTION_NAMES

struct Instruction { InstructionName name; arg_t argument; };

namespace InstructionRepresentationHandler {
    std::string to_string(Instruction instruction) {
        char buffer[11];
        //std::snprintf(buffer, 11, "0x%08x", instruction.argument);
        return InstructionNameRepresentationHandler
               ::to_string(instruction.name) + " " + std::string{buffer}; }
}

namespace Util {
    template<typename T>
    void swap_values(T &x, T &y) {
        auto tmp = x;
        x = y;
        y = tmp; }

    std::string to_upper(std::string _str) {
        std::string str{_str};
        for (auto &c : str)
            c = std::toupper(c);
        return str; }

    void splitUInt32(uint32_t bytes, byte &b3, byte &b2, byte &b1, byte &b0) {
        b3 = static_cast<byte>((bytes >> 24) & 0xff);
        b2 = static_cast<byte>((bytes >> 16) & 0xff);
        b1 = static_cast<byte>((bytes >>  8) & 0xff);
        b0 = static_cast<byte>( bytes        & 0xff); }

    uint32_t combineUInt32(byte b3, byte b2, byte b1, byte b0) {
        return b3 << 24 | b2 << 16 | b1 << 8 | b0; }

    void put_byte(byte b) {
        std::putchar(b); }
    byte get_byte() {
        return std::getchar(); }

    std::vector<byte> unicode_to_utf8(uint32_t unicode) {
        if (unicode <= 0x7f)
            return std::vector<byte>{
                static_cast<byte>(0b0'0000000 | ( unicode        & 0b0'1111111))};
        if (unicode <= 0x07ff)
            return std::vector<byte>{
                static_cast<byte>(0b110'00000 | ((unicode >>  6) & 0b000'11111)),
                static_cast<byte>(0b10'000000 | ( unicode        & 0b00'111111))};
        if (unicode <= 0xffff)
            return std::vector<byte>{
                static_cast<byte>(0b1110'0000 | ((unicode >> 12) & 0b0000'1111)),
                static_cast<byte>(0b10'000000 | ((unicode >>  6) & 0b00'111111)),
                static_cast<byte>(0b10'000000 | ( unicode        & 0b00'111111))};
        if (unicode <= 0x10ffff)
            return std::vector<byte>{
                static_cast<byte>(0b11110'000 | ((unicode >> 18) & 0b00000'111)),
                static_cast<byte>(0b10'000000 | ((unicode >> 12) & 0b00'111111)),
                static_cast<byte>(0b10'000000 | ((unicode >>  6) & 0b00'111111)),
                static_cast<byte>(0b10'000000 | ( unicode        & 0b00'111111))};

        return std::vector<byte>{}; }

    std::tuple<uint32_t, std::size_t> utf8_to_unicode(std::vector<byte> utf8) {
        #define REQ(N) if (utf8.size() < (N))\
            return std::make_tuple(0, 0);

        REQ(1) byte b0 = utf8[0];
        if ((b0 & 0b1'0000000) == 0b0'0000000)
            return std::make_tuple(
                   0b0'1111111 & b0
                , 1);

        REQ(2) byte b1 = utf8[1];
        if ((b0 & 0b111'00000) == 0b110'00000)
            return std::make_tuple(
                  (0b000'11111 & b0) <<  6
                | (0b00'111111 & b1)
                , 2);

        REQ(3) byte b2 = utf8[2];
        if ((b0 & 0b1111'0000) == 0b1110'0000)
            return std::make_tuple(
                  (0b0000'1111 & b0) << 12
                | (0b00'111111 & b1) <<  6
                | (0b00'111111 & b2)
                , 3);

        REQ(4) byte b3 = utf8[3];
        if ((b0 & 0b11111'000) == 0b11110'000)
            return std::make_tuple(
                  (0b00000'111 & b0) << 18
                | (0b00'111111 & b1) << 12
                | (0b00'111111 & b2) <<  6
                | (0b00'111111 & b3)
                , 4);

        return std::make_tuple(0, 0);
        #undef REQ
    }

    void put_utf8_char(uint32_t unicode) {
        for (auto c : unicode_to_utf8(unicode))
            put_byte(c); }

    uint32_t get_utf8_char() {
        std::vector<byte> bytes{};
        while (bytes.size() < 4) {
            byte b = get_byte();
            bytes.push_back(b);
            auto [unicode, readBytes] = utf8_to_unicode(bytes);
            if (readBytes == bytes.size()) {
                return unicode; }}
        return 0xfffd; }

    namespace ANSI_COLORS {
        const std::string INSTRUCTION_NAME = "\33[38;5;119m";
        const std::string INSTRUCTION_ARGUMENT = "\33[38;5;121m";
        const std::string STACK = "\33[38;5;127m";
        const std::string STACK_FAINT = "\33[38;5;53m";
    }

    std::optional<uint32_t> stringToUInt32(std::string s) {
        try {
            long long int n{std::stoll(s, nullptr, 0)};
            if (n < 0)
                return std::nullopt;
            return std::make_optional(static_cast<uint32_t>(n));
        } catch (std::invalid_argument const&_) {
            return std::nullopt; }}
    std::optional<int32_t> stringToInt32(std::string s) {
        try {
            long long int n{std::stoll(s, nullptr, 0)};
            return std::make_optional(static_cast<int32_t>(n));
        } catch (std::invalid_argument const&_) {
            return std::nullopt; }}
}



class ComputationState {
    private:

    std::vector<byte> memory;
    reg_t registerA, registerB;
    uint32_t registerPC, registerSC;

    bool flagAZero, flagANegative, flagAParityEven;
    std::vector<Instruction> program;

    mem_t debugProgramTextSize;
    mem_t debugHighestUsedMemoryLocation;

    public:

    ComputationState() :
        memory{std::vector<byte>(MEMORY_SIZE)},
        registerA{0}, registerB{0}, registerPC{0}, registerSC{0},

        debugProgramTextSize{0},
        debugHighestUsedMemoryLocation{0}
    {
        updateFlags(); }

    Instruction nextInstruction() {
        byte opCode = loadMemory(static_cast<mem_t>(registerPC++));
        byte arg3, arg2, arg1, arg0;
        loadMemory4(static_cast<mem_t>((registerPC += 4) - 4)
                   , arg3, arg2, arg1, arg0);

        auto oInstructionName = InstructionNameRepresentationHandler
                                ::fromByteCode(opCode);
        if (!oInstructionName.has_value())
            return Instruction{InstructionName::NOP, 0x00000000};

        InstructionName name = oInstructionName.value();
        arg_t argument = arg3 << 24 | arg2 << 16 | arg1 << 8 | arg0;

        return Instruction{name, argument};
    }

    bool step() {
        auto instruction = nextInstruction();
        auto jmp = [&](bool cnd) {
            if (cnd)
                registerPC = programCounter_t{instruction.argument}; };

        switch (instruction.name) {
            case InstructionName::NOP: break;

            case InstructionName::LDA: {
                    byte b3, b2, b1, b0;
                    loadMemory4(mem_t{instruction.argument}, b3, b2, b1, b0);
                    registerA = Util::combineUInt32(b3, b2, b1, b0);
                }; break;
            case InstructionName::LDB: {
                    byte b3, b2, b1, b0;
                    loadMemory4(mem_t{instruction.argument}, b3, b2, b1, b0);
                    registerB = Util::combineUInt32(b3, b2, b1, b0);
                }; break;
            case InstructionName::STA: {
                mem_t bytes = static_cast<ureg_t>(registerA);
                storeMemory4(mem_t{instruction.argument}
                            , (bytes >> 24) & 0xff, (bytes >> 16) & 0xff
                            , (bytes >> 8) & 0xff, bytes & 0xff);
                }; break;
            case InstructionName::STB: {
                mem_t bytes = static_cast<ureg_t>(registerB);
                storeMemory4(mem_t{instruction.argument}
                            , (bytes >> 24) & 0xff, (bytes >> 16) & 0xff
                            , (bytes >> 8) & 0xff, bytes & 0xff);
                }; break;
            case InstructionName::LIA: {
                    int32_t offset = static_cast<int32_t>(instruction.argument);
                    byte b3, b2, b1, b0;
                    loadMemory4(static_cast<uint32_t>(registerB + offset)
                               , b3, b2, b1, b0);
                    registerA = Util::combineUInt32(b3, b2, b1, b0);
                }; break;
            case InstructionName::SIA: {
                    int32_t offset = static_cast<int32_t>(instruction.argument);
                    byte b3, b2, b1, b0;
                    Util::splitUInt32(registerA, b3, b2, b1, b0);
                    storeMemory4(static_cast<mem_t>(registerB + offset)
                                , b3, b2, b1, b0);
                }; break;
            case InstructionName::LPC: registerPC = registerA; break;
            case InstructionName::SPC: registerA = registerPC; break;
            case InstructionName::LYA:
                registerA = (static_cast<uint32_t>(registerA) & 0xffffff00)
                          | (static_cast<int32_t>(loadMemory(static_cast<mem_t>(
                             instruction.argument))) & 0x000000ff);
                break;
            case InstructionName::SYA:
                storeMemory(static_cast<mem_t>(instruction.argument)
                           , static_cast<byte>(static_cast<uint32_t>(registerA) & 0xff));
                break;

            case InstructionName::JMP: jmp(true); break;
            case InstructionName::JZ: jmp(flagAZero); break;
            case InstructionName::JNZ: jmp(!flagAZero); break;
            case InstructionName::JN: jmp(flagANegative); break;
            case InstructionName::JNN: jmp(!flagANegative); break;
            case InstructionName::JE: jmp(flagAParityEven); break;
            case InstructionName::JNE: jmp(!flagAParityEven); break;

            case InstructionName::CAL: {
                byte b3, b2, b1, b0;
                Util::splitUInt32(registerPC, b3, b2, b1, b0);
                storeMemory4(static_cast<uint32_t>(registerSC), b3, b2, b1, b0);
                registerSC += 4;
                registerPC = instruction.argument;
            }; break;
            case InstructionName::RET: {
                byte b3, b2, b1, b0;
                registerSC -= 4;
                loadMemory4(registerSC, b3, b2, b1, b0);
                registerPC = Util::combineUInt32(b3, b2, b1, b0);
            }; break;
            case InstructionName::PSH: {
                byte b3, b2, b1, b0;
                Util::splitUInt32(registerA, b3, b2, b1, b0);
                storeMemory4(registerSC, b3, b2, b1, b0);
                registerSC += 4;
            }; break;
            case InstructionName::POP: {
                byte b3, b2, b1, b0;
                registerSC -= 4;
                loadMemory4(registerSC, b3, b2, b1, b0);
                registerA = Util::combineUInt32(b3, b2, b1, b0);
            }; break;
            case InstructionName::LSA: {
                byte b3, b2, b1, b0;
                loadMemory4(registerSC
                           + static_cast<int32_t>(instruction.argument)
                           , b3, b2, b1, b0);
                registerA = Util::combineUInt32(b3, b2, b1, b0);
            }; break;
            case InstructionName::SSA: {
                byte b3, b2, b1, b0;
                Util::splitUInt32(registerA, b3, b2, b1, b0);
                storeMemory4(registerSC
                           + static_cast<int32_t>(instruction.argument)
                           , b3, b2, b1, b0);
            }; break;
            case InstructionName::LSC: registerSC = registerA; break;
            case InstructionName::SSC: registerA = registerSC; break;

            case InstructionName::MOV:
                registerA = static_cast<int32_t>(instruction.argument);
                break;
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
                registerA += static_cast<int32_t>(instruction.argument);
                break;
            case InstructionName::DEC:
                registerA -= static_cast<int32_t>(instruction.argument);
                break;
            case InstructionName::NEG: registerA = -registerA; break;

            case InstructionName::SWP:
                Util::swap_values(registerA, registerB);
                break;
            case InstructionName::AND: registerA &= registerB; break;
            case InstructionName::OR: registerA |= registerB; break;
            case InstructionName::XOR: registerA ^= registerB; break;
            case InstructionName::ADD: registerA += registerB; break;
            case InstructionName::SUB: registerA -= registerB; break;

            case InstructionName::PTU: {
                std::cout << static_cast<uint32_t>(registerA) << "\n";
            }; break;
            case InstructionName::GTU: {
                std::string get;
                std::getline(std::cin, get);
                try {
                    registerA = static_cast<uint32_t>(
                        std::stol(get, nullptr, 0)); }
                catch (std::invalid_argument const&_) {
                    registerA = 0; }
            }; break;
            case InstructionName::PTS: {
                std::cout << static_cast<int32_t>(registerA) << "\n";
            }; break;
            case InstructionName::GTS: {
                std::string get;
                std::getline(std::cin, get);
                try {
                    registerA = static_cast<int32_t>(
                        std::stol(get, nullptr, 0)); }
                catch (std::invalid_argument const&_) {
                    registerA = 0; }
            }; break;
            case InstructionName::PTB: {
                std::cout << std::bitset<32>(registerA) << "\n";
            }; break;
            case InstructionName::GTB: {
                std::string get;
                std::getline(std::cin, get);
                try {
                    registerA = static_cast<int32_t>(
                        std::stol(get, nullptr, 2)); }
                catch (std::invalid_argument const&_) {
                    registerA = 0; }
            }; break;
            case InstructionName::PTC:
                Util::put_utf8_char(registerA);
                break;
            case InstructionName::GTC:
                registerA = Util::get_utf8_char();
                break;

            case InstructionName::HLT: return false;
        }

        updateFlags();
        std::flush(std::cout);
        return true;
    }

    void visualize() {
        std::printf("\n\n\n");

        std::printf("\n=== MEMORY ===\n");
        programCounter_t pc = 0;
        programCounter_t rPC = static_cast<programCounter_t>(registerPC);
        std::size_t w = 16;
        std::printf("       ");
        for (std::size_t x = 0; x < w; x++)
            std::printf("_%01X ", (int) x);
        for (std::size_t y = 0; true; y++) {
            std::printf("\n    %02X_", (int) y);
            for (std::size_t x = 0; x < w; x++) {
                mem_t m = y *w+ x;
                if (registerSC <= pc+4 && pc+4 < registerSC + 4)
                    std::cout << Util::ANSI_COLORS::STACK_FAINT;
                if (registerSC <= pc && pc < registerSC + 4)
                    std::cout << Util::ANSI_COLORS::STACK;
                if (rPC <= pc && pc < rPC + 5)
                    std::cout << (pc == rPC ? Util::ANSI_COLORS::INSTRUCTION_NAME : Util::ANSI_COLORS::INSTRUCTION_ARGUMENT);
                else if (m <= debugHighestUsedMemoryLocation)
                    std::cout << "\33[1m";
                std::printf(" %02X", memory[y *w+ x]);
                std::cout << "\33[0m";
                pc++;
            }

            if ((y+1)*w >= 0x100 && debugHighestUsedMemoryLocation+1 < (y+1)*w)
                break;
        }
        std::printf("\n");

        std::printf("\n=== CURRENT INSTRUCTION ===\n");
        byte opCode = loadMemory(static_cast<mem_t>(registerPC));
        std::string opCodeName = "(err. NOP)";
        auto oInstructionName = InstructionNameRepresentationHandler
                                ::fromByteCode(opCode);
        if (oInstructionName.has_value())
            opCodeName = InstructionNameRepresentationHandler
                         ::to_string(oInstructionName.value());
        byte arg3, arg2, arg1, arg0;
        loadMemory4(static_cast<mem_t>(registerPC) + 1, arg3, arg2, arg1, arg0);
        arg_t argument = Util::combineUInt32(arg3, arg2, arg1, arg0);
        std::printf("    %s%s\033[0m %s0x%08X\033[0m\n", Util::ANSI_COLORS::INSTRUCTION_NAME.c_str(), opCodeName.c_str(), Util::ANSI_COLORS::INSTRUCTION_ARGUMENT.c_str(), argument);

        std::printf("\n=== REGISTERS ===\n");
        std::printf("    A:  0x%08x,    B:  0x%08x,\n    PC: 0x%08x,"
                    "    SC: 0x%08x\n"
                   , registerA, registerB, registerPC, registerSC);

        std::printf("\n=== FLAGS ===\n");
        std::printf("    flagAZero: %d,    flagANegative: %d,\n    "
                    "flagAParityEven: %d\n", flagAZero, flagANegative
                   , flagAParityEven);
    }


    private:

    void updateFlags() {
        flagAZero = registerA == 0;
        flagANegative = registerA < 0;
        flagAParityEven = std::bitset<32>{
            static_cast<uint32_t>(registerA)}.count() % 2 == 0;
    }

    byte loadMemory(mem_t m) {
        debugHighestUsedMemoryLocation = std::max(
            debugHighestUsedMemoryLocation, m);
        return m < MEMORY_SIZE ? memory[m] : 0; }


    void loadMemory4(mem_t m, byte &b3, byte &b2, byte &b1, byte &b0) {
        if (MEMORY_MODE == MemoryMode::LittleEndian) {
            b3 = loadMemory(m+3); b2 = loadMemory(m+2);
            b1 = loadMemory(m+1); b0 = loadMemory(m+0); }
        if (MEMORY_MODE == MemoryMode::BigEndian) {
            b3 = loadMemory(m+0); b2 = loadMemory(m+1);
            b1 = loadMemory(m+2); b0 = loadMemory(m+3); }}

    void storeMemory(mem_t m, byte b) {
        debugHighestUsedMemoryLocation = std::max(
            debugHighestUsedMemoryLocation, m);
        if (m < MEMORY_SIZE)
            memory[m] = b; }
    void storeMemory4(mem_t m, byte b3, byte b2, byte b1, byte b0) {
        if (MEMORY_MODE == MemoryMode::LittleEndian) {
            storeMemory(m+3, b3); storeMemory(m+2, b2);
            storeMemory(m+1, b1); storeMemory(m+0, b0); }
        if (MEMORY_MODE == MemoryMode::BigEndian) {
            storeMemory(m+0, b3); storeMemory(m+1, b2);
            storeMemory(m+2, b1); storeMemory(m+3, b0); }}

    public:
        mem_t storeInstruction(mem_t m, Instruction instruction) {
            storeMemory(m, InstructionNameRepresentationHandler
                           ::toByteCode(instruction.name));
            byte b3, b2, b1, b0;
            Util::splitUInt32(instruction.argument, b3, b2, b1, b0);
            storeMemory4(1+m, b3, b2, b1, b0);
            return 5; }
        mem_t storeData(mem_t m, uint32_t data) {
            byte b3, b2, b1, b0;
            Util::splitUInt32(data, b3, b2, b1, b0);
            storeMemory4(m, b3, b2, b1, b0);
            return 4; }
};


bool parse(std::string filename, ComputationState &cs) {
    bool dbg{false};

    std::ifstream is{filename};
    if (!is.is_open()) {
        std::cerr << "could not read input joy assembly file\n";
        return false; }

    typedef std::tuple<InstructionName, std::optional<std::string>> parsingInstruction;
    typedef uint32_t parsingData;
    std::vector<std::variant<parsingInstruction, parsingData>> parsing;
    std::map<std::string, std::string> definitions{};

    mem_t memPtr = 0;

    for (std::string ln; std::getline(is, ln); ) {
        ln = std::regex_replace(ln, std::regex{";.*"}, "");
        ln = std::regex_replace(ln, std::regex{"  +"}, " ");
        ln = std::regex_replace(ln, std::regex{"^ +"}, "");
        ln = std::regex_replace(ln, std::regex{" +$"}, "");

        if (ln == "")
            continue;

        auto define = [&definitions, dbg](std::string k, std::string v) {
            if (dbg)
                std::cout << "defining " << k << " to be " << v << "\n";
            if (definitions.contains(k)) {
                std::cerr << "duplicate definition: " << k << "\n";
                return false; }
            definitions[k] = v;
            return true; };

        {
            std::smatch _def;
            std::regex_search(ln, _def, std::regex{"^([^ ]+) +:= +([^ ]+)$"});
            if (_def.size() == 3) {
                if (!define(_def[1], _def[2]))
                    return false;
                continue; }
        }

        {
            std::smatch _label;
            std::regex_search(ln, _label, std::regex{"^([^ ]+):$"});
            if (_label.size() == 2) {
                if (!define("@" + std::string{_label[1]}, std::to_string(memPtr)))
                    return false;
                continue; }
        }


        {
            std::smatch _data;
            std::regex_search(ln, _data, std::regex{"^(u)?int *\\[(.+?)\\] *(.+?)$"});
            if (_data.size() == 4) {
                auto oSize = Util::stringToUInt32(_data[2]);
                if (!oSize.has_value() || oSize.value() <= 0) {
                    std::cerr << "invalid data size: " << _data[2] << "\n";
                    return false; }
                std::optional<uint32_t> oValue{std::nullopt};
                if (_data[1] == "u")
                    oValue = Util::stringToUInt32(_data[3]);
                else
                    oValue = static_cast<std::optional<uint32_t>>(Util::stringToInt32(_data[3]));
                if (!oValue.has_value()) {
                    std::cerr << "invalid data value: " << _data[3] << "\n";
                    return false; }
                for (uint32_t j = 0; j < oSize.value(); j++) {
                    parsing.push_back(parsingData{oValue.value()});
                    memPtr += 4;
                }
                continue;
            }
        }

        {
            std::smatch _string;
            std::regex_search(ln, _string, std::regex{"^string *\"(.*?)\"$"});
            if (_string.size() == 2) {
                std::vector<byte> bytes{};
                for (byte b : static_cast<std::string>(_string[1]))
                    bytes.push_back(b);
                while (!bytes.empty()) {
                    auto [unicode, readBytes] = Util::utf8_to_unicode(bytes);
                    while (readBytes--)
                        bytes.erase(bytes.begin());
                    if (dbg) {
                        std::printf("-> STRING CHAR 0x%08x (", unicode);
                        Util::put_utf8_char(unicode);
                        std::printf(")\n"); }
                    parsing.push_back(parsingData{unicode});
                    memPtr += 4;
                }
                parsing.push_back(parsingData{0x00000000});
                memPtr += 4;
                continue;
            }
        }

        {
            std::smatch _instr;
            std::regex_search(ln, _instr, std::regex{"^([^ ]+)( +([^ ]+))?$"});
            //std::cout << "instr: " << _instr[1] << ", " << _instr[3] << "\n";
            if (_instr.size() == 4) {
                auto oName = InstructionNameRepresentationHandler::from_string(_instr[1]);
                if (!oName.has_value()) {
                    std::cerr << "invalid instruction name: " << _instr[1] << "\n";
                    return false; }
                std::optional<std::string> oArg{std::nullopt};
                if (_instr[3] != "")
                    oArg = std::optional{_instr[3]};
                parsing.push_back(parsingInstruction{std::make_tuple(oName.value(), oArg)});
                memPtr += 5;
            }
            continue;
        }

        std::cerr << "incomprehensible: " << ln << "\n";
        return false;
    }

    auto memPtrMax{memPtr};
    memPtr = 0;

    for (auto p : parsing) {
        if (std::holds_alternative<parsingData>(p)) {
            uint32_t data{std::get<parsingData>(p)};
            if (dbg)
                std::printf("data value 0x%08x\n", data);
            memPtr += cs.storeData(memPtr, data);
        }
        else if (std::holds_alternative<parsingInstruction>(p)) {
            parsingInstruction instruction{std::get<parsingInstruction>(p)};
            InstructionName name = std::get<0>(instruction);
            auto oArg = std::get<1>(instruction);
            std::optional<uint32_t> oValue{std::nullopt};
            if (oArg.has_value()) {
                uint32_t value = 0;
                {
                    std::smatch _prgOffset;
                    std::regex_search(oArg.value(), _prgOffset, std::regex{"^prg\\+(.*?)$"});
                    if (_prgOffset.size() == 2) {
                        value += memPtrMax;
                        oArg = std::make_optional(_prgOffset[1]);
                    }
                }

                if (definitions.contains(oArg.value()))
                    oArg = std::make_optional(definitions[oArg.value()]);

                {
                    std::smatch _prgOffset;
                    std::regex_search(oArg.value(), _prgOffset, std::regex{"^prg\\+(.*?)$"});
                    if (_prgOffset.size() == 2) {
                        value += memPtrMax;
                        oArg = std::make_optional(_prgOffset[1]);
                    }
                }

                oValue = static_cast<std::optional<uint32_t>>(Util::stringToInt32(oArg.value()));
                if (!oValue.has_value()) {
                    std::cerr << "invalid value: " << oArg.value() << "\n";
                    return false; }
                value += oValue.value();

                oValue = std::make_optional(value);
            }



            auto [hasArgument, optionalValue] = InstructionNameRepresentationHandler
                                                ::argumentType[name];

            if (!hasArgument && oValue.has_value()) {
                std::cerr << "superfluous argument: " << InstructionNameRepresentationHandler::to_string(name) << " " << oValue.value() << "\n";
                return false; }
            if (hasArgument) {
                if (!optionalValue.has_value() && !oValue.has_value()) {
                    std::cerr << "requiring argument: " << InstructionNameRepresentationHandler::to_string(name) << "\n";
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
            std::getchar(); }
    }

    return true;
}


int main(int argc, char **argv) {
    if (argc < 2) {
        std::cerr << "please provide an input joy assembly file\n";
        return EXIT_FAILURE; }
    if (argc > 2 && std::string{argv[2]} == "visualize")
        DO_VISUALIZE_STEPS = true;
    if (argc > 2 && std::string{argv[2]} == "step")
        DO_VISUALIZE_STEPS = DO_WAIT_FOR_USER = true;

    ComputationState cs{};

    std::string filename{argv[1]};
    std::vector<byte> memory;
    if (!parse(filename, cs)) {
        std::cerr << "faulty joy assembly file\n";
        return EXIT_FAILURE; }

    do {
        if (DO_VISUALIZE_STEPS)
            cs.visualize();
        if (DO_WAIT_FOR_USER)
            getchar();
        else if (DO_VISUALIZE_STEPS)
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
    } while (cs.step());
}
