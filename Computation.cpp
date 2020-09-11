#ifndef JOY_ASSEMBLER__COMPUTATION_CPP
#define JOY_ASSEMBLER__COMPUTATION_CPP

#include "Includes.hh"

struct Statistics {
    uint64_t nInstructions, nMicroInstructions;

    Statistics operator-(Statistics const&statistics) const {
        return Statistics{nInstructions - statistics.nInstructions, nMicroInstructions - statistics.nMicroInstructions}; }
    std::string toString() const {
        return "#" + std::to_string(nInstructions) + ": " + std::to_string(nMicroInstructions); }
};

class ComputationState {
    private:
        std::vector<byte_t> memory;
        bool const memoryIsDynamic;
        MemoryMode const memoryMode;
        word_t registerA, registerB, registerPC, registerSC;
        bool flagAZero, flagANegative, flagAEven;
        Util::rng_t rng;

        std::map<word_t, std::vector<std::tuple<bool, std::string>>>
            const profiler;
        Statistics statistics;
        std::stack<Statistics> profilerStatistics;
        bool embedProfilerOutput;
        std::optional<std::vector<MemorySemantic>> oMemorySemantics;

        bool mock;
        bool erroneous;
    public:
        ComputationStateDebug debug;

    public: ComputationState(
        word_t const memorySize,
        bool const memoryIsDynamic,
        MemoryMode const memoryMode,
        Util::rng_t const&rng,
        std::map<word_t, std::vector<std::tuple<bool, std::string>>> const&profiler,
        bool const embedProfilerOutput, std::optional<std::vector<MemorySemantic>> const&oMemorySemantics
    ) :
        memory{std::vector<byte_t>(memorySize, 0x00000000)},
        memoryIsDynamic{memoryIsDynamic},
        memoryMode{memoryMode},
        registerA{0}, registerB{0}, registerPC{0}, registerSC{0},
        flagAZero{true}, flagANegative{false}, flagAEven{true},

        rng{rng},

        profiler{profiler}, statistics{0, 0}, profilerStatistics{},
        embedProfilerOutput{embedProfilerOutput},
        oMemorySemantics{oMemorySemantics},

        mock{false}, erroneous{false},

        debug{}
    {
        updateFlags(); }

    /* since ComputationState::memory may be rather large, all copy
       constructors are deleted and the default move constructor is used */
    public: ComputationState(ComputationState &)      = delete;
    public: ComputationState(ComputationState const&) = delete;
    public: ComputationState(ComputationState &&)     = default;

    public: void visualize() {
        visualize(true); }

    public: void visualize(bool blockAllowed) {
        if (!debug.doVisualizeSteps)
            return;

        auto paintFaint = Util::ANSI_COLORS::paintFactory(Util::ANSI_COLORS::FAINT);
        auto paintRegister = Util::ANSI_COLORS::paintFactory(Util::ANSI_COLORS::REGISTER);

        std::cout << "\n    ====================- MEMORY -=====================\n";
        word_t pc = 0, rPC = registerPC;
        std::size_t w = 16;
        std::printf("       ");
        for (std::size_t x = 0; x < w; ++x)
            std::cout << (paintFaint("_" + Util::UNibbleAsPaddedHex(x & 0xf) + " "));
        for (std::size_t y = 0; true; ++y) {
            std::cout << ("\n    " + paintFaint(Util::UInt8AsPaddedHex(y & 0xff) + "_"));
            for (std::size_t x = 0; x < w; ++x) {
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
                ++pc;
            }

            if ((y+1)*w >= 0x100 && debug.highestUsedMemoryLocation+1 < (y+1)*w)
                break;
        }
        std::cout << "\n    Current instruction: ";

        byte_t opCode = loadMemory(std::nullopt, registerPC);
        std::string opCodeName{"(err. NOP)"};
        auto oInstructionName = InstructionNameRepresentationHandler
                                ::fromByteCode(opCode);
        if (oInstructionName.has_value())
            opCodeName = InstructionNameRepresentationHandler
                         ::to_string(oInstructionName.value());
        word_t argument{loadMemory4(false, registerPC+1)};
        std::cout << (Util::ANSI_COLORS::paint(Util::ANSI_COLORS::INSTRUCTION_NAME, opCodeName) + " " + Util::ANSI_COLORS::paint(Util::ANSI_COLORS::INSTRUCTION_ARGUMENT, "0x" + Util::UInt32AsPaddedHex(argument)) + " (#" + std::to_string(statistics.nInstructions) + ": " + std::to_string(statistics.nMicroInstructions) + ")\n");

        std::cout << ("    Registers:    " + std::string{"A:  0x"} + paintRegister(Util::UInt32AsPaddedHex(registerA))  + ",     B:  0x" + paintRegister(Util::UInt32AsPaddedHex(registerB))  + "\n");
        std::cout << ("                  " + std::string{"PC: 0x"} + paintRegister(Util::UInt32AsPaddedHex(registerPC)) + ",     SC: 0x" + paintRegister(Util::UInt32AsPaddedHex(registerSC)) + "\n");
        std::cout << ("    Flags (A zero, A negative, A even): " + paintRegister(Util::UBitAsPaddedHex(flagAZero)) + paintRegister(Util::UBitAsPaddedHex(flagANegative)) + paintRegister(Util::UBitAsPaddedHex(flagAEven)) + "\n");

        std::cout << ("    % ");

        if (blockAllowed) {
            if (debug.doWaitForUser)
                UTF8IO::getRune();
            else if (debug.doVisualizeSteps)
                Util::IO::wait(); }
    }

