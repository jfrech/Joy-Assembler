// Jonathan Frech, August 2020

#include <bitset>
#include <fstream>
#include <iostream>
#include <regex>
#include <vector>


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
    M(NOP, NO_ARG), \
    \
    M(LDA, REQ_ARG), M(LDB, REQ_ARG), M(STA, REQ_ARG), M(STB, REQ_ARG), \
    M(LIA, OPT_ARG(0)), M(SIA, OPT_ARG(0)), M(LPC, NO_ARG), M(SPC, NO_ARG), \
    \
    M(JMP, REQ_ARG), M(JNZ, REQ_ARG), M(JZ, REQ_ARG), M(JNN, REQ_ARG), \
    M(JN, REQ_ARG), M(JE, REQ_ARG), M(JNE, REQ_ARG), \
    \
    M(CAL, REQ_ARG), M(RET, NO_ARG), M(PSH, REQ_ARG), M(POP, REQ_ARG), \
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

    std::string to_upper(std::string _str) {
        std::string str{_str};
        for (auto &c : str)
            c = std::toupper(c);
        return str; }

    std::optional<InstructionName> from_string(std::string repr) {
        for (auto const&[in, insr] : representation)
            if (insr == to_upper(repr))
                return std::optional<InstructionName>{in};
        return std::nullopt; }
}
#undef MAP_ON_INSTRUCTION_NAMES

struct Instruction { InstructionName name; arg_t argument; };

namespace InstructionRepresentationHandler {
    std::string to_string(Instruction instruction) {
        char buffer[11];
        std::snprintf(buffer, 11, "0x%08x", instruction.argument);
        return InstructionNameRepresentationHandler
               ::to_string(instruction.name) + " " + std::string{buffer}; }
}

namespace Util {
    template<typename T>
    void swap_values(T &x, T &y) {
        auto tmp = x;
        x = y;
        y = tmp; }

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

    /* TODO implement utf-8 */
    void put_utf8_char(uint32_t unicode) {
        put_byte(0b0111'1111 & unicode);
    }

    /* TODO implement utf-8 */
    uint32_t get_utf8_char() {
        return 0b0111'1111 & get_byte();
    }
}



class ComputationState {
    private:

    std::vector<byte> memory;
    reg_t registerA, registerB;
    uint32_t registerPC;

    bool flagAZero, flagANegative, flagAParityEven;
    std::vector<Instruction> program;

    mem_t debugProgramTextSize;
    mem_t debugHighestUsedMemoryLocation;

    public:

    ComputationState() :
        memory{std::vector<byte>(MEMORY_SIZE)},
        registerA{0}, registerB{0}, registerPC{0},

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

