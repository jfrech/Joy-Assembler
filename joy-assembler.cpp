// Jonathan Frech, August 2020

#include <bitset>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <regex>
#include <set>
#include <variant>
#include <vector>

#include "Util.cpp"
#include "types.hh"
#include "log.cpp"

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
        if (Util::std20::contains(representation, name))
            return representation[name];
        return representation[InstructionName::NOP]; }

    std::optional<InstructionName> from_string(std::string const&repr) {
        for (auto const&[in, insr] : representation)
            if (insr == Util::to_upper(repr))
                return std::optional<InstructionName>{in};
        return std::nullopt; }

    bool isStackInstruction(InstructionName const name) {
        return Util::std20::contains(std::set<InstructionName>{
            InstructionName::CAL, InstructionName::RET, InstructionName::PSH,
            InstructionName::POP, InstructionName::LSA, InstructionName::SSA,
            InstructionName::LSC, InstructionName::SSC}, name); }
}

namespace InstructionRepresentationHandler {
    std::string to_string(Instruction const instruction) {
        return InstructionNameRepresentationHandler::to_string(instruction.name)
               + " " + Util::UInt32AsPaddedHex(instruction.argument); }
}

class ComputationState {
    private:
        word_t memorySize;
        std::vector<byte_t> memory;
        MemoryMode memoryMode;
        word_t registerA, registerB, registerPC, registerSC;
        bool flagAZero, flagANegative, flagAEven;
        bool mock;
        bool erroneous;
    public:
        ComputationStateDebug debug;

    public: ComputationState(word_t const memorySize) :
        memorySize{memorySize},
        memory{std::vector<byte_t>(memorySize)},
        memoryMode{MemoryMode::LittleEndian},
        registerA{0}, registerB{0}, registerPC{0}, registerSC{0},
        flagAZero{true}, flagANegative{false}, flagAEven{true},
        mock{false},
        erroneous{false},

        debug{}
    {
        updateFlags(); }

    public: void enableVisualization() {
        debug.doVisualizeSteps = true; }
    public: void enableStepping() {
        enableVisualization();
        debug.doWaitForUser = true; }
    public: void enableFinalCycles() {
        debug.doShowFinalCycles = true; }
    public: void finalCycles() {
        if (debug.doShowFinalCycles)
            printExecutionCycles(); }

    private: std::optional<Instruction> nextInstruction() {
        byte_t opCode{loadMemory(static_cast<word_t>(registerPC++))};
        word_t arg{loadMemory4(static_cast<word_t>((registerPC += 4) - 4))};

        auto oInstructionName = InstructionNameRepresentationHandler
                                ::fromByteCode(opCode);
        if (!oInstructionName.has_value())
            return std::nullopt;

        InstructionName name = oInstructionName.value();
        word_t argument = arg;

        return std::make_optional(Instruction{name, argument}); }

    private: bool err(std::string const&msg) {
        std::cerr << "ComputationState: " << msg << std::endl;
        return !(erroneous = true); }

