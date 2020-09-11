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

    uint64_t microInstructions(Instruction const instruction) {
        #define N(n, v) case InstructionName::n: return v;
        switch (instruction.name) {
            //NOP LDA LDB STA STB LIA SIA LPC SPC LYA SYA JMP JN JNN JZ JNZ JP JNP JE JNE CAL RET PSH POP LSA SSA LSC SSC MOV NOT SHL SHR INC DEC NEG SWP AND OR XOR ADD SUB PTU PTS PTB PTC GET GTC RND HLT
            N(NOP, 1)
            N(LDA, 2)
            N(LDB, 2)
            N(STA, 2)
            N(STB, 2)
            N(LIA, 2)
            N(SIA, 2)
            N(LPC, 2)
            N(SPC, 2)
            N(LYA, 3)
            N(SYA, 3)
            N(JMP, 2)
            N(JN , 2)
            N(JNN, 2)
            N(JZ , 2)
            N(JNZ, 2)
            N(JP , 2)
            N(JNP, 2)
            N(JE , 2)
            N(JNE, 2)
            N(CAL, 4 + 100)
            N(RET, 3 + 100)
            N(PSH, 3)
            N(POP, 2)
            N(LSA, 2)
            N(SSA, 2)
            N(LSC, 2)
            N(SSC, 2)
            N(MOV, 2)
            N(NOT, 2)
            N(SHL, 2)
            N(SHR, 2)
            N(INC, 2)
            N(DEC, 2)
            N(NEG, 2)
            N(SWP, 2)
            N(AND, 2)
            N(OR , 2)
            N(XOR, 2)
            N(ADD, 2)
            N(SUB, 2)
            N(PTU, 4)
            N(PTS, 4)
            N(PTB, 4)
            N(PTC, 4)
            N(GET, 12)
            N(GTC, 3)
            N(RND, 2)
            N(HLT, 1)
        }
        #undef N

        return 0;
    }
}

#endif
