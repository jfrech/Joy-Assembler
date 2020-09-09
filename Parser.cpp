#ifndef JOY_ASSEMBLER__PARSE_CPP
#define JOY_ASSEMBLER__PARSE_CPP

#include "Includes.hh"

class Parser {
    private:
        typedef uint64_t line_number_t;
        typedef std::tuple<InstructionName, std::optional<std::string>>
            parsingInstruction;
        typedef word_t parsingData;

    private:
        std::set<std::filesystem::path> parsedFilepaths;
        std::vector<std::tuple<std::filesystem::path, line_number_t,
            std::variant<parsingInstruction, parsingData>>> parsing;
        std::map<std::string, std::tuple<line_number_t, std::string>>
            definitions;
        bool stackInstructionWasUsed;
        std::optional<word_t> stackBeginning;
        std::optional<word_t> stackEnd;
        word_t memPtr;

        word_t pragmaMemorySize;
        MemoryMode pragmaMemoryMode;
        std::optional<word_t> pragmaRNGSeed;

        std::map<word_t, std::vector<std::tuple<bool, std::string>>> profiler;

        bool ok;

        Util::rng_t rng;

    public: Parser() :
        parsedFilepaths{},
        parsing{},
        definitions{},
        stackInstructionWasUsed{false},
        stackBeginning{std::nullopt},
        stackEnd{std::nullopt},
        memPtr{0},

        pragmaMemorySize{0x10000},
        pragmaMemoryMode{MemoryMode::LittleEndian},
        pragmaRNGSeed{std::nullopt},

        ok{true},

        rng{}
    { ; }

    public: bool error(
        std::filesystem::path const&filepath, line_number_t const lineNumber,
        std::string const&msg
    ) {
        std::cerr << "file " << filepath << ", ln " << lineNumber << ": "
                  << msg << std::endl;
        return false; }

    public: bool error(std::string const&msg) {
        std::cerr << msg << std::endl;
        return false; }

    public: bool commandlineArg(ComputationState &cs, std::string const&arg) {
        if (arg == "visualize")
            cs.enableVisualization();
        else if (arg == "step")
            cs.enableStepping();
        else if (arg == "cycles")
            cs.enableFinalCycles();
        else
            return err("unknown commandline argument: " + arg);
        return true; }

    public: std::optional<ComputationState> parse(
        std::filesystem::path const&filepath
    ) {
        if (!parseFiles(filepath))
            return std::nullopt;

        if (!pragmas(filepath))
            return std::nullopt;

        std::optional<ComputationState> oCS{
            std::in_place, pragmaMemorySize, pragmaMemoryMode, rng, profiler};

        if (!parseAssemble(oCS.value()))
            return std::nullopt;

        return oCS; }

    private: bool err(std::string const&msg) {
        std::cerr << "Parser: " << msg << std::endl;
        return ok = false; }

