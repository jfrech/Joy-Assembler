// Jonathan Frech, 22nd, 23rd of August 2020

#include <fstream>
#include <iostream>
#include <regex>
#include <vector>


enum class MemoryMode { LittleEndian, BigEndian };

size_t MEMORY_SIZE = 0x10000;
MemoryMode MEMORY_MODE = MemoryMode::LittleEndian;

bool DO_WAIT_FOR_USER = false;
bool DO_VISUALIZE_STEPS = false;

enum class Register { RegisterA, RegisterB };
typedef uint8_t byte;
typedef uint32_t ProgramCounter;

typedef int32_t reg_t;
typedef uint32_t ureg_t;
typedef uint32_t arg_t;
typedef uint32_t mem_t;

enum class InstructionName {
    NOP,
    LDA, LDB, STA, STB, MOV,
    JMP, JNZ, JZ, JNN, JNG,
    IMM, INC, DEC, INV, SHL, SHR,
    SWP, ADD, SUB, AND, OR,
    PUT };

struct Instruction { InstructionName name; arg_t argument; };


namespace InstructionNameStringHandler {
    std::map<InstructionName, std::string> representation = {
        #define R(ACT) {InstructionName::ACT, #ACT}
        R(NOP), R(LDA), R(LDB), R(STA), R(STB), R(MOV), R(JMP), R(JNZ), R(JZ),
        R(JNN), R(JNG), R(IMM), R(INC), R(DEC), R(INV), R(SHL), R(SHR), R(SWP),
        R(ADD), R(SUB), R(AND), R(OR), R(PUT)
        #undef R
    };

    std::string to_string(InstructionName name) {
        if (representation.contains(name))
            return representation[name];
        return representation[InstructionName::NOP]; }

    std::string to_upper(std::string _str) {
        std::string str{_str};
        for (auto &c : str)
            c = std::toupper(c);
        return str; }

    InstructionName from_string(std::string repr) {
        for (auto const&[in, insr] : representation)
            if (insr == to_upper(repr))
                return in;
        return InstructionName::NOP; }
}

namespace InstructionStringHandler {
    std::string to_string(Instruction instruction) {
        char buffer[11];
        std::snprintf(buffer, 11, "0x%08x", instruction.argument);
        return InstructionNameStringHandler::to_string(instruction.name) + " "
               + std::string{buffer}; }
}


class ComputationState {
    public:

    ComputationState(std::vector<Instruction> program) :
        memory{std::vector<byte>(MEMORY_SIZE)},
        programCounter{0},
        registerA{0}, registerB{0},
        flagAZero{true}, flagANonNegative{true},
        program{program},
        highestUsedMemoryLocation{0}
    { ; }

    bool step() {
        if (programCounter >= program.size())
            return false;

        auto instruction = program[programCounter++];
        auto jmp = [&](bool cnd) {
            if (cnd)
                programCounter = ProgramCounter{instruction.argument}; };

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
            case InstructionName::IMM:
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
            case InstructionName::MOV: {
                mem_t m = mem_t{instruction.argument};
                m = m > 0 ? m : 1;
                for (mem_t n = 0; n < m; n++)
                    storeMemory(n + static_cast<mem_t>(registerB)
                               , loadMemory(n + static_cast<mem_t>(registerA)));
            }; break;

            case InstructionName::PUT: std::cout << registerA << "\n"; break;

            case InstructionName::NOP: break;

            default:
                std::cerr << "unknown instruction: "
                          << InstructionStringHandler
                             ::to_string(instruction) << "\n";
                return false;
        }

        updateFlags();
        return true;
    }

    bool didComputationFinish() {
        return programCounter >= program.size(); }

    void visualize() {
        std::printf("\n\n\n");
        std::printf("\n=== PROGRAM ===\n");
        for (ProgramCounter pc = 0; pc < program.size(); pc++) {
            if (programCounter == pc)
                std::printf("    > ");
            else
                std::printf("    . ");
            std::cout << InstructionStringHandler::to_string(program[pc]);
            std::printf("\n"); }
        if (programCounter >= program.size())
            std::printf("    > program ended\n");

        std::printf("\n=== MEMORY ===\n");
        size_t h = 16, w = 16;
        std::printf("       ");
        for (size_t x = 0; x < w; x++)
            std::printf("_%01X ", (int) x);
        for (size_t y = 0; y < h; y++) {
            std::printf("\n    %01X_", (int) y);
            for (size_t x = 0; x < w; x++) {
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
    ProgramCounter programCounter;
    reg_t registerA, registerB;
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
    ProgramCounter pc = 0;
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
            pc++; }
    }

    for (auto na : preParsed) {
        InstructionName name = InstructionNameStringHandler
                               ::from_string(std::get<0>(na));
        arg_t argument;

        std::string preArg = std::get<1>(na);
        if (definitions.contains(preArg))
            preArg = definitions[preArg];
        try {
            argument = std::stol(preArg, nullptr, 0); }
        catch (std::invalid_argument const&_) {
            std::cout << "invalid value: " << preArg << "\n";
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
        size_t memorySize;
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

    ComputationState cs{instructions};

    do {
        if (DO_VISUALIZE_STEPS)
            cs.visualize();
        if (DO_WAIT_FOR_USER)
            getchar();
    } while (cs.step());
    if (DO_VISUALIZE_STEPS)
        cs.visualize();
}
