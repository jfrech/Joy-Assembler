// Jonathan Frech, August 2020

#include <bitset>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <regex>
#include <set>
#include <thread>
#include <variant>
#include <vector>

#include "Util.cpp"
#include "types.hh"

constexpr bool doLog{false};
constexpr inline void log(std::string const&msg) {
    if (doLog)
        std::clog << msg << std::endl; }

namespace InstructionNameRepresentationHandler {
    std::optional<InstructionName> fromByteCode(byte_t const opCode) {
        return instructions[opCode]; }

    byte_t toByteCode(InstructionName const name) {
        for (uint16_t opCode = 0; opCode < 0x100; opCode++) {
            if (!instructions[opCode].has_value())
                continue;
            if (instructions[opCode].value() == name)
                return static_cast<byte_t>(opCode); }
        return 0x00; }

    std::string to_string(InstructionName const name) {
        if (Util::std20::contains<InstructionName, std::string>(representation, name))
            return representation[name];
        return representation[InstructionName::NOP]; }

    std::optional<InstructionName> from_string(std::string const&repr) {
        for (auto const&[in, insr] : representation)
            if (insr == Util::to_upper(repr))
                return std::optional<InstructionName>{in};
        return std::nullopt; }
}

struct Instruction { InstructionName name; word_t argument; };

namespace InstructionRepresentationHandler {
    std::string to_string(Instruction const instruction) {
        return InstructionNameRepresentationHandler::to_string(instruction.name)
               + " " + Util::UInt32AsPaddedHex(instruction.argument); }
}

struct ComputationStateDebug {
    word_t highestUsedMemoryLocation{0};
    uint64_t executionCycles{0};
    bool doWaitForUser{false};
    bool doVisualizeSteps{false};
};

class ComputationState {
    private:
        word_t memorySize;
        std::vector<byte_t> memory;
        MemoryMode memoryMode;

        word_t registerA, registerB, registerPC, registerSC;

        bool flagAZero, flagANegative, flagAEven;

        bool mock;

    public:
        ComputationStateDebug debug;

    public: ComputationState(word_t const memorySize) :
        memorySize{memorySize},
        memory{std::vector<byte_t>(memorySize)},
        memoryMode{MemoryMode::LittleEndian},

        registerA{0}, registerB{0}, registerPC{0}, registerSC{0},

        flagAZero{true}, flagANegative{false}, flagAEven{true},

        mock{false},