    public: bool step() {
        std::optional<Instruction> oInstruction = nextInstruction();
        if (!oInstruction.has_value())
            return err("step: failed to fetch next instruction");
        Instruction instruction = oInstruction.value();

        auto jmp = [&](bool cnd) {
            if (cnd)
                registerPC = static_cast<word_t>(instruction.argument); };

        switch (instruction.name) {
            case InstructionName::NOP:
                break;

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
            case InstructionName::LPC:
                registerPC = registerA;
                break;
            case InstructionName::SPC:
                registerA = registerPC;
                break;
            case InstructionName::LYA:
                registerA = (registerA                        & 0xffffff00)
                          | (loadMemory(instruction.argument) & 0x000000ff);
                break;
            case InstructionName::SYA:
                storeMemory(instruction.argument, static_cast<byte_t>(
                    registerA & 0xff));
                break;

            case InstructionName::JMP: jmp(true); break;
            case InstructionName::JZ : jmp(flagAZero); break;
            case InstructionName::JNZ: jmp(!flagAZero); break;
            case InstructionName::JN : jmp(flagANegative); break;
            case InstructionName::JNN: jmp(!flagANegative); break;
            case InstructionName::JE : jmp(flagAEven); break;
            case InstructionName::JNE: jmp(!flagAEven); break;

            case InstructionName::CAL:
                storeMemory4Stack(registerSC, registerPC);
                registerSC += 4;
                registerPC = instruction.argument;
                break;
            case InstructionName::RET:
                registerSC -= 4;
                registerPC = loadMemory4Stack(registerSC);
                break;
            case InstructionName::PSH:
                storeMemory4Stack(registerSC, registerA);
                registerSC += 4;
                break;
            case InstructionName::POP:
                registerA = loadMemory4Stack(registerSC -= 4);
                break;
            case InstructionName::LSA:
                registerA = loadMemory4Stack(registerSC + instruction.argument);
                break;
            case InstructionName::SSA:
                storeMemory4Stack(registerSC + instruction.argument, registerA);
                break;
            case InstructionName::LSC:
                registerSC = registerA;
                break;
            case InstructionName::SSC:
                registerA = registerSC;
                break;

            case InstructionName::MOV:
                registerA = instruction.argument;
                break;
            case InstructionName::NOT:
                registerA = ~registerA;
                break;
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

            case InstructionName::SWP:
                std::swap(registerA, registerB);
                break;
            case InstructionName::AND:
                registerA &= registerB;
                break;
            case InstructionName::OR:
                registerA |= registerB;
                break;
            case InstructionName::XOR:
                registerA ^= registerB;
                break;
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
                UTF8IO::putRune(static_cast<UTF8::rune_t>(registerA));
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
                registerA = static_cast<word_t>(UTF8IO::getRune());
                break;

            case InstructionName::HLT: return false;
        }

        updateFlags();
        std::flush(std::cout);
        debug.executionCycles++;

        if (erroneous)
            return err("step: erroneous machine state");

        return true;
    }

    public: void visualize() {
        visualize(true); }
    public: void visualize(bool blockAllowed) {
        if (!debug.doVisualizeSteps)
            return;

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

        if (blockAllowed) {
            if (debug.doWaitForUser)
                UTF8IO::getRune();
            else if (debug.doVisualizeSteps)
                Util::IO::wait();
        }
    }

