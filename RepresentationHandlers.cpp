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

    std::string toString(InstructionName const name) {
        for (InstructionDefinition const&idef : instructionDefinitions)
            if (idef.name == name)
                return std::string{idef.nameRepresentation};
        return std::string{instructionDefinitions[0xff].nameRepresentation}; }

    std::optional<InstructionName> from_string(std::string const&repr) {
        for (InstructionDefinition const&idef : instructionDefinitions)
            if (idef.nameRepresentation == Util::stringToUpper(repr))
                return std::make_optional(idef.name);
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
        static_assert(std::is_same<
            std::underlying_type<InstructionName>::type, byte_t>::value);
        std::array<uint64_t, 256> lookupTable{};

        /* NOP LDA LDB STA STB LIA SIA LPC SPC LYA SYA JMP JN JNN JZ JNZ JP JNP
           JE JNE CAL RET PSH POP LSA SSA LSC SSC MOV NOT SHL SHR INC DEC NEG
           SWP AND OR XOR ADD SUB PTU PTS PTB PTC GET GTC RND HLT */
        #define N(INS, MIC) lookupTable[static_cast<std::underlying_type< \
            InstructionName>::type>(InstructionName::INS)] = (MIC)
        N(NOP, 1);
        N(LDA, 4);
        N(LDB, 4);
        N(STA, 4);
        N(STB, 4);
        N(LIA, 6);
        N(SIA, 6);
        N(LPC, 2);
        N(SPC, 2);
        N(LYA, 4);
        N(SYA, 4);
        N(JMP, 2);
        N(JN , 2);
        N(JNN, 2);
        N(JZ , 2);
        N(JNZ, 2);
        N(JP , 2);
        N(JNP, 2);
        N(JE , 2);
        N(JNE, 2);
        N(CAL, 11);
        N(RET, 9);
        N(PSH, 9);
        N(POP, 9);
        N(LSA, 6);
        N(SSA, 6);
        N(LSC, 2);
        N(SSC, 2);
        N(MOV, 2);
        N(NOT, 1);
        N(SHL, 1);
        N(SHR, 1);
        N(INC, 1);
        N(DEC, 1);
        N(NEG, 1);
        N(SWP, 6);
        N(AND, 2);
        N(OR , 2);
        N(XOR, 2);
        N(ADD, 2);
        N(SUB, 2);
        N(PTU, 1+32+1);
        N(PTS, 1+32+1);
        N(PTB, 1+32+1);
        N(PTC, 1+32+1);
        N(GET, 32+2);
        N(GTC, 32+2);
        N(RND, 32+2);
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
    std::string toString(Instruction const&instruction) {
        return InstructionNameRepresentationHandler::toString(instruction.name)
               + " 0x" + Util::UInt32AsPaddedHex(instruction.argument); }

    std::optional<std::string> staticallyValidInstruction(
        std::vector<MemorySemantic> const&memorySemantics,
        Instruction const&instruction
    ) {
        if (
            InstructionNameRepresentationHandler
                ::doesPointAtData(instruction.name)
        ) {
            if (instruction.argument+3 >= memorySemantics.size())
                return std::make_optional("static analysis detected an "
                    "out-of-bounds data error");
            if (
                memorySemantics[instruction.argument]
                    != MemorySemantic::DataHead
            )
                return std::make_optional("static analysis detected a "
                    "misaligned data error (head)");
            for (std::size_t j{1}; j < 4; ++j)
                if (
                    memorySemantics[instruction.argument+j]
                    != MemorySemantic::Data
                )
                    return std::make_optional("static analysis detected a "
                        "misaligned data error (non-head)"); }
        if (
            InstructionNameRepresentationHandler
                ::doesPointAtDataByte(instruction.name)
        ) {
            if (instruction.argument >= memorySemantics.size())
                return std::make_optional("static analysis detected "
                    "an out-of-bounds data error (byte)");
            if (
                memorySemantics[instruction.argument]
                    != MemorySemantic::DataHead
                && memorySemantics[instruction.argument]
                    != MemorySemantic::Data
            )
                return std::make_optional("static analysis detected a "
                    "misaligned data error (byte)"); }
        if (
            InstructionNameRepresentationHandler
                ::doesPointAtInstruction(instruction.name)
        ) {
            if (instruction.argument+4 >= memorySemantics.size())
                return std::make_optional("static analysis detected an out-of-bounds instruction error");
            if (
                memorySemantics[instruction.argument]
                    != MemorySemantic::InstructionHead)
                return std::make_optional("static analysis detected a "
                    "misaligned instruction error (head)");
            for (std::size_t j{1}; j < 5; ++j)
                if (
                    memorySemantics[instruction.argument+j]
                        != MemorySemantic::Instruction
                )
                    return std::make_optional("static analysis detected a "
                        "misaligned instruction error (non-head)"); }

        return std::nullopt;
    }
}

#endif
