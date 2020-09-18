#ifndef JOY_ASSEMBLER__TYPES_HH
#define JOY_ASSEMBLER__TYPES_HH

typedef uint8_t byte_t;
typedef uint32_t word_t;

enum class MemoryMode : bool { LittleEndian, BigEndian };
enum class MemorySemantic : uint8_t {
    InstructionHead, Instruction, DataHead, Data };

/* TODO :: finalize op-codes */
#define NO_ARG std::tuple{false, std::nullopt}
#define REQ_ARG std::tuple{true, std::nullopt}
#define OPT_ARG(ARG) std::tuple{true, std::optional<word_t>{ARG}}
#define MAP_ON_INSTRUCTION_NAMES(M) \
    M(NOP, OPT_ARG(0)), \
    \
    M(LDA, REQ_ARG), M(LDB, REQ_ARG), M(STA, REQ_ARG), M(STB, REQ_ARG), \
    M(LIA, OPT_ARG(0)), M(SIA, OPT_ARG(0)), M(LPC, NO_ARG), M(SPC, NO_ARG), \
    M(LYA, REQ_ARG), M(SYA, REQ_ARG), \
    \
    M(JMP, REQ_ARG), M(JN, REQ_ARG), M(JNN, REQ_ARG), M(JZ, REQ_ARG), \
    M(JNZ, REQ_ARG), M(JP, REQ_ARG), M(JNP, REQ_ARG), \
    M(JE, REQ_ARG), M(JNE, REQ_ARG), \
    \
    M(CAL, REQ_ARG), M(RET, NO_ARG), M(PSH, NO_ARG), M(POP, NO_ARG), \
    M(LSA, OPT_ARG(0)), M(SSA, OPT_ARG(0)), M(LSC, NO_ARG), M(SSC, NO_ARG), \
    \
    M(MOV, REQ_ARG), M(NOT, NO_ARG), M(SHL, OPT_ARG(1)), M(SHR, OPT_ARG(1)), \
    M(INC, OPT_ARG(1)), M(DEC, OPT_ARG(1)), M(NEG, NO_ARG), \
    \
    M(SWP, NO_ARG), M(ADD, NO_ARG), M(SUB, NO_ARG), M(AND, NO_ARG), \
    M(OR, NO_ARG), M(XOR, NO_ARG), \
    \
    M(GET, NO_ARG), M(GTC, NO_ARG), \
    M(PTU, NO_ARG), M(PTS, NO_ARG), M(PTB, NO_ARG), M(PTC, NO_ARG), \
    \
    M(RND, NO_ARG), \
    \
    M(HLT, NO_ARG),


enum class InstructionName : byte_t {
    #define PROJECTION(ACT, _) ACT
    MAP_ON_INSTRUCTION_NAMES(PROJECTION)
    #undef PROJECTION
};

/******************************************************************************/
struct InstructionDefinition {
    bool opCodeUsed{false};

    InstructionName name{InstructionName::NOP};
    std::string_view nameRepresentation{"(unused instruction)"};
    bool requiresArgument{false};
    std::optional<word_t> optionalArgument{std::nullopt};
    uint64_t microInstructions{0};
};

constexpr InstructionDefinition defineInstruction (
    InstructionName const name, std::string_view const&nameRepresentation,
    uint64_t const microInstructions
) {
    return InstructionDefinition{true, name, nameRepresentation, false,
        std::nullopt, microInstructions};
}

constexpr InstructionDefinition defineInstruction (
    InstructionName const name, std::string_view const&nameRepresentation,
    std::optional<word_t> const&optionalArgument,
    uint64_t const microInstructions
) {
    return InstructionDefinition{true, name, nameRepresentation,
        optionalArgument == std::nullopt, optionalArgument, microInstructions};
}

