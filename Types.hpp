#ifndef JOY_ASSEMBLER__TYPES_HPP
#define JOY_ASSEMBLER__TYPES_HPP

typedef uint8_t byte_t;
typedef uint32_t word_t;
typedef uint64_t uint_t;

enum class MemoryMode : bool { LittleEndian, BigEndian };
enum class MemorySemantic : uint8_t {
    InstructionHead, Instruction, DataHead, Data };

enum class InstructionName : byte_t {
    /* The order in which the following enum identifiers appear defines
       their op-code. */
    NOP, LDA, LDB, STA, STB, LIA, SIA, LPC, SPC, LYA, SYA, JMP, JN, JNN, JZ,
    JNZ, JP, JNP, JE, JNE, CAL, RET, PSH, POP, LSA, SSA, LSC, SSC, MOV, NOT,
    SHL, SHR, INC, DEC, NEG, SWP, ADD, SUB, AND, OR, XOR, GET, GTC, PTU, PTS,
    PTB, PTC, RND, HLT
};

struct InstructionDefinition {
    public:
        bool opCodeUsed{false};
        InstructionName name{InstructionName::NOP};
        std::string_view nameRepresentation{"(unused instruction)"};
        bool requiresArgument{false};
        std::optional<word_t> optionalArgument{std::nullopt};
        uint_t microInstructions{0};
};

namespace InstructionDefinitionsUtil {
    using InstructionDefinitionsArray = std::array<InstructionDefinition, 256>;

    constexpr void wArg(
        InstructionDefinitionsArray &ida,
        InstructionName const name, std::string_view const nameRepresentation,
        std::optional<word_t> const&optionalArgument,
        uint_t const microInstructions
    ) {
        ida[static_cast<std::underlying_type<InstructionName>::type>(name)] =
            InstructionDefinition{true, name, nameRepresentation,
                optionalArgument == std::nullopt, optionalArgument,
                microInstructions};
    }

    constexpr void woArg(
        InstructionDefinitionsArray &ida,
        InstructionName const name, std::string_view const nameRepresentation,
        uint_t const microInstructions
    ) {
        ida[static_cast<std::underlying_type<InstructionName>::type>(name)] =
            InstructionDefinition{true, name, nameRepresentation,
                false, std::nullopt, microInstructions};
    }

    constexpr uint_t const ioPenalty{32};

