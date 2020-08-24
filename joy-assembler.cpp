// Jonathan Frech, August 2020

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


#define MAP_ON_INSTRUCTION_NAMES(M) \
    M(NOP), \
    M(LDA), M(LDB), M(STA), M(STB), \
    M(JMP), M(JNZ), M(JZ), M(JNN), M(JNG), \
    M(MOV), M(INC), M(DEC), M(INV), M(SHL), M(SHR), M(SWP), \
    M(ADD), M(SUB), M(AND), M(OR), \
    M(PUT), \
    M(HLT)

enum class InstructionName {
    #define ID(ACT) ACT
    MAP_ON_INSTRUCTION_NAMES(ID)
    #undef ID
};

namespace InstructionNameRepresentationHandler {
    std::map<InstructionName, std::string> representation = {
        #define REPR(ACT) {InstructionName::ACT, #ACT}
        MAP_ON_INSTRUCTION_NAMES(REPR)
        #undef REPR
    };
    std::array<std::optional<InstructionName>, 256> instructions = {
        #define NAMESPACE_ID(ACT) InstructionName::ACT
        MAP_ON_INSTRUCTION_NAMES(NAMESPACE_ID)
        #undef NAMESPACE_ID
    };

    std::optional<InstructionName> fromByteCode(byte opCode) {
        return instructions[opCode]; }
    byte toByteCode(InstructionName name) {
        for (uint16_t opCode = 0; opCode < 0x100; opCode++)
            if (instructions[opCode].has_value() && instructions[opCode].value() == name)
                return static_cast<byte>(opCode);
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
        return InstructionNameRepresentationHandler::to_string(instruction.name) + " "
               + std::string{buffer}; }
}


void splitUInt32(uint32_t bytes, byte &b3, byte &b2, byte &b1, byte &b0) {
    b3 = static_cast<byte>((bytes >> 24) & 0xff);
    b2 = static_cast<byte>((bytes >> 16) & 0xff);
    b1 = static_cast<byte>((bytes >>  8) & 0xff);
    b0 = static_cast<byte>( bytes        & 0xff); }

class ComputationState {
    public:

    ComputationState() :
        memory{std::vector<byte>(MEMORY_SIZE)},
        registerA{0}, registerB{0}, registerPC{0},
        flagAZero{true}, flagANonNegative{true},
        highestUsedMemoryLocation{0}
    { ; }

    Instruction nextInstruction() {
        byte opCode = loadMemory(static_cast<mem_t>(registerPC++));
        byte arg3, arg2, arg1, arg0;
        loadMemory4(static_cast<mem_t>((registerPC += 4) - 4), arg3, arg2, arg1, arg0);

        auto oInstructionName = InstructionNameRepresentationHandler::fromByteCode(opCode);
        if (!oInstructionName.has_value())
            return Instruction{InstructionName::NOP, 0x00000000};

        InstructionName name = oInstructionName.value();
        arg_t argument = arg3 << 24 | arg2 << 16 | arg1 << 8 | arg0;

        //std::cout << "read instruction: " << InstructionRepresentationHandler::to_string(Instruction{name, argument});

        return Instruction{name, argument};
    }

    void loadInstructions(std::vector<Instruction> instructions) {
        programCounter_t pc = 0;
        for (auto i : instructions) {
            storeMemory(static_cast<mem_t>(pc++), InstructionNameRepresentationHandler::toByteCode(i.name));
            byte arg3, arg2, arg1, arg0;
            splitUInt32(i.argument, arg3, arg2, arg1, arg0);
            storeMemory4(static_cast<mem_t>((pc += 4) - 4), arg3, arg2, arg1, arg0);
        }
    }

