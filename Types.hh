#ifndef JOY_ASSEMBLER__TYPES_HH
#define JOY_ASSEMBLER__TYPES_HH

typedef uint8_t byte_t;
typedef uint32_t word_t;

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
        uint64_t microInstructions{0};
};

constexpr std::array<InstructionDefinition, 256> const instructionDefinitions{
    []() {
    static_assert(std::is_same<
        std::underlying_type<InstructionName>::type, byte_t>::value);
    std::array<InstructionDefinition, 256> defs{};

    uint64_t const ioPenalty{32};

    auto const withArgument{[&defs](
        InstructionName const name, std::string_view const&nameRepresentation,
        std::optional<word_t> const&optionalArgument,
        uint64_t const microInstructions
    ) {
        defs[static_cast<std::underlying_type<InstructionName>::type>(name)] =
            InstructionDefinition{true, name, nameRepresentation,
                optionalArgument == std::nullopt, optionalArgument,
                microInstructions}; }};

    auto const withoutArgument{[&defs](
        InstructionName const name, std::string_view const&nameRepresentation,
        uint64_t const microInstructions
    ) {
        defs[static_cast<std::underlying_type<InstructionName>::type>(name)]
            = InstructionDefinition{true, name, nameRepresentation, false,
                std::nullopt, microInstructions}; }};

    withArgument(InstructionName::NOP, "NOP", std::optional<word_t>{0}, 1);
    withArgument(InstructionName::LDA, "LDA", std::nullopt, 4);
    withArgument(InstructionName::LDB, "LDB", std::nullopt, 4);
    withArgument(InstructionName::STA, "STA", std::nullopt, 4);
    withArgument(InstructionName::STB, "STB", std::nullopt, 4);
    withArgument(InstructionName::LIA, "LIA", std::optional<word_t>{0}, 6);
    withArgument(InstructionName::SIA, "SIA", std::optional<word_t>{0}, 6);
    withoutArgument(InstructionName::LPC, "LPC", 2);
    withoutArgument(InstructionName::SPC, "SPC", 2);
    withArgument(InstructionName::LYA, "LYA", std::nullopt, 4);
    withArgument(InstructionName::SYA, "SYA", std::nullopt, 4);
    withArgument(InstructionName::JMP, "JMP", std::nullopt, 2);
    withArgument(InstructionName::JN , "JN" , std::nullopt, 2);
    withArgument(InstructionName::JNN, "JNN", std::nullopt, 2);
    withArgument(InstructionName::JZ , "JZ" , std::nullopt, 2);
    withArgument(InstructionName::JNZ, "JNZ", std::nullopt, 2);
    withArgument(InstructionName::JP , "JP" , std::nullopt, 2);
    withArgument(InstructionName::JNP, "JNP", std::nullopt, 2);
    withArgument(InstructionName::JE , "JE" , std::nullopt, 2);
    withArgument(InstructionName::JNE, "JNE", std::nullopt, 2);
    withArgument(InstructionName::CAL, "CAL", std::nullopt, 11);
    withoutArgument(InstructionName::RET, "RET", 9);
    withoutArgument(InstructionName::PSH, "PSH", 9);
    withoutArgument(InstructionName::POP, "POP", 9);
    withArgument(InstructionName::LSA, "LSA", std::optional<word_t>{0}, 6);
    withArgument(InstructionName::SSA, "SSA", std::optional<word_t>{0}, 6);
    withoutArgument(InstructionName::LSC, "LSC", 2);
    withoutArgument(InstructionName::SSC, "SSC", 2);
    withArgument(InstructionName::MOV, "MOV", std::nullopt, 2);
    withoutArgument(InstructionName::NOT, "NOT", 1);
    withArgument(InstructionName::SHL, "SHL", std::optional<word_t>{1}, 1);
    withArgument(InstructionName::SHR, "SHR", std::optional<word_t>{1}, 1);
    withArgument(InstructionName::INC, "INC", std::optional<word_t>{1}, 1);
    withArgument(InstructionName::DEC, "DEC", std::optional<word_t>{1}, 1);
    withoutArgument(InstructionName::NEG, "NEG", 1);
    withoutArgument(InstructionName::SWP, "SWP", 6);
    withoutArgument(InstructionName::ADD, "ADD", 2);
    withoutArgument(InstructionName::SUB, "SUB", 2);
    withoutArgument(InstructionName::AND, "AND", 2);
    withoutArgument(InstructionName::OR , "OR" , 2);
    withoutArgument(InstructionName::XOR, "XOR", 2);
    withoutArgument(InstructionName::GET, "GET", ioPenalty+2);
    withoutArgument(InstructionName::GTC, "GTC", ioPenalty+2);
    withoutArgument(InstructionName::PTU, "PTU", 1+ioPenalty+1);
    withoutArgument(InstructionName::PTS, "PTS", 1+ioPenalty+1);
    withoutArgument(InstructionName::PTB, "PTB", 1+ioPenalty+1);
    withoutArgument(InstructionName::PTC, "PTC", 1+ioPenalty+1);
    withoutArgument(InstructionName::RND, "RND", ioPenalty+2);
    withoutArgument(InstructionName::HLT, "HLT", 1);
    return defs; }()};

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
    uint64_t nInstructions, nMicroInstructions;

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