        debug{}
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
        debug.executionCycles++;

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
                else if (m <= debug.highestUsedMemoryLocation)
                    std::cout << Util::ANSI_COLORS::MEMORY_LOCATION_USED;
                std::printf(" %02X", memory[y *w+ x]);
                std::cout << Util::ANSI_COLORS::CLEAR;
                pc++;
            }

            if ((y+1)*w >= 0x100 && debug.highestUsedMemoryLocation+1 < (y+1)*w)
                break;
        }
        std::printf("\n");

        std::printf("=== CURRENT INSTRUCTION ===\n");
        byte_t opCode = loadMemory(static_cast<word_t>(registerPC));
        std::string opCodeName = "(err. NOP)";
        auto oInstructionName = InstructionNameRepresentationHandler
                                ::fromByteCode(opCode);
        if (oInstructionName.has_value())
            opCodeName = InstructionNameRepresentationHandler
                         ::to_string(oInstructionName.value());
        word_t argument{loadMemory4(static_cast<word_t>(registerPC) + 1)};
        std::cout << "    " << Util::ANSI_COLORS::paint(Util::ANSI_COLORS::INSTRUCTION_NAME, opCodeName) << " " << Util::ANSI_COLORS::paint(Util::ANSI_COLORS::INSTRUCTION_ARGUMENT, Util::UInt32AsPaddedHex(argument)) << "\n";

        std::printf("=== REGISTERS ===\n");
        std::printf("    A:  0x%08x,    B:  0x%08x,\n    PC: 0x%08x,"
                    "    SC: 0x%08x\n"
                   , registerA, registerB, registerPC, registerSC);

        std::printf("=== FLAGS ===\n");
        std::printf("    flagAZero: %d,    flagANegative: %d,\n    "
                    "flagAEven: %d\n", flagAZero, flagANegative
                   , flagAEven);

        std::cout << "\n";
        printExecutionCycles();
    }

    void printExecutionCycles() {
        std::cout << "Execution cycles: " << debug.executionCycles << "\n"; }

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

    /* TODO throw on invalid memory location ~> machine should halt */
    byte_t loadMemory(word_t const m) {
        debug.highestUsedMemoryLocation = std::max(
            debug.highestUsedMemoryLocation, m);
        return m < memorySize ? memory[m] : 0;
    }
    /* TODO throw on invalid memory location ~> machine should halt */
    void storeMemory(word_t const m, byte_t const b) {
        debug.highestUsedMemoryLocation = std::max(
            debug.highestUsedMemoryLocation, m);
        if (m < memorySize)
            memory[m] = b;
    }

    word_t loadMemory4(word_t const m) {
        byte_t b3, b2, b1, b0;
        switch (memoryMode) {
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

    void storeMemory4(word_t const m, word_t const w) {
        byte_t b3 = static_cast<byte_t>((w >> 24) & 0xff);
        byte_t b2 = static_cast<byte_t>((w >> 16) & 0xff);
        byte_t b1 = static_cast<byte_t>((w >>  8) & 0xff);
        byte_t b0 = static_cast<byte_t>( w        & 0xff);
        switch (memoryMode) {
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

    public: word_t storeInstruction(word_t const m, Instruction const instruction) {
        storeMemory(m, InstructionNameRepresentationHandler
                       ::toByteCode(instruction.name));
        storeMemory4(1+m, instruction.argument);
        debug.highestUsedMemoryLocation = 0;
        return 5;
    }

    public: word_t storeData(word_t const m, word_t const data) {
        storeMemory4(m, data);
        debug.highestUsedMemoryLocation = 0;
        return 4;
    }
};

typedef uint64_t line_number_t;

typedef std::tuple<InstructionName, std::optional<std::string>> parsingInstruction;
typedef word_t parsingData;

struct ParsingState {
    std::filesystem::path filepath;
    std::set<std::filesystem::path> parsedFilepaths;
    std::vector<std::tuple<std::filesystem::path, line_number_t
                          , std::variant<parsingInstruction, parsingData>>>
        parsing;
    std::map<std::string, std::string> definitions;

    bool error(line_number_t const lineNumber, std::string const&msg) {
        std::cerr << "file " << filepath << ", ln " << lineNumber << ": " << msg << std::endl;
        return false; }
};

bool parse1(ParsingState &ps) {
    auto const&filepath = ps.filepath;
    auto &parsedFilepaths = ps.parsedFilepaths;
    auto &parsing = ps.parsing;

    if (Util::std20::contains(parsedFilepaths, filepath))
        return ps.error(0, "recursive file inclusion; not parsing file twice");
    parsedFilepaths.insert(filepath);

    std::ifstream is{filepath};
    if (!is.is_open())
        return ps.error(0, "unable to read file");

    for (auto [memPtr, lineNumber, ln] = std::make_tuple<word_t, line_number_t, std::string>(0, 1, ""); std::getline(is, ln); lineNumber++) {
        ln = std::regex_replace(ln, std::regex{";.*"}, "");
        ln = std::regex_replace(ln, std::regex{"\\s+"}, " "); /* TODO refactor REGEXs from here */
        ln = std::regex_replace(ln, std::regex{"^\\s+"}, "");
        ln = std::regex_replace(ln, std::regex{"\\s+$"}, "");

        if (ln == "")
            continue;

        log("ln " + std::to_string(lineNumber) + ": " + ln);

        auto define = [&ps, lineNumber]
            (std::string k, std::string v)
        {
            log("defining " + k + " to be " + v);
            if (Util::std20::contains<std::string, std::string>(ps.definitions, k))
                return ps.error(lineNumber, "duplicate definition: " + k);
            ps.definitions[k] = v;
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
                    if (_item.size() != 4)
                        return ps.error(lineNumber, "invalid data: " + data);
                    std::string item{_item[1]};
                    if (item[0] == '"') {
                        std::optional<std::vector<rune_t>> oRunes = Util::parseString(item);
                        if (!oRunes.has_value())
                            return ps.error(lineNumber, "invalid data: " + data);
                        for (rune_t rune : oRunes.value()) {
                            parsing.push_back(std::make_tuple(filepath, lineNumber,
                                parsingData{static_cast<word_t>(rune)}));
                            memPtr += 4; }
                        data = _item[3];
                        continue; }

                    {
                        std::smatch _match;
                        if (std::regex_match(item, _match, std::regex{"\\[(.+)\\]\\s*(.+)"})) {
                            auto oSize = Util::stringToUInt32(_match[1]);
                            if (!oSize.has_value())
                                return ps.error(lineNumber, "invalid data size: " + item + ", i.e. " + std::string{_match[1]});
                            auto oValue = Util::stringToUInt32(_match[2]);
                            if (!oValue.has_value())
                                return ps.error(lineNumber, "invalid data value: " + item + ", i.e. " + std::string{_match[2]});
                            for (std::size_t j = 0; j < oSize.value(); j++) {
                                parsing.push_back(std::make_tuple(filepath, lineNumber,
                                    parsingData{oValue.value()}));
                                memPtr += 4; }
                            data = _item[3];
                            continue; }
                    }

                    auto oValue = Util::stringToUInt32(item);
                    if (!oValue.has_value())
                        return ps.error(lineNumber, "invalid data value: " + item);
                    parsing.push_back(std::make_tuple(filepath, lineNumber,
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
                if (!oSize.has_value() || oSize.value() <= 0)
                    return ps.error(lineNumber, "invalid int data size: " + std::string{_int[1]});
                std::optional<word_t> oValue{Util::stringToUInt32(_int[2])};
                if (!oValue.has_value())
                    return ps.error(lineNumber, "invalid int data value: " + std::string{_int[2]});
                for (std::size_t j = 0; j < oSize.value(); j++) {
                    parsing.push_back(std::make_tuple(filepath, lineNumber,
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
                if (!oName.has_value())
                    return ps.error(lineNumber, "invalid instruction name: " + std::string{_instr[1]});
                std::optional<std::string> oArg{std::nullopt};
                if (_instr[3] != "")
                    oArg = std::optional{_instr[3]};
                parsing.push_back(std::make_tuple(filepath, lineNumber,
                    parsingInstruction{std::make_tuple(oName.value(), oArg)}));
                memPtr += 5;
                continue;
            }
        }

        return ps.error(lineNumber, "incomprehensible");
    }

    return true;
}

bool parse2(ParsingState &ps, ComputationState &cs) {
    word_t memPtr{0};
    for (auto [filepath, lineNumber, p] : ps.parsing) {
        if (std::holds_alternative<parsingData>(p)) {
            word_t data{std::get<parsingData>(p)};
            log("data value " + Util::UInt32AsPaddedHex(data));
            memPtr += cs.storeData(memPtr, data);
        }
        else if (std::holds_alternative<parsingInstruction>(p)) {
            parsingInstruction instruction{std::get<parsingInstruction>(p)};
            InstructionName name = std::get<0>(instruction);
            auto oArg = std::get<1>(instruction);
            std::optional<word_t> oValue{std::nullopt};
            if (oArg.has_value()) {
                word_t value = 0;
                if (Util::std20::contains(ps.definitions, oArg.value()))
                    oArg = std::make_optional(ps.definitions.at(oArg.value()));

                oValue = static_cast<std::optional<word_t>>(Util::stringToUInt32(oArg.value()));
                if (!oValue.has_value())
                    return ps.error(lineNumber, "invalid value: " + oArg.value());
                value += oValue.value();

                oValue = std::make_optional(value);
            }

            auto [hasArgument, optionalValue] = InstructionNameRepresentationHandler
                                                ::argumentType[name];

            if (!hasArgument && oValue.has_value())
                return ps.error(lineNumber, "superfluous argument: " + InstructionNameRepresentationHandler::to_string(name) + " " + std::to_string(oValue.value()));
            if (hasArgument) {
                if (!optionalValue.has_value() && !oValue.has_value())
                    return ps.error(lineNumber, "requiring argument: " + InstructionNameRepresentationHandler::to_string(name));
                if (!oValue.has_value())
                    oValue = optionalValue;
            }

            if (oValue.has_value()) {
                log("instruction " + InstructionNameRepresentationHandler::to_string(name) + " " + std::to_string(oValue.value()));
                auto argument = oValue.value();
                memPtr += cs.storeInstruction(memPtr, Instruction{name, argument});
            } else {
                log("instruction " + InstructionNameRepresentationHandler::to_string(name) + " (none)");
                memPtr += cs.storeInstruction(memPtr, Instruction{name, 0x00000000});
            }
        }
    }

    return true;
}


int main(int const argc, char const*argv[]) {
    if (argc < 2) {
        std::cerr << "please provide an input joy assembly file\n";
        return EXIT_FAILURE; }
    ComputationState cs{0x10000};

    if (argc > 2 && std::string{argv[2]} == "visualize")
        cs.debug.doVisualizeSteps = true;
    if (argc > 2 && std::string{argv[2]} == "step")
        cs.debug.doVisualizeSteps = cs.debug.doWaitForUser = true;

    std::filesystem::path filepath{std::filesystem::current_path() / std::filesystem::path(argv[1])};


    std::set<std::filesystem::path> parsedFilepaths{};
    ParsingState ps{};
    ps.filepath = filepath;
    if (!parse1(ps)) {
        std::cerr << "parsing failed at stage 1: " << filepath << std::endl;
        return EXIT_FAILURE; }

    if (!parse2(ps, cs)) {
        std::cerr << "parsing failed at stage 2: " << filepath << std::endl;
        return EXIT_FAILURE; }

    if (argc > 2 && std::string{argv[2]} == "memory-dump") {
        do {
            cs.memoryDump();
        } while (cs.step());
        cs.memoryDump();
        return EXIT_SUCCESS; }

    do {
        if (cs.debug.doVisualizeSteps)
            cs.visualize();
        if (cs.debug.doWaitForUser)
            std::cin.get();
        else if (cs.debug.doVisualizeSteps)
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
    } while (cs.step());

    if (argc > 2 && std::string{argv[2]} == "cycles")
        cs.printExecutionCycles();

}
