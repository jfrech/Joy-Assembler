#ifndef JOY_ASSEMBLER__LOG_CPP
#define JOY_ASSEMBLER__LOG_CPP

bool constexpr doLog{false};

void log(std::string const&msg) {
    if (doLog)
        std::clog << msg << std::endl; }

#endif
