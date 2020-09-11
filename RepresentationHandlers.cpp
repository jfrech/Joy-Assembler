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
}

#endif
