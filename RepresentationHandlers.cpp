#ifndef JOY_ASSEMBLER__REPRESENTATION_HANDLERS_CPP
#define JOY_ASSEMBLER__REPRESENTATION_HANDLERS_CPP

#include "Types.hh"
#include "Util.cpp"

namespace InstructionNameRepresentationHandler {
    std::optional<InstructionName> fromByteCode(byte_t const opCode) {
        return instructions[opCode]; }

    byte_t toByteCode(InstructionName const name) {
        for (uint16_t opCode = 0; opCode < 0x100; ++opCode) {
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
            if (insr == Util::stringToUpper(repr))
                return std::optional<InstructionName>{in};
        return std::nullopt; }

    #define I(INS) InstructionName::INS
    #define FACTORY(IDENT, INSTRS) \
        bool IDENT(InstructionName const name) { \
            return Util::std20::contains(std::set<InstructionName>/*{*/ \
                INSTRS/*}*/, name); }
    FACTORY(isStackInstruction, ({
        I(CAL), I(RET), I(PSH), I(POP), I(LSA), I(SSA), I(LSC), I(SSC)}))
    FACTORY(doesPointAtData, ({
        I(LDA), I(LDB), I(STA), I(STB)}))
    FACTORY(doesPointAtDataByte, ({
        I(LYA), I(SYA)}))
    FACTORY(doesPointAtInstruction, ({
        I(JMP), I(JN), I(JNN), I(JZ), I(JNZ), I(JP), I(JNP), I(JE), I(JNE)}))
    #undef I
    #undef FACTORY

    constexpr std::array<uint64_t, 256> buildMicroInstructionLookupTable() {
        static_assert(std::is_same<std::underlying_type<InstructionName>::type, byte_t>::value);
        std::array<uint64_t, 256> lookupTable{};

        //NOP LDA LDB STA STB LIA SIA LPC SPC LYA SYA JMP JN JNN JZ JNZ JP JNP JE JNE CAL RET PSH POP LSA SSA LSC SSC MOV NOT SHL SHR INC DEC NEG SWP AND OR XOR ADD SUB PTU PTS PTB PTC GET GTC RND HLT
        #define N(INS, MIC) lookupTable[static_cast<std::underlying_type< \
            InstructionName>::type>(InstructionName::INS)] = (MIC)
        N(NOP, 1);
        N(LDA, 2);
        N(LDB, 2);
        N(STA, 2);
        N(STB, 2);
        N(LIA, 5);
        N(SIA, 5);
        N(LPC, 2);
        N(SPC, 2);
        N(LYA, 2);
        N(SYA, 2);
        N(JMP, 2);
        N(JN , 3);
        N(JNN, 3);
        N(JZ , 3);
        N(JNZ, 3);
        N(JP , 3);
        N(JNP, 3);
        N(JE , 3);
        N(JNE, 3);
        N(CAL, 8);
        N(RET, 8);
        N(PSH, 6);
        N(POP, 6);
        N(LSA, 5);
        N(SSA, 5);
        N(LSC, 5);
        N(SSC, 5);
        N(MOV, 2);
        N(NOT, 3);
        N(SHL, 3);
        N(SHR, 3);
        N(INC, 3);
        N(DEC, 3);
        N(NEG, 3);
        N(SWP, 5);
        N(AND, 5);
        N(OR , 5);
        N(XOR, 5);
        N(ADD, 5);
        N(SUB, 5);
        N(PTU, 9);
        N(PTS, 9);
        N(PTB, 9);
        N(PTC, 9);
        N(GET, 9);
        N(GTC, 9);
        N(RND, 9);
        N(HLT, 1);
        #undef N

        return lookupTable;
    }

    uint64_t microInstructions(InstructionName const name) {
        static_assert(std::is_same<
            std::underlying_type<InstructionName>::type, byte_t>::value);
        return buildMicroInstructionLookupTable()[
            static_cast<std::underlying_type<InstructionName>::type>(name)]; }
}

namespace InstructionRepresentationHandler {
    std::string to_string(Instruction const&instruction) {
        return InstructionNameRepresentationHandler::to_string(instruction.name)
               + " 0x" + Util::UInt32AsPaddedHex(instruction.argument); }

    std::optional<std::string> staticallyValidInstruction(std::vector<MemorySemantic> const&memorySemantics, Instruction const&instruction) {
        if (InstructionNameRepresentationHandler::doesPointAtData(instruction.name)) {
            if (instruction.argument+3 >= memorySemantics.size())
                return std::make_optional("static analysis detected an out-of-bounds data error");
            if (memorySemantics[instruction.argument] != MemorySemantic::DataHead)
                return std::make_optional("static analysis detected a misaligned data error (head)");
            for (std::size_t j{1}; j < 4; ++j)
                if (memorySemantics[instruction.argument+j] != MemorySemantic::Data)
                    return std::make_optional("static analysis detected a misaligned data error (non-head)"); }
        if (InstructionNameRepresentationHandler::doesPointAtDataByte(instruction.name)) {
            if (instruction.argument >= memorySemantics.size())
                return std::make_optional("static analysis detected an out-of-bounds data error (byte)");
            if (memorySemantics[instruction.argument] != MemorySemantic::DataHead && memorySemantics[instruction.argument] != MemorySemantic::Data)
                return std::make_optional("static analysis detected a misaligned data error (byte)"); }
        if (InstructionNameRepresentationHandler::doesPointAtInstruction(instruction.name)) {
            if (instruction.argument+4 >= memorySemantics.size())
                return std::make_optional("static analysis detected an out-of-bounds instruction error");
            if (memorySemantics[instruction.argument] != MemorySemantic::InstructionHead)
                return std::make_optional("static analysis detected a misaligned instruction error (head)");
            for (std::size_t j{1}; j < 5; ++j)
                if (memorySemantics[instruction.argument+j] != MemorySemantic::Instruction)
                    return std::make_optional("static analysis detected a misaligned instruction error (non-head)"); }
        return std::nullopt;
    }
}

#endif