    constexpr InstructionDefinitionsArray build() {
        static_assert(std::is_same<
            std::underlying_type<InstructionName>::type, byte_t>::value);

        InstructionDefinitionsArray ida{};

        wArg(ida, InstructionName::NOP, "NOP", std::optional<word_t>{0}, 1);
        wArg(ida, InstructionName::LDA, "LDA", std::nullopt, 4);
        wArg(ida, InstructionName::LDB, "LDB", std::nullopt, 4);
        wArg(ida, InstructionName::STA, "STA", std::nullopt, 4);
        wArg(ida, InstructionName::STB, "STB", std::nullopt, 4);
        wArg(ida, InstructionName::LIA, "LIA", std::optional<word_t>{0}, 6);
        wArg(ida, InstructionName::SIA, "SIA", std::optional<word_t>{0}, 6);
        woArg(ida, InstructionName::LPC, "LPC", 2);
        woArg(ida, InstructionName::SPC, "SPC", 2);
        wArg(ida, InstructionName::LYA, "LYA", std::nullopt, 4);
        wArg(ida, InstructionName::SYA, "SYA", std::nullopt, 4);
        wArg(ida, InstructionName::JMP, "JMP", std::nullopt, 2);
        wArg(ida, InstructionName::JN , "JN" , std::nullopt, 3);
        wArg(ida, InstructionName::JNN, "JNN", std::nullopt, 3);
        wArg(ida, InstructionName::JZ , "JZ" , std::nullopt, 3);
        wArg(ida, InstructionName::JNZ, "JNZ", std::nullopt, 3);
        wArg(ida, InstructionName::JP , "JP" , std::nullopt, 3);
        wArg(ida, InstructionName::JNP, "JNP", std::nullopt, 3);
        wArg(ida, InstructionName::JE , "JE" , std::nullopt, 3);
        wArg(ida, InstructionName::JNE, "JNE", std::nullopt, 3);
        wArg(ida, InstructionName::CAL, "CAL", std::nullopt, 11);
        woArg(ida, InstructionName::RET, "RET", 9);
        woArg(ida, InstructionName::PSH, "PSH", 9);
        woArg(ida, InstructionName::POP, "POP", 9);
        wArg(ida, InstructionName::LSA, "LSA", std::optional<word_t>{0}, 6);
        wArg(ida, InstructionName::SSA, "SSA", std::optional<word_t>{0}, 6);
        woArg(ida, InstructionName::LSC, "LSC", 2);
        woArg(ida, InstructionName::SSC, "SSC", 2);
        wArg(ida, InstructionName::MOV, "MOV", std::nullopt, 2);
        woArg(ida, InstructionName::NOT, "NOT", 1);
        wArg(ida, InstructionName::SHL, "SHL", std::optional<word_t>{1}, 1);
        wArg(ida, InstructionName::SHR, "SHR", std::optional<word_t>{1}, 1);
        wArg(ida, InstructionName::INC, "INC", std::optional<word_t>{1}, 1);
        wArg(ida, InstructionName::DEC, "DEC", std::optional<word_t>{1}, 1);
        woArg(ida, InstructionName::NEG, "NEG", 1);
        woArg(ida, InstructionName::SWP, "SWP", 3);
        woArg(ida, InstructionName::ADD, "ADD", 2);
        woArg(ida, InstructionName::SUB, "SUB", 2);
        woArg(ida, InstructionName::AND, "AND", 2);
        woArg(ida, InstructionName::OR , "OR" , 2);
        woArg(ida, InstructionName::XOR, "XOR", 2);
        woArg(ida, InstructionName::GET, "GET", ioPenalty+2);
        woArg(ida, InstructionName::GTC, "GTC", ioPenalty+2);
        woArg(ida, InstructionName::PTU, "PTU", 1+ioPenalty+1);
        woArg(ida, InstructionName::PTS, "PTS", 1+ioPenalty+1);
        woArg(ida, InstructionName::PTB, "PTB", 1+ioPenalty+1);
        woArg(ida, InstructionName::PTC, "PTC", 1+ioPenalty+1);
        woArg(ida, InstructionName::RND, "RND", ioPenalty+2);
        woArg(ida, InstructionName::HLT, "HLT", 1);

        return ida;
    }
}

constexpr std::array<InstructionDefinition, 256> const instructionDefinitions{
    InstructionDefinitionsUtil::build()};

struct Instruction {
    public:
        InstructionName name;
        word_t argument;

        bool operator==(Instruction const& instruction) const {
            return name == instruction.name
                && argument == instruction.argument; }
        bool operator!=(Instruction const& instruction) const {
            return !operator==(instruction); }
};

struct ComputationStateDebug {
    public:
        word_t highestUsedMemoryLocation;
        bool doWaitForUser, doVisualizeSteps;
        std::optional<std::tuple<word_t, word_t>> stackBoundaries;

    public: ComputationStateDebug() :
        highestUsedMemoryLocation{0},
        doWaitForUser{false}, doVisualizeSteps{false},
        stackBoundaries{std::nullopt}
    { ; }
};

struct ComputationStateStatistics {
    uint_t nInstructions, nMicroInstructions;

    ComputationStateStatistics operator-(
        ComputationStateStatistics const&statistics
    ) const {
        return ComputationStateStatistics{
            nInstructions - statistics.nInstructions,
            nMicroInstructions - statistics.nMicroInstructions}; }

    std::string toString() const {
        return "#" + std::to_string(nInstructions) + ": "
            + std::to_string(nMicroInstructions); }
};

#endif