    private: void printExecutionCycles() {
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

    byte_t loadMemory(word_t const m) {
        debug.highestUsedMemoryLocation = std::max(
            debug.highestUsedMemoryLocation, m);
        if (/*m < 0 || */m >= memory.size()) {
            err("loadMemory: memory out of bounds");
            return 0; }
        return memory[m]; }

    void storeMemory(word_t const m, byte_t const b) {
        debug.highestUsedMemoryLocation = std::max(
            debug.highestUsedMemoryLocation, m);
        if (/*m < 0 || */m >= memory.size()) {
            err("storeMemory: memory out of bounds");
            return; }
        memory[m] = b; }

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

    /* TODO assert stack boundaries and aligment */
    word_t loadMemory4Stack(word_t const m) {
        return loadMemory4(m); }
    /* TODO assert stack boundaries and aligment */
    void storeMemory4Stack(word_t const m, word_t const w) {
        storeMemory4(m, w); }

    public: word_t storeInstruction(
            word_t const m, Instruction const instruction
    ) {
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

bool parse1(Parsing::ParsingState &ps) {
    auto const&filepath = ps.filepath;
    auto &parsedFilepaths = ps.parsedFilepaths;

    if (Util::std20::contains(parsedFilepaths, filepath))
        return ps.error(0, "recursive file inclusion; not parsing file twice");
    parsedFilepaths.insert(filepath);

    std::ifstream is{filepath};
    if (!is.is_open())
        return ps.error(0, "unable to read file");

    std::string const&regexIdentifier{"[._[:alpha:]-][._[:alnum:]-]*"};
    std::string const&regexValue{"[@._[:alnum:]-][._[:alnum:]-]*"};


    for (
        auto [memPtr, lineNumber, ln]
            = std::make_tuple<word_t, Parsing::line_number_t, std::string>(0, 1, "");
        std::getline(is, ln);
        lineNumber++
    ) {


        auto pushData = [&](uint32_t const data) {
            log("pushing data: " + Util::UInt32AsPaddedHex(data));
            ps.parsing.push_back(std::make_tuple(filepath, lineNumber,
                Parsing::parsingData{data}));
            memPtr += 4;
        };
        auto pushInstruction = [&](InstructionName const&name, std::optional<std::string> const&oArg) {
            log("pushing instruction: " + InstructionNameRepresentationHandler::to_string(name) + oArg.value_or("(no arg.)"));
            ps.parsing.push_back(std::make_tuple(filepath, lineNumber,
                Parsing::parsingInstruction{std::make_tuple(name, oArg)}));
            memPtr += 5;
        };



        ln = std::regex_replace(ln, std::regex{";.*$"}, "");
        ln = std::regex_replace(ln, std::regex{"\\s+"}, " ");
        ln = std::regex_replace(ln, std::regex{"^ +"}, "");
        ln = std::regex_replace(ln, std::regex{" +$"}, "");

        if (ln == "")
            continue;

        log("ln " + std::to_string(lineNumber) + ": " + ln);

        auto define = [&ps, lineNumber]
            (std::string k, std::string v)
        {
            log("defining " + k + " to be " + v);
            if (Util::std20::contains(ps.definitions, k))
                return ps.error(lineNumber, "duplicate definition: " + k);
            ps.definitions[k] = v;
            return true;
        };


        #define ON_MATCH(REGEX, LAMBDA) { \
            std::smatch smatch{}; \
            if (std::regex_match(ln, smatch, std::regex{(REGEX)})) { \
                if (!(LAMBDA)(smatch)) \
                    return false; \
                continue; \
            } \
        }

        ON_MATCH("^(" + regexIdentifier + ") := (" + regexValue + ")$", (
            [define](std::smatch const&smatch) {
                return define(std::string{smatch[1]}, std::string{smatch[2]}); }))

        ON_MATCH("^(" + regexIdentifier + "):$", (
            [define, &ps, &memPtr](std::smatch const&smatch) {
                std::string label{smatch[1]};
                if (!define("@" + label, std::to_string(memPtr)))
                    return false;
                if (label == "stack" && !ps.stackBeginning.has_value())
                    ps.stackBeginning = std::make_optional(memPtr);
                return true; }))

        ON_MATCH("^data ?(.+)$", (
            [pushData, lineNumber, regexValue, &ps, &memPtr](std::smatch const&smatch) {
                log("parsing `data` ...");

                std::string commaSeparated{std::string{smatch[1]} + ","};
                for (uint64_t elementNumber{1}; commaSeparated != ""; elementNumber++) {
                    std::smatch smatch{};
                    if (std::regex_match(commaSeparated, smatch, std::regex{"^(" + ("(\\[(" + regexValue + ")\\])? ?(" + regexValue + ")?") + "|" + (std::string{"\""} + "([^\"]|\\\")*?" + "\"") + ") ?, ?(.*)$"})) {

                        log("smatch of size " + std::to_string(smatch.size()));
                        for (auto j = 0u; j < smatch.size(); j++)
                            log("smatch[" + std::to_string(j) + "]: @@@" + std::string{smatch[j]} + "@@@");

                        std::string unparsedElement{smatch[1]};
                        std::string unparsedSize{smatch[3]};
                        std::string unparsedValue{smatch[4]};
                        commaSeparated = std::string{smatch[6]};

                        if (unparsedElement == "")
                            return ps.error(lineNumber, "invalid data element (element number " + std::to_string(elementNumber) + "): empty element");

                        if (unparsedElement.front() == '\"') {
                            log("parsing string: " + unparsedElement);
                            std::optional<std::vector<UTF8::rune_t>> oRunes = Util::parseString(unparsedElement);
                            if (!oRunes.has_value())
                                return ps.error(lineNumber, "invalid data string element (element number " + std::to_string(elementNumber) + "): " + unparsedElement);
                            for (UTF8::rune_t rune : oRunes.value())
                                pushData(static_cast<word_t>(rune));
                            continue;
                        }

                        log("parsing uint: " + unparsedElement);

                        if (unparsedSize == "")
                            unparsedSize = std::string{"1"};
                        std::optional<word_t> oSize{Util::stringToUInt32(unparsedSize)};
                        if (!oSize.has_value())
                            return ps.error(lineNumber, "invalid data uint element size (element number " + std::to_string(elementNumber) + "): " + unparsedSize);
                        log("    ~> size: " + std::to_string(oSize.value()));

                        if (unparsedValue == "")
                            unparsedValue = std::string{"0"};
                        std::optional<word_t> oValue{Util::stringToUInt32(unparsedValue)};
                        if (!oValue.has_value())
                            return ps.error(lineNumber, "invalid data uint element value (element number " + std::to_string(elementNumber) + "): " + unparsedValue);
                        log("    ~> value: " + std::to_string(oValue.value()));

                        for (word_t j = 0; j < oSize.value(); j++)
                            pushData(oValue.value());
                        continue;
                    }
                    return ps.error(lineNumber, "incomprehensible data element (element number " + std::to_string(elementNumber) + ")");
                }
                return true;
            }))


        ON_MATCH("^(" + regexIdentifier + ")( (" + regexValue + "))?$", (
            [pushInstruction, lineNumber, &ps, &memPtr](std::smatch const&smatch) {
                auto oName = InstructionNameRepresentationHandler::from_string(smatch[1]);
                if (!oName.has_value())
                    return ps.error(lineNumber, "invalid instruction name: " + std::string{smatch[1]});
                std::optional<std::string> oArg{std::nullopt};
                if (smatch[3] != "")
                    oArg = std::optional{smatch[3]};
                pushInstruction(oName.value(), oArg);
                return true; }))

        #undef ON_MATCH

        return ps.error(lineNumber, "incomprehensible");
    }

    return true;
}

bool parse2(Parsing::ParsingState &ps, ComputationState &cs) {

    bool memPtrGTStackBeginningAndNonDataOccurred{false};

    word_t memPtr{0};
    for (auto [filepath, lineNumber, p] : ps.parsing) {
        if (std::holds_alternative<Parsing::parsingData>(p)) {
            word_t data{std::get<Parsing::parsingData>(p)};
            log("data value " + Util::UInt32AsPaddedHex(data));
            memPtr += cs.storeData(memPtr, data);

            if (!memPtrGTStackBeginningAndNonDataOccurred)
                ps.stackEnd = std::make_optional(memPtr);
        }
        else if (std::holds_alternative<Parsing::parsingInstruction>(p)) {
            if (memPtr > ps.stackBeginning)
                memPtrGTStackBeginningAndNonDataOccurred = true;

            Parsing::parsingInstruction instruction{std::get<Parsing::parsingInstruction>(p)};
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

            ps.stackInstructionWasUsed = ps.stackInstructionWasUsed
                || InstructionNameRepresentationHandler
                   ::isStackInstruction(name);
        }
    }

    if (ps.stackInstructionWasUsed && !Util::std20::contains(ps.definitions, std::string{"@stack"}))
        return ps.error(0, "stack instructions are used yet no stack was defined");

    return true;
}


int main(int const argc, char const*argv[]) {
    if (argc < 2) {
        std::cerr << "please provide an input joy assembly file\n";
        return EXIT_FAILURE; }

    ComputationState cs{0x10000};

    if (argc > 2 && std::string{argv[2]} == "visualize")
        cs.enableVisualization();
    if (argc > 2 && std::string{argv[2]} == "step")
        cs.enableStepping();
    if (argc > 2 && std::string{argv[2]} == "cycles")
        cs.enableFinalCycles();

    Parsing::ParsingState ps{
        std::filesystem::current_path() / std::filesystem::path(argv[1])};
    if (!parse1(ps))
        return ps.error(0, "parsing failed at stage 1"), EXIT_FAILURE;
    if (!parse2(ps, cs))
        return ps.error(0, "parsing failed at stage 2"), EXIT_FAILURE;

    //TODO std::cerr << "got for stack: " << ps.stackBeginning.value() << " and " << ps.stackEnd.value() << std::endl;

    if (argc > 2 && std::string{argv[2]} == "memory-dump") {
        do cs.memoryDump(); while (cs.step()); cs.memoryDump();
        return EXIT_SUCCESS; }

    do cs.visualize(); while (cs.step()); cs.finalCycles();

    return EXIT_SUCCESS;
}
