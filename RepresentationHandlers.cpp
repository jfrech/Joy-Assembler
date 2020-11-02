#ifndef JOY_ASSEMBLER__REPRESENTATION_HANDLERS_CPP
#define JOY_ASSEMBLER__REPRESENTATION_HANDLERS_CPP

#include "Types.hpp"
#include "Util.cpp"

namespace InstructionNameRepresentationHandler {

    InstructionName fromByteCode(byte_t const opCode) {
        std::optional<InstructionName> oName{
            instructionDefinitions[opCode].name};

        if (!oName.has_value())
            throw std::runtime_error{
                "invalid op-code: " + std::to_string(opCode)};

        return oName.value();
    }

    byte_t toByteCode(InstructionName const name) {
        for (uint16_t opCode = 0; opCode < 0x100; ++opCode) {
            if (!instructionDefinitions[opCode].opCodeUsed)
                continue;
            if (instructionDefinitions[opCode].name == name)
                return static_cast<byte_t>(opCode); }
        return 0x00;
    }

    std::string toString(InstructionName const name) {
        for (InstructionDefinition const&idef : instructionDefinitions)
            if (idef.name == name)
                return idef.getNameRepresentation();
        return instructionDefinitions[0xff].getNameRepresentation();
    }

    std::optional<InstructionName> from_string(std::string const&repr) {
        for (InstructionDefinition const&idef : instructionDefinitions)
            if (idef.getNameRepresentation() == Util::stringToUpper(repr))
                return std::make_optional(idef.name);
        return std::nullopt;
    }

    bool doesPointAtStack(InstructionName const name) {
        return Util::contains(std::array{
            InstructionName::CAL, InstructionName::RET,
            InstructionName::PSH, InstructionName::POP,
            InstructionName::LSA, InstructionName::SSA,
            InstructionName::LSC, InstructionName::SSC,
        }, name);
    }
    bool doesPointAtData(InstructionName const name) {
        return Util::contains(std::array{
            InstructionName::LDA, InstructionName::LDB,
            InstructionName::STA, InstructionName::STB,
        }, name);
    }
    bool doesPointAtDataByte(InstructionName const name) {
        return Util::contains(std::array{
            InstructionName::LYA, InstructionName::SYA,
        }, name);
    }
    bool doesPointAtInstruction(InstructionName const name) {
        return Util::contains(std::array{
            InstructionName::JMP,
            InstructionName::JN, InstructionName::JNN,
            InstructionName::JZ, InstructionName::JNZ,
            InstructionName::JP, InstructionName::JNP,
            InstructionName::JE, InstructionName::JNE,
        }, name);
    }

    namespace MicroInstructionsUtil {
        /* TODO `constexpr` causes an internal compiler error */ std::array<uint_t, 256> build() {
            static_assert(std::is_same<
                std::underlying_type<InstructionName>::type, byte_t>::value);
            std::array<uint_t, 256> lookupTable{};

            for (InstructionDefinition const&idef : instructionDefinitions)
                lookupTable[static_cast<std::underlying_type<InstructionName>
                    ::type>(idef.name)] = idef.microInstructions;

            return lookupTable;
        }

        std::array<uint_t, 256> const /* TODO `constexpr` requires build() to be `constexpr` */ lookupTable{build()};
    }

    constexpr uint_t microInstructions(InstructionName const name) {
        return MicroInstructionsUtil::lookupTable[
            static_cast<std::underlying_type<InstructionName>::type>(name)];
    }
}

namespace InstructionRepresentationHandler {

    std::string toString(Instruction const&instruction) {
        return InstructionNameRepresentationHandler::toString(instruction.name)
               + " 0x" + Util::UInt32AsPaddedHex(instruction.argument);
    }

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
                        "misaligned data error (non-head)");
        }

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
                    "misaligned data error (byte)");
        }

        if (
            InstructionNameRepresentationHandler
                ::doesPointAtInstruction(instruction.name)
        ) {
            if (instruction.argument+4 >= memorySemantics.size())
                return std::make_optional("static analysis detected an "
                    "out-of-bounds instruction error");
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
                        "misaligned instruction error (non-head)");
        }

        return std::nullopt;
    }
}

#endif
