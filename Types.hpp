#ifndef JOY_ASSEMBLER__TYPES_HPP
#define JOY_ASSEMBLER__TYPES_HPP

using byte_t = uint8_t;
using word_t = uint32_t;
using uint_t = uint64_t;

enum class MemoryMode : bool { LittleEndian, BigEndian };
enum class MemorySemantic : uint8_t {
    InstructionHead, Instruction, DataHead, Data
};
using WordMemorySemantic = std::array<std::optional<MemorySemantic>, 4>;
WordMemorySemantic constexpr wordMemorySemanticInstruction{
    std::make_optional(MemorySemantic::InstructionHead), std::make_optional(MemorySemantic::Instruction),
    std::make_optional(MemorySemantic::Instruction), std::make_optional(MemorySemantic::Instruction)};
WordMemorySemantic constexpr wordMemorySemanticData{
    std::make_optional(MemorySemantic::DataHead), std::make_optional(MemorySemantic::Data),
    std::make_optional(MemorySemantic::Data), std::make_optional(MemorySemantic::Data)};
WordMemorySemantic constexpr wordMemorySemanticNone{
    std::nullopt, std::nullopt, std::nullopt, std::nullopt};

enum class InstructionName : byte_t {
    /* The order in which the following enum identifiers appear *defines*
       their op-code. */
    NOP, LDA, LDB, STA, STB, LIA, SIA, LPC, SPC, LYA, SYA, JMP, JN, JNN, JZ,
    JNZ, JP, JNP, JE, JNE, CAL, RET, PSH, POP, LSA, SSA, LSC, SSC, MOV, NOT,
    SHL, SHR, INC, DEC, NEG, SWP, ADD, SUB, AND, OR, XOR, GET, GTC, PTU, PTS,
    PTB, PTC, RND, HLT
};

struct InstructionDefinition {
    bool opCodeUsed{false};
    InstructionName name{InstructionName::NOP};
    /* a slight hack since neither `std::string` nor `std::string_view`
       work properly in C++17 `constexpr` contexts */
    char const*_nameRepresentation{};
    bool requiresArgument{false};
    std::optional<word_t> optionalArgument{std::nullopt};
    uint_t microInstructions{0};

    /* a slight hack; see `_nameRepresentation` */
    std::string getNameRepresentation() const {
        if (!_nameRepresentation)
            return std::string{"erroneous-instruction"};
        return std::string{_nameRepresentation};
    }

    bool doesTakeArgument() const {
        return requiresArgument || optionalArgument != std::nullopt;
    }
};

namespace InstructionDefinitionsUtil {
    using InstructionDefinitionsArray = std::array<InstructionDefinition, 256>;
    uint_t constexpr ioPenalty{32};

    constexpr void instructionWithArgument(
        InstructionDefinitionsArray &ida,
        InstructionName const name, char const*_nameRepresentation,
        std::optional<word_t> const&optionalArgument,
        uint_t const microInstructions
    ) {
        ida[static_cast<std::underlying_type<InstructionName>::type>(name)] =
            InstructionDefinition{true, name, _nameRepresentation,
                optionalArgument == std::nullopt, optionalArgument,
                microInstructions};
    }

    constexpr void instructionWithoutArgument(
        InstructionDefinitionsArray &ida,
        InstructionName const name, char const*_nameRepresentation,
        uint_t const microInstructions
    ) {
        ida[static_cast<std::underlying_type<InstructionName>::type>(name)] =
            InstructionDefinition{true, name, _nameRepresentation,
                false, std::nullopt, microInstructions};
    }

