#ifndef JOY_ASSEMBLER__TYPES_HH
#define JOY_ASSEMBLER__TYPES_HH

#include <bitset>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <regex>
#include <set>
#include <variant>
#include <vector>

typedef uint8_t byte_t;
typedef uint32_t word_t;

enum class MemoryMode { LittleEndian, BigEndian };

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
    M(JMP, REQ_ARG), M(JZ, REQ_ARG), M(JNZ, REQ_ARG), M(JN, REQ_ARG), \
    M(JNN, REQ_ARG), M(JE, REQ_ARG), M(JNE, REQ_ARG), \
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

enum class InstructionName {
    #define PROJECTION(ACT, _) ACT
    MAP_ON_INSTRUCTION_NAMES(PROJECTION)
    #undef PROJECTION
};

struct Instruction { InstructionName name; word_t argument; };

namespace InstructionNameRepresentationHandler {
    std::map<InstructionName, std::string> representation = {
        #define REPR(ACT, _) {InstructionName::ACT, #ACT}
        MAP_ON_INSTRUCTION_NAMES(REPR)
        #undef REPR
    };
    std::array<std::optional<InstructionName>, 256> instructions = {
        #define NAMESPACE_PROJECTION(ACT, _) InstructionName::ACT
        MAP_ON_INSTRUCTION_NAMES(NAMESPACE_PROJECTION)
        #undef NAMESPACE_PROJECTION
    };

    std::map<InstructionName, std::tuple<bool, std::optional<word_t>>>
        argumentType = {
            #define ARG_TYPES(ACT, CANDEF) \
                {InstructionName::ACT, CANDEF}
            MAP_ON_INSTRUCTION_NAMES(ARG_TYPES)
            #undef ARG_TYPES
    };
}

#undef NO_ARG
#undef REQ_ARG
#undef OPT_ARG
#undef MAP_ON_INSTRUCTION_NAMES

struct ComputationStateDebug {
    public:
        word_t highestUsedMemoryLocation;
        uint64_t executionCycles;
        bool doWaitForUser, doVisualizeSteps, doShowFinalCycles;
        std::optional<std::tuple<word_t, word_t>> stackBoundaries;

    public: ComputationStateDebug() :
        highestUsedMemoryLocation{0},
        executionCycles{0},
        doWaitForUser{false}, doVisualizeSteps{false}, doShowFinalCycles{false},
        stackBoundaries{std::nullopt}
    { ; }
};

#endif