    void loadInstructions(std::vector<Instruction> instructions) {
        programCounter_t pc = 0;
        for (auto i : instructions) {
            storeMemory(static_cast<mem_t>(pc++)
                       , InstructionNameRepresentationHandler
                         ::toByteCode(i.name));
            byte arg3, arg2, arg1, arg0;
            Util::splitUInt32(i.argument, arg3, arg2, arg1, arg0);
            storeMemory4(static_cast<mem_t>((pc += 4) - 4)
                        , arg3, arg2, arg1, arg0);
            debugProgramTextSize += 5;
        }
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
                storeMemory4(static_cast<uint32_t>(registerB), b3, b2, b1, b0);
                registerPC = instruction.argument;
            }; break;
            case InstructionName::RET: {
                byte b3, b2, b1, b0;
                loadMemory4(static_cast<mem_t>(registerB), b3, b2, b1, b0);
                registerPC = Util::combineUInt32(b3, b2, b1, b0);
            }; break;
            case InstructionName::PSH: {
                byte b3, b2, b1, b0;
                loadMemory4(static_cast<mem_t>(instruction.argument)
                           , b3, b2, b1, b0);
                mem_t stack = Util::combineUInt32(b3, b2, b1, b0) + 4;

                Util::splitUInt32(static_cast<uint32_t>(registerA)
                                 , b3, b2, b1, b0);
                storeMemory4(stack, b3, b2, b1, b0);

                Util::splitUInt32(stack, b3, b2, b1, b0);
                storeMemory4(static_cast<mem_t>(instruction.argument)
                            , b3, b2, b1, b0);
            }; break;
            case InstructionName::POP: {
                byte b3, b2, b1, b0;
                loadMemory4(static_cast<mem_t>(instruction.argument)
                           , b3, b2, b1, b0);
                mem_t stack = Util::combineUInt32(b3, b2, b1, b0);

                loadMemory4(stack, b3, b2, b1, b0);
                registerA = Util::combineUInt32(b3, b2, b1, b0);

                Util::splitUInt32(stack - 4, b3, b2, b1, b0);
                storeMemory4(static_cast<mem_t>(instruction.argument)
                            , b3, b2, b1, b0);
            }; break;

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
        std::size_t h = 16*3 + 8, w = 16;
        std::printf("       ");
        for (std::size_t x = 0; x < w; x++)
            std::printf("_%01X ", (int) x);
        for (std::size_t y = 0; y < h; y++) {
            std::printf("\n    %02X_", (int) y);
            for (std::size_t x = 0; x < w; x++) {
                mem_t m = y *w+ x;
                if (m < debugProgramTextSize)
                    if (rPC <= pc && pc < rPC + 5)
                        if (pc == rPC)
                            std::cout << "\33[48;5;119m";
                        else
                            std::cout << "\33[48;5;121m";
                    else if ((pc / 5) % 2 == 0)
                        if (pc % 5 == 0)
                            std::cout << "\33[48;5;33m";
                        else
                            std::cout << "\33[48;5;104m";
                    else
                        if (pc % 5 == 0)
                            std::cout << "\33[48;5;205m";
                        else
                            std::cout << "\33[48;5;168m";
                else if (m <= debugHighestUsedMemoryLocation)
                    std::cout << "\33[1m";
                std::printf(" %02X", memory[y *w+ x]);
                std::cout << "\33[0m";
                pc++;
            }
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
        std::printf("    %s 0x%08X\n", opCodeName.c_str(), argument);

        std::printf("\n=== REGISTERS ===\n");
        std::printf("    A: 0x%08x,    B: 0x%08x,    PC: 0x%08x\n"
                   , registerA, registerB, registerPC);

        std::printf("\n=== FLAGS ===\n");
        std::printf("    flagAZero: %d,    flagANegative: %d,    "
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

    void storeMemory(mem_t m, byte b) {
        debugHighestUsedMemoryLocation = std::max(
            debugHighestUsedMemoryLocation, m);
        if (m < MEMORY_SIZE)
            memory[m] = b; }

    void loadMemory4(mem_t m, byte &b3, byte &b2, byte &b1, byte &b0) {
        if (MEMORY_MODE == MemoryMode::LittleEndian) {
            b3 = loadMemory(m+3); b2 = loadMemory(m+2);
            b1 = loadMemory(m+1); b0 = loadMemory(m+0); }
        if (MEMORY_MODE == MemoryMode::BigEndian) {
            b3 = loadMemory(m+0); b2 = loadMemory(m+1);
            b1 = loadMemory(m+2); b0 = loadMemory(m+3); }}
    void storeMemory4(mem_t m, byte b3, byte b2, byte b1, byte b0) {
        if (MEMORY_MODE == MemoryMode::LittleEndian) {
            storeMemory(m+3, b3); storeMemory(m+2, b2);
            storeMemory(m+1, b1); storeMemory(m+0, b0); }
        if (MEMORY_MODE == MemoryMode::BigEndian) {
            storeMemory(m+0, b3); storeMemory(m+1, b2);
            storeMemory(m+2, b1); storeMemory(m+3, b0); }}
};


bool parse(std::string filename, std::vector<Instruction> &instructions) {
    std::ifstream is{filename};
    if (!is.is_open()) {
        std::cerr << "could not read input joy assembly file\n";
        return false; }

    std::vector<std::tuple<std::string, std::optional<std::string>>> preParsed{};
    std::map<std::string, std::string> definitions{};
    programCounter_t pc = 0;
    for (std::string ln; std::getline(is, ln); ) {
        ln = std::regex_replace(ln, std::regex{";.*"}, "");
        ln = std::regex_replace(ln, std::regex{"  +"}, " ");
        ln = std::regex_replace(ln, std::regex{"^ +"}, "");
        ln = std::regex_replace(ln, std::regex{" +$"}, "");

        auto define = [&definitions](std::string k, std::string v) {
            if (definitions.contains(k)) {
                std::cerr << "duplicate definition: " << k << "\n";
                return false; }
            definitions[k] = v;
            return true; };

        std::smatch _def;
        std::regex_search(ln, _def, std::regex{"^([^ ]+) +:= +([^ ]+)$"});
        if (_def.size() == 3) {
            if (!define(_def[1], _def[2]))
                return false;
            continue; }

        std::smatch _label;
        std::regex_search(ln, _label, std::regex{"^([^ ]+):$"});
        if (_label.size() == 2) {
            if (!define("@" + std::string{_label[1]}, std::to_string(pc)))
                return false;
            continue; }

        std::smatch _instr, _arg;
        std::regex_search(ln, _instr, std::regex{"^([^ ]+)"});
        std::regex_search(ln, _arg, std::regex{"^[^ ]+ ([^ ]+)$"});
        if (_instr.size() == 2) {
            std::string instr{_instr[1]};
            std::optional<std::string> oArg{std::nullopt};
            if (_arg.size() == 2)
                oArg = std::optional{_arg[1]};
            preParsed.push_back(std::make_tuple(instr, oArg));
            pc += 5; }
    }

    for (auto na : preParsed) {
        std::optional oName = InstructionNameRepresentationHandler
                              ::from_string(std::get<0>(na));
        if (!oName.has_value()) {
            std::cout << "invalid instruction: " << std::get<0>(na) << "\n";
            return false; }
        InstructionName name = oName.value();

        auto oArg = std::get<1>(na);
        auto [hasArgument, optionalValue] = InstructionNameRepresentationHandler
                                            ::argumentType[name];
        if (!oArg.has_value()) {
            if (!hasArgument) {
                instructions.push_back(Instruction{name, 0});
                continue; }
            if (optionalValue.has_value()) {
                instructions.push_back(Instruction{
                    name, optionalValue.value()});
                continue; }
            std::cerr << "instruction requires argument: "
                      << InstructionNameRepresentationHandler::to_string(name)
                      << "\n";
            return false;
        }
        if (oArg.has_value() && !hasArgument) {
            std::cerr << "superfluous instruction argument: "
                      << InstructionNameRepresentationHandler::to_string(name)
                      << "\n";
            return false; }
        std::string preArg = oArg.value();


        arg_t argument = 0;

        std::smatch _offset;
        std::regex_search(preArg, _offset, std::regex{"^prg+(.*)$"});
        if (_offset.size() == 2) {
            argument += 5 * static_cast<arg_t>(preParsed.size());
            preArg = _offset[1]; }

        if (definitions.contains(preArg))
            preArg = definitions[preArg];

        std::regex_search(preArg, _offset, std::regex{"^prg+(.*)$"});
        if (_offset.size() == 2) {
            argument += 5 * static_cast<arg_t>(preParsed.size());
            preArg = _offset[1]; }

        try {
            argument += std::stol(preArg, nullptr, 0); }
        catch (std::invalid_argument const&_) {
            std::cerr << "invalid value: " << preArg << "\n";
            return false; }

        instructions.push_back(Instruction{name, argument});
    }

    if (definitions.contains("pragma_memory-mode")) {
        std::string pmm = definitions["pragma_memory-mode"];
        if (pmm == "little-endian")
            MEMORY_MODE = MemoryMode::LittleEndian;
        else if (pmm == "big-endian")
            MEMORY_MODE = MemoryMode::BigEndian;
        else {
            std::cerr << "unknown memory mode pragma: " << pmm << "\n";
            return false; }}
    if (definitions.contains("pragma_memory-size")) {
        std::string ms = definitions["pragma_memory-size"];
        std::size_t memorySize;
        try {
            memorySize = std::stol(ms, nullptr, 0); }
        catch (std::invalid_argument const&_) {
            std::cerr << "unknown memory size: " << ms << "\n";
            return false; }
        MEMORY_SIZE = memorySize; }

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

    std::string filename{argv[1]};
    std::vector<Instruction> instructions;
    if (!parse(filename, instructions)) {
        std::cerr << "faulty joy assembly file\n";
        return EXIT_FAILURE; }

    ComputationState cs{};
    cs.loadInstructions(instructions);

    do {
        if (DO_VISUALIZE_STEPS)
            cs.visualize();
        if (DO_WAIT_FOR_USER)
            getchar();
    } while (cs.step());
    if (DO_VISUALIZE_STEPS)
        cs.visualize();
}