    constexpr InstructionDefinitionsArray build() {
        static_assert(std::is_same<
            std::underlying_type<InstructionName>::type, byte_t>::value);

        InstructionDefinitionsArray ida{};

        instructionWithArgument(ida, InstructionName::NOP, "NOP", std::optional<word_t>{0}, 1);
        instructionWithArgument(ida, InstructionName::LDA, "LDA", std::nullopt, 4);
        instructionWithArgument(ida, InstructionName::LDB, "LDB", std::nullopt, 4);
        instructionWithArgument(ida, InstructionName::STA, "STA", std::nullopt, 4);
        instructionWithArgument(ida, InstructionName::STB, "STB", std::nullopt, 4);
        instructionWithArgument(ida, InstructionName::LIA, "LIA", std::optional<word_t>{0}, 6);
        instructionWithArgument(ida, InstructionName::SIA, "SIA", std::optional<word_t>{0}, 6);
        instructionWithoutArgument(ida, InstructionName::LPC, "LPC", 2);
        instructionWithoutArgument(ida, InstructionName::SPC, "SPC", 2);
        instructionWithArgument(ida, InstructionName::LYA, "LYA", std::nullopt, 4);
        instructionWithArgument(ida, InstructionName::SYA, "SYA", std::nullopt, 4);
        instructionWithArgument(ida, InstructionName::JMP, "JMP", std::nullopt, 2);
        instructionWithArgument(ida, InstructionName::JN , "JN" , std::nullopt, 3);
        instructionWithArgument(ida, InstructionName::JNN, "JNN", std::nullopt, 3);
        instructionWithArgument(ida, InstructionName::JZ , "JZ" , std::nullopt, 3);
        instructionWithArgument(ida, InstructionName::JNZ, "JNZ", std::nullopt, 3);
        instructionWithArgument(ida, InstructionName::JP , "JP" , std::nullopt, 3);
        instructionWithArgument(ida, InstructionName::JNP, "JNP", std::nullopt, 3);
        instructionWithArgument(ida, InstructionName::JE , "JE" , std::nullopt, 3);
        instructionWithArgument(ida, InstructionName::JNE, "JNE", std::nullopt, 3);
        instructionWithArgument(ida, InstructionName::CAL, "CAL", std::nullopt, 11);
        instructionWithoutArgument(ida, InstructionName::RET, "RET", 9);
        instructionWithoutArgument(ida, InstructionName::PSH, "PSH", 9);
        instructionWithoutArgument(ida, InstructionName::POP, "POP", 9);
        instructionWithArgument(ida, InstructionName::LSA, "LSA", std::optional<word_t>{0}, 6);
        instructionWithArgument(ida, InstructionName::SSA, "SSA", std::optional<word_t>{0}, 6);
        instructionWithoutArgument(ida, InstructionName::LSC, "LSC", 2);
        instructionWithoutArgument(ida, InstructionName::SSC, "SSC", 2);
        instructionWithArgument(ida, InstructionName::MOV, "MOV", std::nullopt, 2);
        instructionWithoutArgument(ida, InstructionName::NOT, "NOT", 1);
        instructionWithArgument(ida, InstructionName::SHL, "SHL", std::optional<word_t>{1}, 1);
        instructionWithArgument(ida, InstructionName::SHR, "SHR", std::optional<word_t>{1}, 1);
        instructionWithArgument(ida, InstructionName::INC, "INC", std::optional<word_t>{1}, 1);
        instructionWithArgument(ida, InstructionName::DEC, "DEC", std::optional<word_t>{1}, 1);
        instructionWithoutArgument(ida, InstructionName::NEG, "NEG", 1);
        instructionWithoutArgument(ida, InstructionName::SWP, "SWP", 3);
        instructionWithoutArgument(ida, InstructionName::ADD, "ADD", 2);
        instructionWithoutArgument(ida, InstructionName::SUB, "SUB", 2);
        instructionWithoutArgument(ida, InstructionName::AND, "AND", 2);
        instructionWithoutArgument(ida, InstructionName::OR , "OR" , 2);
        instructionWithoutArgument(ida, InstructionName::XOR, "XOR", 2);
        instructionWithoutArgument(ida, InstructionName::GET, "GET", ioPenalty+2);
        instructionWithoutArgument(ida, InstructionName::GTC, "GTC", ioPenalty+2);
        instructionWithoutArgument(ida, InstructionName::PTU, "PTU", 1+ioPenalty+1);
        instructionWithoutArgument(ida, InstructionName::PTS, "PTS", 1+ioPenalty+1);
        instructionWithoutArgument(ida, InstructionName::PTB, "PTB", 1+ioPenalty+1);
        instructionWithoutArgument(ida, InstructionName::PTC, "PTC", 1+ioPenalty+1);
        instructionWithoutArgument(ida, InstructionName::RND, "RND", ioPenalty+2);
        instructionWithoutArgument(ida, InstructionName::HLT, "HLT", 1);

        return ida;
    }
}

std::array<InstructionDefinition, 256> constexpr instructionDefinitions{
    InstructionDefinitionsUtil::build()};

struct Instruction {
    InstructionName name;
    word_t argument;

    bool operator==(Instruction const& instruction) const {
        return name == instruction.name
            && argument == instruction.argument;
    }

    bool operator!=(Instruction const& instruction) const {
        return !(*this == instruction);
    }
};

struct ComputationStateDebug {
    word_t highestUsedMemoryLocation{0};
    bool doWaitForUser{false}, doVisualizeSteps{false};
    std::optional<std::tuple<word_t, word_t>> stackBoundaries{std::nullopt};
};

struct ComputationStateStatistics {
    uint_t nInstructions, nMicroInstructions;

    ComputationStateStatistics operator-(
        ComputationStateStatistics const&statistics
    ) const {
        return ComputationStateStatistics{
            nInstructions - statistics.nInstructions,
            nMicroInstructions - statistics.nMicroInstructions};
    }

    std::string toString() const {
        return "#" + std::to_string(nInstructions) + ": "
            + std::to_string(nMicroInstructions);
    }
};

#endif