    public: void memoryDump() {
        mock = true;

        std::string dump{""};
        dump += "A: 0x" + Util::UInt32AsPaddedHex(registerA);
        dump += ", B: 0x" + Util::UInt32AsPaddedHex(registerB);
        dump += ", PC: 0x" + Util::UInt32AsPaddedHex(registerPC);
        dump += ", SC: 0x" + Util::UInt32AsPaddedHex(registerSC);
        dump += "; memory (" + std::to_string(memory.size()) + "B):";
        word_t mx = memory.size();
        while (--mx != 0 && memory[mx] == 0)
            ;
        ++mx;
        for (word_t m = 0; m < mx; ++m)
            dump += " " + Util::UInt8AsPaddedHex(memory[m]);

        std::cout << dump << "\n";
    }

    private: void checkProfiler() {
        if (!Util::std20::contains(profiler, registerPC))
            return;

        for (auto [start, msg] : profiler.at(registerPC)) {
            std::string const str{"[# " + std::to_string(statistics.nInstructions) + ": " + std::to_string(statistics.nMicroInstructions) + "] "};
            std::string const strPad{std::string(str.size(), ' ')};

            if (start) {
                if (!embedProfilerOutput)
                    std::clog << str << "starting profiler: " + msg << std::endl;
                profilerStatistics.push(statistics);
            } else {
                if (!embedProfilerOutput)
                    std::clog << str << "stopping profiler: " + msg << std::endl;
                if (profilerStatistics.empty())
                    std::clog << str << "profiler could not be stopped since it was not started" << std::endl;
                else {
                    Statistics start{profilerStatistics.top()}, stop{statistics};
                    profilerStatistics.pop();
                    if (start.nInstructions > stop.nInstructions || start.nMicroInstructions > stop.nMicroInstructions)
                        std::clog << strPad << "profiler time travelled" << std::endl;
                    else {
                        Statistics const elapsed{stop - start};
                        if (!embedProfilerOutput)
                            std::clog << strPad << "-> number of elapsed instructions: " + elapsed.toString() << std::endl;
                        else
                            std::cout << std::to_string(elapsed.nMicroInstructions) << "\n";
                    }
                }
            }
        }
    }

    public: bool step() {
        checkProfiler();

        std::optional<Instruction> oInstruction = nextInstruction();
        if (!oInstruction.has_value())
            return err("step: failed to fetch next instruction");
        Instruction instruction = oInstruction.value();

        ++statistics.nInstructions;
        statistics.nMicroInstructions += InstructionRepresentationHandler::microInstructions(instruction);

        auto jmp = [&](bool const cnd) {
            if (cnd)
                registerPC = instruction.argument; };

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

            case InstructionName::JMP:
                jmp(true);
                break;
            case InstructionName::JN:
                jmp(flagANegative);
                break;
            case InstructionName::JNN:
                jmp(!flagANegative);
                break;
            case InstructionName::JZ:
                jmp(flagAZero);
                break;
            case InstructionName::JNZ:
                jmp(!flagAZero);
                break;
            case InstructionName::JP:
                jmp(!flagANegative && !flagAZero);
                break;
            case InstructionName::JNP:
                jmp(flagANegative || flagAZero);
                break;
            case InstructionName::JE:
                jmp(flagAEven);
                break;
            case InstructionName::JNE:
                jmp(!flagAEven);
                break;

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
                registerA <<= instruction.argument;
                break;
            case InstructionName::SHR:
                registerA >>= instruction.argument;
                break;
            case InstructionName::INC:
                registerA += instruction.argument;
                break;
            case InstructionName::DEC:
                registerA -= instruction.argument;
                break;
            case InstructionName::NEG:
                registerA = -registerA;
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
                registerA += registerB;
                break;
            case InstructionName::SUB:
                registerA -= registerB;
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

            case InstructionName::RND:
                registerA = rng.unif(registerA);
                break;

            case InstructionName::HLT:
                return false;
        }