    private: bool parseFiles(std::filesystem::path const&filepath) {
        if (filepath != filepath.lexically_normal())
            return parseFiles(filepath.lexically_normal());

        if (!is_regular_file(filepath))
            return error("not a regular file");
        if (Util::std20::contains(parsedFilepaths, filepath))
            return error("recursive file inclusion; not parsing file twice");
        parsedFilepaths.insert(filepath);

        std::ifstream is{filepath};
        if (!is.is_open())
            return error("unable to read file");

        std::string const regexIdentifier{"[.$_[:alpha:]-][.$_[:alnum:]-]*"};
        std::string const regexValue{"[@.$'_[:alnum:]+-][.$'_[:alnum:]\\\\-]*"};
        std::string const regexString{"\"([^\"]|\\\")*?\""};

        line_number_t lineNumber{1};
        std::string ln{};
        for (; std::getline(is, ln); ++lineNumber) {
            auto pushData = [&](uint32_t const data) {
                log("pushing data: 0x" + Util::UInt32AsPaddedHex(data));
                {
                    parsing.push_back(std::make_tuple(filepath, lineNumber,
                        parsingData{data}));
                    memPtr += 4;
                }
            };

            ln = std::regex_replace(ln, std::regex{";.*$"}, "");
            ln = std::regex_replace(ln, std::regex{"\\s+"}, " ");
            ln = std::regex_replace(ln, std::regex{"^ +"}, "");
            ln = std::regex_replace(ln, std::regex{" +$"}, "");

            if (ln == "")
                continue;

            log("ln " + std::to_string(lineNumber) + ": " + ln);

            auto define = [&](std::string k, std::string v)
            {
                log("defining " + k + " to be " + v);
                if (Util::std20::contains(definitions, k))
                    return error(filepath, lineNumber,
                        "duplicate definition: " + k);
                definitions[k] = std::make_tuple(lineNumber, v);
                return true;
            };

            #define ON_MATCH(REGEX, LAMBDA) { \
                std::smatch smatch{}; \
                if (std::regex_match(ln, smatch, std::regex{(REGEX)})) { \
                    if (!(LAMBDA)(smatch)) \
                        return false; \
                    continue; }}

            ON_MATCH("^(" + regexIdentifier + ") ?:= ?(" + regexValue + ")$", (
                [define](std::smatch const&smatch) {
                    return define(std::string{smatch[1]},
                        std::string{smatch[2]}); }))

            ON_MATCH("^(" + regexIdentifier + "):$", (
                [&](std::smatch const&smatch) {
                    std::string label{smatch[1]};
                    if (!define("@" + label, std::to_string(memPtr)))
                        return false;
                    if (label == "stack" && !stackBeginning.has_value())
                        stackBeginning = std::make_optional(memPtr);
                    return true; }))

            /* TODO :: too wide */
            ON_MATCH("^data ?(.+)$", (
                [&](std::smatch const&smatch) {
                    log("parsing `data` ...");
                    std::string commaSeparated{std::string{smatch[1]} + ","};
                    for (uint64_t elementNumber{1}; commaSeparated != ""; ++elementNumber) {
                        auto dataError = [&](std::string const&msg, std::string const&detail) {
                            return error(filepath, lineNumber, msg + " (element number " + std::to_string(elementNumber) + "): " + detail); };

                        std::smatch smatch{};
                        if (std::regex_match(commaSeparated, smatch, std::regex{"^(" + ("(\\[(" + regexValue + ")\\])? ?(" + regexValue + "|" + "runif " + regexValue + "|" + "rperm" + ")?") + "|" + regexString + ") ?, ?(.*)$"})) {

                            log("smatch of size " + std::to_string(smatch.size()));
                            for (auto j = 0u; j < smatch.size(); ++j)
                                log("smatch[" + std::to_string(j) + "]: @@@" + std::string{smatch[j]} + "@@@");

                            std::string unparsedElement{smatch[1]};
                            std::string unparsedSize{smatch[3]};
                            std::string unparsedValue{smatch[4]};
                            commaSeparated = std::string{smatch[6]};

                            if (unparsedElement == "")
                                return dataError("invalid data element", "empty element");

                            if (unparsedElement.front() == '\"') {
                                log("parsing string: " + unparsedElement);
                                std::optional<std::vector<UTF8::rune_t>> oRunes = Util::parseString(unparsedElement);
                                if (!oRunes.has_value())
                                    return dataError("invalid data string element", unparsedElement);
                                for (UTF8::rune_t rune : oRunes.value())
                                    pushData(static_cast<word_t>(rune));
                                continue;
                            }

                            log("parsing non-string: " + unparsedElement);

                            if (unparsedSize == "")
                                unparsedSize = std::string{"1"};
                            std::optional<word_t> oSize{Util::stringToUInt32(unparsedSize)};
                            if (!oSize.has_value())
                                return dataError("invalid data uint element size", unparsedSize);
                            log("    ~> size: " + std::to_string(oSize.value()));

                            {
                                std::smatch smatch{};
                                if (std::regex_match(unparsedValue, smatch, std::regex{"^runif (" + regexValue + ")$"})) {
                                    unparsedValue = std::string{smatch[1]};
                                    std::optional<word_t> oValue{Util::stringToUInt32(unparsedValue)};
                                    if (!oValue.has_value())
                                        return dataError("invalid data unif range value", unparsedValue);

                                    for (word_t r : rng.unif(oSize.value(), oValue.value()))
                                        pushData(r);
                                    continue;
                                }
                                if (std::regex_match(unparsedValue, smatch, std::regex{"^rperm$"})) {
                                    for (word_t r : rng.perm(oSize.value()))
                                        pushData(r);
                                    continue;
                                }
                            }

                            log("parsing uint: " + unparsedElement);

                            if (unparsedValue == "")
                                unparsedValue = std::string{"0"};
                            std::optional<word_t> oValue{Util::stringToUInt32(unparsedValue)};
                            if (!oValue.has_value())
                                return dataError("invalid data uint element value", unparsedValue);
                            log("    ~> value: " + std::to_string(oValue.value()));

                            for (word_t j = 0; j < oSize.value(); ++j)
                                pushData(oValue.value());
                            continue;
                        }
                        return dataError("incomprehensible data element trunk", commaSeparated);
                    }
                    return true; }))

            ON_MATCH("^include ?(" + regexString + ")$", (
                [&](std::smatch const&smatch) {
                    std::optional<std::vector<UTF8::rune_t>> oIncludeFilepathRunes{
                        Util::parseString(std::string{smatch[1]})};
                    if (!oIncludeFilepathRunes.has_value())
                        return error(filepath, lineNumber, "malformed utf-8 include string: " + std::string{smatch[1]});

                    std::optional<std::string> oIncludeFilepath{
                        UTF8::runeVectorToOptionalString(oIncludeFilepathRunes.value())};
                    if (!oIncludeFilepath.has_value())
                        return error(filepath, lineNumber, "malformed include string (contains non-7-bit ASCII): " + std::string{smatch[1]});

                    std::filesystem::path const includeFilepath{
                        filepath.parent_path() / oIncludeFilepath.value()};

                    bool success{};
                    log("including with memPtr = " + std::to_string(memPtr));
                    success = parseFiles(includeFilepath);
                    log("included with memPtr = " + std::to_string(memPtr));

                    if (!success)
                        return error(filepath, lineNumber, "could not include "
                            "file: " + std::string{includeFilepath});
                    return true; }))

            ON_MATCH("^include.*$", (
                [&](std::smatch const&_) {
                    (void) _;
                    return error(filepath, lineNumber,
                        "improper include: either empty or missing quotes"); }))

            ON_MATCH("^profiler ([^ ]*?)(, ?(.*))?$", (
                [&](std::smatch const&smatch) {
                    std::string const profilerStartStop{smatch[1]};
                    if (profilerStartStop != "start" && profilerStartStop != "stop")
                        return error(filepath, lineNumber, "invalid profiler "
                            "directive (must be 'start' or 'stop'): "
                            + profilerStartStop);

                    std::string profilerMessage{smatch[3]};
                    profilerMessage = "file " + std::string{filepath} + ", ln "
                        + std::to_string(lineNumber)
                        + std::string{profilerMessage == "" ? "" : ": "} + profilerMessage;

                    if (!Util::std20::contains(profiler, memPtr))
                        profiler[memPtr] = std::vector<std::tuple<bool, std::string>>{};
                    profiler[memPtr].push_back(std::make_tuple(profilerStartStop == std::string{"start"}, profilerMessage));

                    return true; }))

            ON_MATCH("^(" + regexIdentifier + ")( (" + regexValue + "))?$", (
                [&](std::smatch const&smatch) {
                    auto oName = InstructionNameRepresentationHandler
                        ::from_string(smatch[1]);
                    if (!oName.has_value())
                        return error(filepath, lineNumber, "invalid "
                            "instruction name: " + std::string{smatch[1]});
                    std::optional<std::string> oArg{std::nullopt};
                    if (smatch[3] != "")
                        oArg = std::optional{smatch[3]};
                    log("pushing instruction: "
                        + InstructionNameRepresentationHandler::to_string(oName.value())
                        + " " + oArg.value_or("(no arg.)"));
                    {
                        parsing.push_back(std::make_tuple(filepath, lineNumber,
                            parsingInstruction{std::make_tuple(oName.value(), oArg)}));
                        memPtr += 5;
                    }
                    return true; }))

            #undef ON_MATCH

            return error(filepath, lineNumber, "incomprehensible");
        }

