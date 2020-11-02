#ifndef JOY_ASSEMBLER__COMPUTATION_CPP
#define JOY_ASSEMBLER__COMPUTATION_CPP

#include "Includes.hpp"

class ComputationState {
    friend class Parser;

    private:
        std::vector<byte_t> memory;
        bool const memoryIsDynamic;
        MemoryMode const memoryMode;
        word_t registerA, registerB, registerPC, registerSC;
        bool flagAZero, flagANegative, flagAEven;
        Util::rng_t rng;

        std::vector<std::vector<std::tuple<bool, std::string>>> const profiler;
        ComputationStateStatistics statistics;
        std::stack<ComputationStateStatistics> profilerStatistics;
        bool embedProfilerOutput;
        std::optional<std::vector<MemorySemantic>> oMemorySemantics;

        bool mock;
        mutable bool ok;
    public:
        ComputationStateDebug debug;

    public: ComputationState(
        word_t const memorySize,
        bool const memoryIsDynamic,
        MemoryMode const memoryMode,
        Util::rng_t const&rng,
        std::vector<std::vector<std::tuple<bool, std::string>>> const&profiler,
        bool const embedProfilerOutput,
        std::optional<std::vector<MemorySemantic>> const&oMemorySemantics
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

        mock{false}, ok{true},

        debug{}
    {
        updateFlags(); }

    /* since ComputationState::memory may be rather large, all copy
       constructors are deleted and the default move constructor is used */
    public: ComputationState(ComputationState &)      = delete;
    public: ComputationState(ComputationState const&) = delete;
    public: ComputationState(ComputationState &&)     = default;

    public: void visualize(bool const blockAllowed=true) {
        if (!debug.doVisualizeSteps)
            return;

        auto const paintFaint{
            Util::ANSI_COLORS::paintFactory(Util::ANSI_COLORS::FAINT)};
        auto const paintRegister{
            Util::ANSI_COLORS::paintFactory(Util::ANSI_COLORS::REGISTER)};

        auto const print{[](std::string const&msg) {
            std::cout << msg; }};

        print("\n    ====================- MEMORY -=====================\n");
        word_t pc{0}, rPC{registerPC};
        word_t const w{16};
        print("       ");
        for (word_t x = 0; x < w; ++x)
            print(paintFaint("_" + Util::UNibbleAsPaddedHex(x & 0xf) + " "));
        for (word_t y = 0; true; ++y) {
            print("\n    " + paintFaint(
                Util::UInt8AsPaddedHex(y & 0xff) + "_"));
            for (word_t x = 0; x < w; ++x) {
                word_t m{y *w+ x};

                if (m >= memory.size()) {
                    print(" --");
                    continue; }

                if (oMemorySemantics.has_value()) {
                    std::vector<MemorySemantic> const memorySemantics{
                        oMemorySemantics.value()};
                    if (m < memorySemantics.size())
                        print(Util::ANSI_COLORS
                            ::memorySemanticColor(memorySemantics[m])); }

                if (registerSC != 0) {
                    if (registerSC <= pc+4 && pc+4 < registerSC + 4)
                        print(Util::ANSI_COLORS::STACK_FAINT);
                    if (registerSC <= pc && pc < registerSC + 4)
                        print(Util::ANSI_COLORS::STACK);
                }
                if (rPC <= pc && pc < rPC + 5)
                    print(pc == rPC ? Util::ANSI_COLORS::INSTRUCTION_NAME
                        : Util::ANSI_COLORS::INSTRUCTION_ARGUMENT);
                else if (m <= debug.highestUsedMemoryLocation)
                    print(Util::ANSI_COLORS::MEMORY_LOCATION_USED);


                print(" " + Util::UInt8AsPaddedHex(memory[y *w+ x]));
                print(Util::ANSI_COLORS::CLEAR);
                ++pc;
            }

            /* TODO */
            if ((y+1)*w >= 0x100 && debug.highestUsedMemoryLocation+1 < (y+1)*w)
                break;
        }
        print("\n    Current instruction: ");

        std::string opCodeName{
            InstructionNameRepresentationHandler::toString(
                InstructionNameRepresentationHandler::fromByteCode(
                    loadMemory(registerPC, std::nullopt)))};
        word_t argument{loadMemory4(registerPC+1, wordMemorySemanticInstructionData)};
        print(Util::ANSI_COLORS::paint(Util::ANSI_COLORS
                ::INSTRUCTION_NAME, opCodeName)
            + " " + Util::ANSI_COLORS::paint(Util::ANSI_COLORS
                ::INSTRUCTION_ARGUMENT, "0x"
                + Util::UInt32AsPaddedHex(argument))
            + " (#" + std::to_string(statistics.nInstructions) + ": "
            + std::to_string(statistics.nMicroInstructions) + ")\n");

        print("    Registers:    " + std::string{"A:  0x"}
            + paintRegister(Util::UInt32AsPaddedHex(registerA))
            + ",     B:  0x"
            + paintRegister(Util::UInt32AsPaddedHex(registerB)) + "\n");
        print("                  " + std::string{"PC: 0x"}
            + paintRegister(Util::UInt32AsPaddedHex(registerPC))
            + ",     SC: 0x"
            + paintRegister(Util::UInt32AsPaddedHex(registerSC)) + "\n");
        print("    Flags (A zero, A negative, A even): "
            + paintRegister(Util::UBitAsPaddedHex(flagAZero))
            + paintRegister(Util::UBitAsPaddedHex(flagANegative))
            + paintRegister(Util::UBitAsPaddedHex(flagAEven)) + "\n");

        print("    % ");

        if (blockAllowed) {
            if (debug.doWaitForUser)
                UTF8IO::getRune();
            else if (debug.doVisualizeSteps)
                Util::IO::wait(); }
    }

    public: void memoryDump() {
        mock = true;

        std::cout
            << "A: 0x" << Util::UInt32AsPaddedHex(registerA)
            << ", B: 0x" << Util::UInt32AsPaddedHex(registerB)
            << ", PC: 0x" << Util::UInt32AsPaddedHex(registerPC)
            << ", SC: 0x" << Util::UInt32AsPaddedHex(registerSC)
            << "; memory (" << std::to_string(memory.size()) + "B):";

        // do not print unnecessary zeros
        if (!memory.empty()) {
            word_t mx{static_cast<word_t>(memory.size()-1)};
            while (mx > 0 && memory[mx] == 0)
                --mx;
            for (byte_t const b : memory) {
                std::cout << " " + Util::UInt8AsPaddedHex(b);
                if (mx-- <= 0)
                    break;
            }
        }

        std::cout << "\n";
    }

    private: void checkProfiler() {
        if (profiler.size() <= registerPC)
            return;

        auto const prf{[](std::string const&msg) {
            std::clog << msg << std::endl; }};

        for (auto [doStart, msg] : profiler.at(registerPC)) {
            std::string const str{"[# "
                + std::to_string(statistics.nInstructions) + ": "
                + std::to_string(statistics.nMicroInstructions) + "] "};
            std::string const strPad{std::string(str.size(), ' ')};

            if (doStart) {
                if (!embedProfilerOutput)
                    prf(str + "starting profiler: " + msg);
                profilerStatistics.push(statistics);
                continue; }

            if (!embedProfilerOutput)
                prf(str + "stopping profiler: " + msg);
            if (profilerStatistics.empty()) {
                prf(str + "profiler could not be stopped since it was "
                    "never started");
                continue; }

            ComputationStateStatistics const start{profilerStatistics.top()};
            ComputationStateStatistics const stop{statistics};
            profilerStatistics.pop();
            if (
                start.nInstructions > stop.nInstructions
                || start.nMicroInstructions > stop.nMicroInstructions
            ) {
                prf(strPad + "profiler time travelled");
                continue; }

            ComputationStateStatistics const elapsed{stop - start};
            if (!embedProfilerOutput) {
                prf(strPad + "-> number of elapsed instructions: "
                    + elapsed.toString());
                continue; }

            std::cout << std::to_string(elapsed.nMicroInstructions) << "\n";
        }
    }

    public: bool step() {
        checkProfiler();

        Instruction instruction{nextInstruction()};

        ++statistics.nInstructions;
        statistics.nMicroInstructions += InstructionNameRepresentationHandler
            ::microInstructions(instruction.name);

        auto jmp = [&](bool const cnd) {
            if (cnd)
                registerPC = instruction.argument; };

        switch (instruction.name) {
            case InstructionName::NOP:
                break;

            case InstructionName::LDA:
                registerA = loadMemory4(instruction.argument, wordMemorySemanticData);
                break;
            case InstructionName::LDB:
                registerB = loadMemory4(instruction.argument, wordMemorySemanticData);
                break;
            case InstructionName::STA:
                storeMemory4(instruction.argument, registerA, wordMemorySemanticData);
                break;
            case InstructionName::STB:
                storeMemory4(instruction.argument, registerB, wordMemorySemanticData);
                break;
            case InstructionName::LIA:
                registerA = loadMemory4(registerB + instruction.argument, wordMemorySemanticData);
                break;
            case InstructionName::SIA:
                storeMemory4(registerB + instruction.argument, registerA, wordMemorySemanticData);
                break;
            case InstructionName::LPC:
                registerA = registerPC;
                break;
            case InstructionName::SPC:
                registerPC = registerA;
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
                registerA = registerSC;
                break;
            case InstructionName::SSC:
                registerSC = registerA;
                break;

            case InstructionName::MOV:
                registerA = instruction.argument;
                break;
            case InstructionName::NOT:
                registerA = ~registerA;
                break;
            case InstructionName::SHL:
                if (instruction.argument < 32)
                    registerA <<= instruction.argument;
                else
                    registerA = 0;
                break;
            case InstructionName::SHR:
                if (instruction.argument < 32)
                    registerA >>= instruction.argument;
                else
                    registerA = 0;
                break;
            case InstructionName::INC:
                registerA += instruction.argument;
                break;
            case InstructionName::DEC:
                registerA -= instruction.argument;
                break;
            case InstructionName::NEG:
                registerA = Util::toTwo_sComplement<int32_t, uint32_t, 32>(
                    -Util::fromTwo_sComplement<uint32_t, int32_t, 32>(
                        registerA));
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
                registerA += Util::toTwo_sComplement<int32_t, uint32_t, 32>(
                    -Util::fromTwo_sComplement<uint32_t, int32_t, 32>(
                        registerB));
                break;

            case InstructionName::PTU:
                if (mock)
                    break;
                std::cout << static_cast<uint32_t>(registerA) << "\n";
                break;
            case InstructionName::PTS:
                if (mock)
                    break;
                std::cout << static_cast<int32_t>(
                    Util::fromTwo_sComplement<uint32_t, int32_t, 32>(
                        registerA)) << "\n";
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
                    oN = Util::stringToOptionalUInt32(get);
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

        if (!ok)
            return err("step: erroneous machine state");

        return true;
    }

    public: word_t storeInstruction(
            word_t const m, Instruction const instruction
    ) {
        if (oMemorySemantics.has_value()) {
            std::optional<std::string> e{
                InstructionRepresentationHandler
                ::staticallyValidInstruction(
                    oMemorySemantics.value(), instruction)};
            if (e.has_value())
                err("instruction " + InstructionRepresentationHandler
                    ::toString(instruction) + ": " + e.value());
        }

        storeMemory(m, InstructionNameRepresentationHandler
            ::toByteCode(instruction.name), MemorySemantic::InstructionHead);
        storeMemory4(1+m, instruction.argument,
            wordMemorySemanticNone/*TODO Why no semantic check?*/);
        debug.highestUsedMemoryLocation = 0;
        return 5;
    }

    public: word_t storeData(word_t const m, word_t const data) {
        storeMemory4(m, data, wordMemorySemanticData);
        debug.highestUsedMemoryLocation = 0;
        return 4;
    }

    private: Instruction nextInstruction() {
        byte_t opCode{loadMemory(
            registerPC++, MemorySemantic::InstructionHead)};
        word_t argument{loadMemory4(
            (registerPC += 4) - 4, wordMemorySemanticInstructionData)};

        return Instruction{InstructionNameRepresentationHandler
            ::fromByteCode(opCode), argument};
    }

    private: void updateFlags() {
        flagAZero = registerA == 0;
        flagANegative = Util::fromTwo_sComplement<uint32_t, int32_t, 32>(
            registerA) < 0;
        flagAEven = registerA % 2 == 0; }

    private: byte_t loadMemory(
        word_t const m,
        std::optional<MemorySemantic> const&oSem=std::nullopt
    ) {
        debug.highestUsedMemoryLocation = std::max(
            debug.highestUsedMemoryLocation, m);

        if (/*m < 0 || */m >= memory.size()) {
            if (!memoryIsDynamic)
                throw std::runtime_error{"loadMemory: memory out of bounds ("
                    + std::to_string(m) + " >= " + std::to_string(memory.size())
                    + ")"};
            memory.resize(m+1);
        }

        if (oSem.has_value() && oMemorySemantics.has_value()) {
            if (oMemorySemantics.value().size() <= m)
                throw std::runtime_error{"loadMemory: no semantics available"};
            if (oMemorySemantics.value()[m] != oSem.value())
                throw std::runtime_error{
                    "loadMemory: statically invalid memory access"};
        }

        return memory[m];
    }

    private: void storeMemory(
        word_t const m, byte_t const b,
        std::optional<MemorySemantic> const&oSem=std::nullopt
    ) {
        debug.highestUsedMemoryLocation = std::max(
            debug.highestUsedMemoryLocation, m);

        if (/*m < 0 || */m >= memory.size()) {
            if (!memoryIsDynamic)
                throw std::runtime_error{"storeMemory: memory out of bounds ("
                    + std::to_string(m) + " >= " + std::to_string(memory.size())
                    + ")"};
            memory.resize(m+1);
        }

        if (oSem.has_value() && oMemorySemantics.has_value()) {
            if (oMemorySemantics.value().size() <= m)
                throw std::runtime_error{"storeMemory: no semantics available"};
            if (oMemorySemantics.value()[m] != oSem.value())
                throw std::runtime_error{
                    "storeMemory: statically invalid memory access"};
        }

        memory[m] = b;
    }

    private: word_t loadMemory4(
        word_t const m,
        WordMemorySemantic const&wordMemorySemantic
    ) {
        byte_t b3{0}, b2{0}, b1{0}, b0{0};
        switch (memoryMode) {
            case MemoryMode::LittleEndian:
                b3 = loadMemory(m+3, wordMemorySemantic[3]);
                b2 = loadMemory(m+2, wordMemorySemantic[2]);
                b1 = loadMemory(m+1, wordMemorySemantic[1]);
                b0 = loadMemory(m+0, wordMemorySemantic[0]);
                break;

            case MemoryMode::BigEndian:
                b3 = loadMemory(m+0, wordMemorySemantic[3]);
                b2 = loadMemory(m+1, wordMemorySemantic[2]);
                b1 = loadMemory(m+2, wordMemorySemantic[1]);
                b0 = loadMemory(m+3, wordMemorySemantic[0]);
                break;
        }

        return (b3 << 24) | (b2 << 16) | (b1 << 8) | b0;
    }

    private: void storeMemory4(
        word_t const m, word_t const w,
        WordMemorySemantic const&wordMemorySemantic
    ) {
        byte_t const b3{static_cast<byte_t>((w >> 24) & 0xff)};
        byte_t const b2{static_cast<byte_t>((w >> 16) & 0xff)};
        byte_t const b1{static_cast<byte_t>((w >>  8) & 0xff)};
        byte_t const b0{static_cast<byte_t>( w        & 0xff)};
        switch (memoryMode) {
            case MemoryMode::LittleEndian:
                storeMemory(m+3, b3, wordMemorySemantic[3]);
                storeMemory(m+2, b2, wordMemorySemantic[1]);
                storeMemory(m+1, b1, wordMemorySemantic[2]);
                storeMemory(m+0, b0, wordMemorySemantic[0]);
                break;

            case MemoryMode::BigEndian:
                storeMemory(m+0, b3, wordMemorySemantic[3]);
                storeMemory(m+1, b2, wordMemorySemantic[1]);
                storeMemory(m+2, b1, wordMemorySemantic[2]);
                storeMemory(m+3, b0, wordMemorySemantic[0]);
                break;
        }
    }

    private: void assureStackBoundaries(
        std::string const&callSite, word_t const m
    ) {
        if (!debug.stackBoundaries.has_value())
            throw std::runtime_error{
                callSite + ": no stack boundaries are defined"};
        if (m < std::get<0>(debug.stackBoundaries.value()))
            throw std::runtime_error{callSite + ": stack underflow"};
        if (m >= std::get<1>(debug.stackBoundaries.value()))
            throw std::runtime_error{callSite + ": stack overflow"};
        if ((m - std::get<0>(debug.stackBoundaries.value())) % 4 != 0)
            throw std::runtime_error{callSite + ": stack misalignment"};
    }

    private: word_t loadMemory4Stack(word_t const m) {
        assureStackBoundaries("loadMemory4Stack", m);
        return loadMemory4(m, wordMemorySemanticData); }

    private: void storeMemory4Stack(word_t const m, word_t const w) {
        assureStackBoundaries("storeMemory4Stack", m);
        storeMemory4(m, w, wordMemorySemanticData); }

    private: bool err(std::string const&msg) const {
        std::cerr << "ComputationState: " << msg << std::endl;
        return ok = false; }
};

#endif