        updateFlags();
        std::flush(std::cout);

        if (erroneous)
            return err("step: erroneous machine state");

        return true;
    }

    public: void enableVisualization() {
        debug.doVisualizeSteps = true; }

    public: void enableStepping() {
        enableVisualization();
        debug.doWaitForUser = true; }

    public: void initializeStack(
            word_t const stackBeginning, word_t const stackEnd
    ) {
        debug.stackBoundaries = std::make_tuple(stackBeginning, stackEnd);
        registerSC = stackBeginning; }

    public: word_t storeInstruction(
            word_t const m, Instruction const instruction
    ) {
        storeMemory(MemorySemantic::InstructionHead, m, InstructionNameRepresentationHandler
                       ::toByteCode(instruction.name));
        storeMemory4(false/*TODO MemorySemantic::Instruction*/, 1+m, instruction.argument);
        debug.highestUsedMemoryLocation = 0;
        return 5;
    }

    public: word_t storeData(word_t const m, word_t const data) {
        storeMemory4(m, data);
        debug.highestUsedMemoryLocation = 0;
        return 4;
    }

    private: std::optional<Instruction> nextInstruction() {
        byte_t opCode{loadMemory(MemorySemantic::InstructionHead, registerPC++)};
        word_t argument{loadMemory4(false/*TODO MemorySemantic::Instruction*/, (registerPC += 4) - 4)};

        auto oInstructionName = InstructionNameRepresentationHandler
                                ::fromByteCode(opCode);
        if (!oInstructionName.has_value())
            return std::nullopt;

        Instruction instruction{oInstructionName.value(), static_cast<word_t>(argument)};

        return std::make_optional(instruction); }

    private: void updateFlags() {
        flagAZero = registerA == 0;
        flagANegative = static_cast<int32_t>(registerA) < 0;
        flagAEven = registerA % 2 == 0;
    }

    private: void accountForDynamicMemory(word_t const idx) {
        if (!memoryIsDynamic || idx < memory.size())
            return;

        word_t idx2{2 * idx};
        if (idx2 <= idx)
            idx2 = 0xffffffff;
        if (idx2 <= idx)
            err("accountForDynamicMemory: cannot grow");

        memory.resize(idx2);
        /* TODO catch? */
    }

    private: byte_t loadMemory(word_t const m) {
        return loadMemory(std::nullopt, m); }
    private: byte_t loadMemory(std::optional<MemorySemantic> const&oSem, word_t const m) {
        debug.highestUsedMemoryLocation = std::max(
            debug.highestUsedMemoryLocation, m);

        accountForDynamicMemory(m);
        if (/*m < 0 || */m >= memory.size()) {
            err("loadMemory: memory out of bounds (" + std::to_string(m) + " >= " + std::to_string(memory.size()) + ")");
            return 0; }

        if (oSem.has_value() && oMemorySemantics.has_value()) {
            if (oMemorySemantics.value().size() <= m) {
                err("loadMemory: no semantics available");
                return 0; }
            if (oMemorySemantics.value()[m] != oSem.value()) {
                err("loadMemory: statically invalid memory access");
                return 0; }
        }

        return memory[m];
    }

    private: void storeMemory(word_t const m, byte_t const b) {
        storeMemory(std::nullopt, m, b); }
    private: void storeMemory(std::optional<MemorySemantic> const&oSem, word_t const m, byte_t const b) {
        debug.highestUsedMemoryLocation = std::max(
            debug.highestUsedMemoryLocation, m);

        accountForDynamicMemory(m);
        if (/*m < 0 || */m >= memory.size()) {
            err("storeMemory: memory out of bounds (" + std::to_string(m) + " >= " + std::to_string(memory.size()) + ")");
            return; }

        if (oSem.has_value() && oMemorySemantics.has_value()) {
            if (oMemorySemantics.value().size() <= m) {
                err("storeMemory: no semantics available");
                return; }
            if (oMemorySemantics.value()[m] != oSem.value()) {
                err("storeMemory: statically invalid memory access");
                return; }
        }

        memory[m] = b;
    }

    private: word_t loadMemory4(word_t const m) {
        return loadMemory4(true, m); }
    private: word_t loadMemory4(bool const doStaticAnalysis, word_t const m) {
        byte_t b3{0}, b2{0}, b1{0}, b0{0};
        switch (memoryMode) {
            case MemoryMode::BigEndian:
                b3 = loadMemory(doStaticAnalysis ? std::make_optional(MemorySemantic::DataHead) : std::nullopt, m+0);
                b2 = loadMemory(doStaticAnalysis ? std::make_optional(MemorySemantic::Data)     : std::nullopt, m+1);
                b1 = loadMemory(doStaticAnalysis ? std::make_optional(MemorySemantic::Data)     : std::nullopt, m+2);
                b0 = loadMemory(doStaticAnalysis ? std::make_optional(MemorySemantic::Data)     : std::nullopt, m+3);
                break;

            case MemoryMode::LittleEndian:
                b3 = loadMemory(doStaticAnalysis ? std::make_optional(MemorySemantic::Data)     : std::nullopt, m+3);
                b2 = loadMemory(doStaticAnalysis ? std::make_optional(MemorySemantic::Data)     : std::nullopt, m+2);
                b1 = loadMemory(doStaticAnalysis ? std::make_optional(MemorySemantic::Data)     : std::nullopt, m+1);
                b0 = loadMemory(doStaticAnalysis ? std::make_optional(MemorySemantic::DataHead) : std::nullopt, m+0);
                break;
        }
        return (b3 << 24) | (b2 << 16) | (b1 << 8) | b0;
    }

    private: void storeMemory4(word_t const m, word_t const w) {
        storeMemory4(true, m, w); }
    private: void storeMemory4(bool const doStaticAnalysis, word_t const m, word_t const w) {
        byte_t b3 = static_cast<byte_t>((w >> 24) & 0xff);
        byte_t b2 = static_cast<byte_t>((w >> 16) & 0xff);
        byte_t b1 = static_cast<byte_t>((w >>  8) & 0xff);
        byte_t b0 = static_cast<byte_t>( w        & 0xff);
        switch (memoryMode) {
            case MemoryMode::LittleEndian:
                storeMemory(doStaticAnalysis ? std::make_optional(MemorySemantic::Data)     : std::nullopt, m+3, b3);
                storeMemory(doStaticAnalysis ? std::make_optional(MemorySemantic::Data)     : std::nullopt, m+2, b2);
                storeMemory(doStaticAnalysis ? std::make_optional(MemorySemantic::Data)     : std::nullopt, m+1, b1);
                storeMemory(doStaticAnalysis ? std::make_optional(MemorySemantic::DataHead) : std::nullopt, m+0, b0);
                break;

            case MemoryMode::BigEndian:
                storeMemory(doStaticAnalysis ? std::make_optional(MemorySemantic::DataHead) : std::nullopt, m+0, b3);
                storeMemory(doStaticAnalysis ? std::make_optional(MemorySemantic::Data)     : std::nullopt, m+1, b2);
                storeMemory(doStaticAnalysis ? std::make_optional(MemorySemantic::Data)     : std::nullopt, m+2, b1);
                storeMemory(doStaticAnalysis ? std::make_optional(MemorySemantic::Data)     : std::nullopt, m+3, b0);
                break;
        }
    }

    private: word_t loadMemory4Stack(word_t const m) {
        if (!debug.stackBoundaries.has_value()) {
            err("loadMemory4Stack: no stack boundaries defined");
            return 0; }
        if (m < std::get<0>(debug.stackBoundaries.value())) {
            err("loadMemory4Stack: stack underflow");
            return 0; }
        if (m >= std::get<1>(debug.stackBoundaries.value())) {
            err("loadMemory4Stack: stack overflow");
            return 0; }
        if ((m - std::get<0>(debug.stackBoundaries.value())) % 4 != 0) {
            err("loadMemory4Stack: stack misalignment");
            return 0; }
        return loadMemory4(m); }

    private: void storeMemory4Stack(word_t const m, word_t const w) {
        if (!debug.stackBoundaries.has_value()) {
            err("storeMemory4Stack: no stack boundaries defined");
            return; }
        if (m < std::get<0>(debug.stackBoundaries.value())) {
            err("storeMemory4Stack: stack underflow");
            return; }
        if (m >= std::get<1>(debug.stackBoundaries.value())) {
            err("storeMemory4Stack: stack overflow");
            return; }
        if ((m - std::get<0>(debug.stackBoundaries.value())) % 4 != 0) {
            err("storeMemory4Stack: stack misalignment");
            return; }
        storeMemory4(m, w); }

    private: bool err(std::string const&msg) {
        std::cerr << "ComputationState: " << msg << std::endl;
        return !(erroneous = true); }
};

#endif