std::array<InstructionDefinition, 256> constexpr instructionDefinitions{[]() {;
    std::array<InstructionDefinition, 256> defs{};
    uint8_t opCode{0};
    defs[opCode++] = defineInstruction(InstructionName::NOP, "NOP", std::optional<word_t>{0}, 1);
    defs[opCode++] = defineInstruction(InstructionName::LDA, "LDA", std::nullopt, 4);
    defs[opCode++] = defineInstruction(InstructionName::LDB, "LDB", std::nullopt, 4);
    defs[opCode++] = defineInstruction(InstructionName::STA, "STA", std::nullopt, 4);
    defs[opCode++] = defineInstruction(InstructionName::STB, "STB", std::nullopt, 4);
    defs[opCode++] = defineInstruction(InstructionName::LIA, "LIA", std::optional<word_t>{0}, 6);
    defs[opCode++] = defineInstruction(InstructionName::SIA, "SIA", std::optional<word_t>{0}, 6);
    defs[opCode++] = defineInstruction(InstructionName::LPC, "LPC", 2);
    defs[opCode++] = defineInstruction(InstructionName::SPC, "SPC", 2);
    defs[opCode++] = defineInstruction(InstructionName::LYA, "LYA", std::nullopt, 4);
    defs[opCode++] = defineInstruction(InstructionName::SYA, "SYA", std::nullopt, 4);
    defs[opCode++] = defineInstruction(InstructionName::JMP, "JMP", std::nullopt, 2);
    defs[opCode++] = defineInstruction(InstructionName::JN , "JN" , std::nullopt, 2);
    defs[opCode++] = defineInstruction(InstructionName::JNN, "JNN", std::nullopt, 2);
    defs[opCode++] = defineInstruction(InstructionName::JZ , "JZ" , std::nullopt, 2);
    defs[opCode++] = defineInstruction(InstructionName::JNZ, "JNZ", std::nullopt, 2);
    defs[opCode++] = defineInstruction(InstructionName::JP , "JP" , std::nullopt, 2);
    defs[opCode++] = defineInstruction(InstructionName::JNP, "JNP", std::nullopt, 2);
    defs[opCode++] = defineInstruction(InstructionName::JE , "JE" , std::nullopt, 2);
    defs[opCode++] = defineInstruction(InstructionName::JNE, "JNE", std::nullopt, 2);
    defs[opCode++] = defineInstruction(InstructionName::CAL, "CAL", std::nullopt, 11);
    defs[opCode++] = defineInstruction(InstructionName::RET, "RET", 9);
    defs[opCode++] = defineInstruction(InstructionName::PSH, "PSH", 9);
    defs[opCode++] = defineInstruction(InstructionName::POP, "POP", 9);
    defs[opCode++] = defineInstruction(InstructionName::LSA, "LSA", std::optional<word_t>{0}, 6);
    defs[opCode++] = defineInstruction(InstructionName::SSA, "SSA", std::optional<word_t>{0}, 6);
    defs[opCode++] = defineInstruction(InstructionName::LSC, "LSC", 2);
    defs[opCode++] = defineInstruction(InstructionName::SSC, "SSC", 2);
    defs[opCode++] = defineInstruction(InstructionName::MOV, "MOV", std::nullopt, 2);
    defs[opCode++] = defineInstruction(InstructionName::NOT, "NOT", 1);
    defs[opCode++] = defineInstruction(InstructionName::SHL, "SHL", std::optional<word_t>{1}, 1);
    defs[opCode++] = defineInstruction(InstructionName::SHR, "SHR", std::optional<word_t>{1}, 1);
    defs[opCode++] = defineInstruction(InstructionName::INC, "INC", std::optional<word_t>{1}, 1);
    defs[opCode++] = defineInstruction(InstructionName::DEC, "DEC", std::optional<word_t>{1}, 1);
    defs[opCode++] = defineInstruction(InstructionName::NEG, "NEG", 1);
    defs[opCode++] = defineInstruction(InstructionName::SWP, "SWP", 6);
    defs[opCode++] = defineInstruction(InstructionName::AND, "AND", 2);
    defs[opCode++] = defineInstruction(InstructionName::OR , "OR" , 2);
    defs[opCode++] = defineInstruction(InstructionName::XOR, "XOR", 2);
    defs[opCode++] = defineInstruction(InstructionName::ADD, "ADD", 2);
    defs[opCode++] = defineInstruction(InstructionName::SUB, "SUB", 2);
    uint64_t constexpr ioPenalty{32};
    defs[opCode++] = defineInstruction(InstructionName::PTU, "PTU", 1+ioPenalty+1);
    defs[opCode++] = defineInstruction(InstructionName::PTS, "PTS", 1+ioPenalty+1);
    defs[opCode++] = defineInstruction(InstructionName::PTB, "PTB", 1+ioPenalty+1);
    defs[opCode++] = defineInstruction(InstructionName::PTC, "PTC", 1+ioPenalty+1);
    defs[opCode++] = defineInstruction(InstructionName::GET, "GET", ioPenalty+2);
    defs[opCode++] = defineInstruction(InstructionName::GTC, "GTC", ioPenalty+2);
    defs[opCode++] = defineInstruction(InstructionName::RND, "RND", ioPenalty+2);
    defs[opCode++] = defineInstruction(InstructionName::HLT, "HLT", 1);
    return defs;
}()};
/******************************************************************************/

struct Instruction {
    public:
        InstructionName name;
        word_t argument;

    public:
        bool operator==(Instruction const& instruction) const {
            return name == instruction.name && argument == instruction.argument; }
        bool operator!=(Instruction const& instruction) const {
            return !operator==(instruction); }
};

namespace InstructionNameRepresentationHandler {
   std::array<std::optional<InstructionName>, 256> instructions = {
        #define NAMESPACE_PROJECTION(ACT, _) InstructionName::ACT
        MAP_ON_INSTRUCTION_NAMES(NAMESPACE_PROJECTION)
        #undef NAMESPACE_PROJECTION
    };// */

    std::map<InstructionName, std::tuple<bool, std::optional<word_t>>>
        argumentType = {
            #define ARG_TYPES(ACT, CANDEF) \
                {InstructionName::ACT, CANDEF}
            MAP_ON_INSTRUCTION_NAMES(ARG_TYPES)
            #undef ARG_TYPES
    };// */
}

#undef NO_ARG
#undef REQ_ARG
#undef OPT_ARG
#undef MAP_ON_INSTRUCTION_NAMES

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

    ComputationStateStatistics operator-(ComputationStateStatistics const&statistics) const {
        return ComputationStateStatistics{nInstructions - statistics.nInstructions, nMicroInstructions - statistics.nMicroInstructions}; }

    std::string toString() const {
        return "#" + std::to_string(nInstructions) + ": " + std::to_string(nMicroInstructions); }
};


#endif