        return true;
    }

    private: bool pragmas(std::filesystem::path const&filepath) {
        std::map<std::string, std::function<
            bool(line_number_t, std::string const&)>> const&pragmaActions
        {
            {"pragma_memory-mode", [&](line_number_t lineNumber, std::string const&mm) {
                if (mm == "little-endian") {
                    pragmaMemoryMode = MemoryMode::LittleEndian;
                    return true; }
                if (mm == "big-endian") {
                    pragmaMemoryMode = MemoryMode::BigEndian;
                    return true; }
                return error(filepath, lineNumber, "invalid pragma_memory-mode: " + mm); }},

            {"pragma_memory-size", [&](line_number_t lineNumber, std::string const&ms) {
                std::optional<word_t> oMemorySize{Util::stringToUInt32(ms)};
                if (!oMemorySize.has_value())
                    return error(filepath, lineNumber, "invalid pragma_memory-size: " + ms);
                pragmaMemorySize = oMemorySize.value();
                return true; }},

            {"pragma_rng-seed", [&](line_number_t lineNumber, std::string const&rs) {
                std::optional<word_t> oRNGSeed{Util::stringToUInt32(rs)};
                if (!oRNGSeed.has_value())
                    return error(filepath, lineNumber, "invalid pragma_rng-seed: " + rs);
                pragmaRNGSeed = std::make_optional(oRNGSeed.value());
                return true; }},
        };

        for (auto [pragma, action] : pragmaActions)
            if (Util::std20::contains(definitions, pragma)) {
                auto [lineNumber, definition] = definitions[pragma];
                if (!action(lineNumber, definition))
                    return false; }

        if (pragmaRNGSeed.has_value())
            rng.seed(pragmaRNGSeed.value());

        return true;
    }

    private: bool parseAssemble(ComputationState &cs) {
        log("@@@ parseAssemble @@@");

        bool memPtrGTStackBeginningAndNonDataOccurred{false};
        bool haltInstructionWasUsed{false};

        word_t memPtr{0};
        for (auto const&[filepath, lineNumber, p] : parsing) {
            if (std::holds_alternative<parsingData>(p)) {
                word_t data{std::get<parsingData>(p)};
                log("data value 0x" + Util::UInt32AsPaddedHex(data));
                memPtr += cs.storeData(memPtr, data);

                if (!memPtrGTStackBeginningAndNonDataOccurred)
                    stackEnd = std::make_optional(memPtr);
            }
            else if (std::holds_alternative<parsingInstruction>(p)) {
                if (memPtr > stackBeginning)
                    memPtrGTStackBeginningAndNonDataOccurred = true;

                parsingInstruction instruction{
                    std::get<parsingInstruction>(p)};
                InstructionName name = std::get<0>(instruction);
                auto oArg = std::get<1>(instruction);
                std::optional<word_t> oValue{std::nullopt};
                if (oArg.has_value()) {
                    if (Util::std20::contains(definitions, oArg.value())) {
                        auto [_, definition] = definitions[oArg.value()];
                        oArg = std::make_optional(definition); }

                    if (oArg.value() == "")
                        return error(filepath, lineNumber, "no instruction argument");
                    if (oArg.value().front() == '@') {
                        std::string label{oArg.value()};
                        label.erase(label.begin());

                        std::vector<std::string> labels{};
                        for (auto const&[lbl, _] : definitions)
                            labels.push_back(lbl);
                        labels = Util::sortByLevenshteinDistanceTo(labels, label);
                        std::string msg{"label @" + label + " was not defined; "
                            + "did you possibly mean one of the following defined "
                            + "labels?"};
                        for (std::size_t j = 0; j < labels.size() && j < 3; ++j)
                            msg += "\n    " + std::to_string(j+1) + ") "
                                + labels[j];
                        if (labels.size() <= 0)
                            msg += "\n    (no labels have been defined)";
                        return error(filepath, lineNumber, msg);
                    }


                    if (oArg.value().front() == '\'') {
                        if (oArg.value().size() < 2 || oArg.value().back() != '\'')
                            return error(filepath, lineNumber, "invalid character literal");
                        std::string s{oArg.value()};
                        s.front() = s.back() = '"';
                        std::optional<std::vector<UTF8::rune_t>> oRunes{Util::parseString(s)};
                        if (oRunes.has_value() && oRunes.value().size() != 1)
                            oRunes = std::nullopt;
                        if (!oRunes.has_value())
                            return error(filepath, lineNumber, "invalid character literal");
                        oValue = static_cast<std::optional<word_t>>(std::make_optional(oRunes.value().at(0)));
                    } else {
                        oValue = static_cast<std::optional<word_t>>(
                            Util::stringToUInt32(oArg.value()));
                        if (!oValue.has_value())
                            return error(filepath, lineNumber, "invalid argument value: "
                                + oArg.value());
                    }
                }

                auto [hasArgument, optionalValue] =
                    InstructionNameRepresentationHandler::argumentType[name];
                if (!hasArgument && oValue.has_value())
                    return error(filepath, lineNumber, "superfluous argument: "
                        + InstructionNameRepresentationHandler::to_string(name)
                        + " " + std::to_string(oValue.value()));
                if (hasArgument) {
                    if (!optionalValue.has_value() && !oValue.has_value())
                        return error(filepath, lineNumber, "requiring argument: "
                            + InstructionNameRepresentationHandler
                              ::to_string(name));
                    if (!oValue.has_value())
                        oValue = optionalValue; }

                if (oValue.has_value()) {
                    log("instruction " + InstructionNameRepresentationHandler
                        ::to_string(name) + " " + std::to_string(oValue.value()));
                    auto argument = oValue.value();
                    memPtr += cs.storeInstruction(memPtr, Instruction{
                        name, argument});
                } else {
                    log("instruction " + InstructionNameRepresentationHandler
                        ::to_string(name) + " (none)");
                    memPtr += cs.storeInstruction(memPtr, Instruction{
                        name, 0x00000000}); }

                haltInstructionWasUsed |= InstructionName::HLT == name;
                stackInstructionWasUsed |=
                    InstructionNameRepresentationHandler::isStackInstruction(name);
            }
            else
                return error("internal error: parsing piece holds invalid alternative");
        }

        if (!haltInstructionWasUsed)
            return error("no halt instruction was used");

        if (stackInstructionWasUsed)
            if (!Util::std20::contains(definitions, std::string{"@stack"}))
                return error("stack instructions are used yet no stack was "
                    "defined");

        if (stackBeginning.has_value() != stackEnd.has_value())
            return error("inconsistent stack boundaries");

        if (stackBeginning.has_value() && stackEnd.has_value()) {
            log("got as stack boundaries: "
                + std::to_string(stackBeginning.value()) + " and "
                + std::to_string(stackEnd.value()));

            cs.initializeStack(stackBeginning.value(), stackEnd.value());
        } else
            log("no stack was defined");

        return true;
    }
};

#endif