    bool step() {
        auto instruction = nextInstruction();
        auto jmp = [&](bool cnd) {
            if (cnd)
                registerPC = programCounter_t{instruction.argument}; };

        switch (instruction.name) {
            case InstructionName::LDA: {
                    byte b3, b2, b1, b0;
                    loadMemory4(mem_t{instruction.argument}, b3, b2, b1, b0);
                    registerA = b3 << 24 | b2 << 16 | b1 << 8 | b0;
                }; break;
            case InstructionName::LDB: {
                    byte b3, b2, b1, b0;
                    loadMemory4(mem_t{instruction.argument}, b3, b2, b1, b0);
                    registerB = b3 << 24 | b2 << 16 | b1 << 8 | b0;
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
            case InstructionName::MOV:
                registerA = static_cast<reg_t>(instruction.argument); break;
            case InstructionName::SWP: {
                    auto tmp = registerA;
                    registerA = registerB;
                    registerB = tmp;
                    updateFlags();
                }; break;
            case InstructionName::JNZ: jmp(!flagAZero); break;
            case InstructionName::JZ: jmp(flagAZero); break;
            case InstructionName::JMP: jmp(true); break;
            case InstructionName::JNN: jmp(flagANonNegative); break;
            case InstructionName::JNG: jmp(!flagANonNegative); break;
            case InstructionName::AND: registerA &= registerB; break;
            case InstructionName::OR: registerA |= registerB; break;
            case InstructionName::INV: registerA = ~registerA; break;
            case InstructionName::SHL: {
                uint8_t shift = static_cast<uint8_t>(instruction.argument);
                registerA <<= shift > 0 ? shift : 1;
            }; break;
            case InstructionName::SHR: {
                uint8_t shift = static_cast<uint8_t>(instruction.argument);
                registerA >>= shift > 0 ? shift : 1;
            }; break;
            case InstructionName::ADD: registerA += registerB; break;
            case InstructionName::SUB: registerA -= registerB; break;
            case InstructionName::INC: registerA++; break;
            case InstructionName::DEC: registerA--; break;

            case InstructionName::PUT: std::cout << registerA << "\n"; break;

            case InstructionName::NOP: break;
            case InstructionName::HLT: return false;

            default:
                std::cerr << "unknown instruction: "
                          << InstructionRepresentationHandler
                             ::to_string(instruction) << "\n";
                return false;
        }

        updateFlags();
        return true;
    }

    void visualize() {
        std::printf("\n\n\n");
        /*
        std::printf("\n=== PROGRAM ===\n");
        for (programCounter_t pc = 0; pc < program.size(); pc++) {
            if (programCounter == pc)
                std::printf("    > ");
            else
                std::printf("    . ");
            std::cout << InstructionRepresentationHandler::to_string(program[pc]);
            std::printf("\n"); }
        if (programCounter >= program.size())
            std::printf("    > program ended\n");
        */

        std::printf("\n=== MEMORY ===\n");
        std::size_t h = 16, w = 16;
        std::printf("       ");
        for (std::size_t x = 0; x < w; x++)
            std::printf("_%01X ", (int) x);
        for (std::size_t y = 0; y < h; y++) {
            std::printf("\n    %01X_", (int) y);
            for (std::size_t x = 0; x < w; x++) {
                mem_t m = y *w+ x;
                if (m <= highestUsedMemoryLocation)
                    std::cout << "\33[1m";
                std::printf(" %02X", memory[y *w+ x]);
                if (m <= highestUsedMemoryLocation)
                    std::cout << "\33[0m"; }}
        std::printf("\n");

        std::printf("\n=== REGISTERS ===\n");
        std::printf("    A: %08X,    B: %08X\n", registerA, registerB);

        std::printf("\n=== FLAGS ===\n");
        std::printf("    flagAZero: %d,    flagANonNegative: %d\n"
                   , flagAZero, flagANonNegative);
    }


    private:

    std::vector<byte> memory;
    programCounter_t programCounter;
    reg_t registerA, registerB, registerPC;
    bool flagAZero, flagANonNegative;
    std::vector<Instruction> program;

    mem_t highestUsedMemoryLocation;

    void updateFlags() {
        flagAZero = registerA == 0;
        flagANonNegative = registerA >= 0; }

    byte loadMemory(mem_t m) {
        highestUsedMemoryLocation = std::max(highestUsedMemoryLocation, m);
        return m < MEMORY_SIZE ? memory[m] : 0; }

    void storeMemory(mem_t m, byte b) {
        highestUsedMemoryLocation = std::max(highestUsedMemoryLocation, m);
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

    std::vector<std::tuple<std::string, std::string>> preParsed{};
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
            std::string instr = _instr[1], arg = "0";
            if (_arg.size() == 2)
                arg = _arg[1];
            preParsed.push_back(std::make_tuple(instr, arg));
            pc += 5; }
    }

    for (auto na : preParsed) {
        std::optional oName = InstructionNameRepresentationHandler
                              ::from_string(std::get<0>(na));
        if (!oName.has_value()) {
            std::cout << "invalid instruction: " << std::get<0>(na) << "\n";
            return false; }
        InstructionName name = oName.value();
        arg_t argument = 0;

        std::string preArg = std::get<1>(na);

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
            std::cout << "invalid value: " << preArg << "\n";
            return false; }

        //std::cout << "parsed instruction: " << InstructionRepresentationHandler::to_string(Instruction{name, argument}) << "\n";

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
