#ifndef JOY_ASSEMBLER__PARSE_CPP
#define JOY_ASSEMBLER__PARSE_CPP

#include "Includes.hpp"

class Parser {
    private:
        using parsingInstruction = std::tuple<
            InstructionName, std::optional<std::string>>;
        using parsingData = word_t;

    private:
        std::set<std::filesystem::path> parsedFilepaths;
        std::vector<std::tuple<std::filesystem::path, uint_t,
            std::variant<parsingInstruction, parsingData>>> parsing;
        std::map<std::string, std::tuple<uint_t, std::string>>
            definitions;
        bool stackInstructionWasUsed;
        std::optional<word_t> stackBeginning;
        std::optional<word_t> stackEnd;

        word_t memorySize;
        bool memoryIsDynamic;

        MemoryMode pragmaMemoryMode;
        std::optional<word_t> pragmaRNGSeed;
        bool pragmaStaticProgram;
        bool pragmaStaticStackCheck;

        std::vector<std::vector<std::tuple<bool, std::string>>> profiler;
        bool embedProfilerOutput;

        std::optional<std::vector<MemorySemantic>> oMemorySemantics;

        Util::rng_t rng;

    public: Parser() :
        parsedFilepaths{},
        parsing{},
        definitions{},
        stackInstructionWasUsed{false},
        stackBeginning{std::nullopt},
        stackEnd{std::nullopt},

        memorySize{/*TODO possibly odd gcc diagnostic when writing `-1`*/},
        memoryIsDynamic{false},

        pragmaMemoryMode{MemoryMode::LittleEndian},
        pragmaRNGSeed{std::nullopt},
        pragmaStaticProgram{true},
        pragmaStaticStackCheck{true},

        profiler{},
        embedProfilerOutput{false},

        oMemorySemantics{std::nullopt},

        rng{}
    { ; }

    public: bool error(
        std::filesystem::path const&filepath, uint_t const lineNumber,
        std::string const&msg
    ) const {
        std::cerr << "parsing: file " << filepath << ", ln " << lineNumber
                  << ": " << msg << std::endl;
        return false; }

    public: bool error(std::string const&msg) const {
        std::cerr << msg << std::endl;
        return false; }

    public: bool commandlineArg(ComputationState &cs, std::string const&arg) {
        if (arg == "visualize")
            cs.debug.doVisualizeSteps = true;
        else if (arg == "step") {
            cs.debug.doVisualizeSteps = true;
            cs.debug.doWaitForUser = true; }
        else
            return error("unknown commandline argument: " + arg);
        return true; }

    public: std::optional<ComputationState> parse(
        std::filesystem::path const&filepath
    ) {
        word_t memPtr{0};
        if (!parseFiles(filepath, memPtr))
            return std::nullopt;

        if (!pragmas(filepath))
            return std::nullopt;

        oMemorySemantics = constructMemorySemantics();

        std::optional<ComputationState> oCS{std::in_place,
            memorySize, memoryIsDynamic, pragmaMemoryMode, rng, profiler,
            embedProfilerOutput, oMemorySemantics};

        if (!parseAssemble(oCS.value()))
            return std::nullopt;

        return oCS; }

    private: bool parseFiles(
        std::filesystem::path const&filepath, word_t &memPtr
    ) {
        if (filepath != filepath.lexically_normal())
            return parseFiles(filepath.lexically_normal(), memPtr);

        if (!std::filesystem::exists(filepath))
            return error("file does not exist: " + filepath.u8string());
        if (!std::filesystem::is_regular_file(filepath))
            return error("not a regular file: " + filepath.u8string());
        if (Util::std20::contains(parsedFilepaths, filepath))
            return error("recursive file inclusion; not parsing file twice: "
                + filepath.u8string());
        parsedFilepaths.insert(filepath);

        std::ifstream f{filepath};
        if (!f.is_open())
            return error("unable to read file: " + filepath.u8string());

        std::string const regexIdentifier{"[[:alpha:]_][[:alnum:]_$-]*"};
        std::string const regexValue{"['@[:alnum:]+-][^,]*"};
        std::string const regexString{"\"([^\"]|\\\")*?\""};

        /***/

        uint_t lineNumber{1};
        std::string ln{};

        auto const pushData{[&](uint32_t const data) {
            parsing.push_back(std::make_tuple(filepath, lineNumber,
                parsingData{data}));
            memPtr += 4;
        }};

        auto const pushInstruction{[&](
            InstructionName const name, std::optional<std::string> const&oArg
        ) {
            parsing.push_back(std::make_tuple(filepath, lineNumber,
                parsingInstruction{std::make_tuple(name, oArg)}));
            memPtr += 5;
        }};

        auto const define{[&](std::string k, std::string v) {
            if (Util::std20::contains(definitions, k))
                return error(filepath, lineNumber,
                    "duplicate definition: " + k);
            definitions[k] = std::make_tuple(lineNumber, v);
            pragmas(filepath);

            return true;
        }};

        std::vector<std::tuple<
            std::string, std::function<bool(std::smatch const&)>
        >> const onMatch{

            { "^(" + regexIdentifier + ") ?:= ?(" + regexValue + ")$"
            , [&](std::smatch const&smatch) {
                return define(std::string{smatch[1]}, std::string{smatch[2]});
            }},

            { "^(" + regexIdentifier + "):$"
            , [&](std::smatch const&smatch) {
                std::string label{smatch[1]};
                if (!define("@" + label, std::to_string(memPtr)))
                    return false;
                if (label == "stack" && !stackBeginning.has_value())
                    stackBeginning = std::make_optional(memPtr);
                return true;
            }},

            { "^data ?(.+)$"
            , [&](std::smatch const&smatch) {
                log("parsing `data` ...");
                std::string commaSeparated{std::string{smatch[1]} + ","};
                for (uint_t elemNr{1}; commaSeparated != ""; ++elemNr) {
                    auto dataError = [&](
                        std::string const&msg, std::string const&detail
                    ) {
                        return error(filepath, lineNumber, msg
                            + " (element number " + std::to_string(elemNr)
                            + "): " + detail); };

                    std::smatch smatch{};
                    if (std::regex_match(commaSeparated, smatch, std::regex{
                        "^(" + ("(\\[(" + regexValue + ")\\])? ?(" + regexValue
                        + "|" + "runif " + regexValue + "|" + "rperm" + ")?")
                        + "|" + regexString + ") ?, ?(.*)$"})
                    ) {
                        std::string unparsedElement{smatch[1]};
                        std::string unparsedSize{smatch[3]};
                        std::string unparsedValue{smatch[4]};
                        commaSeparated = std::string{smatch[6]};

                        if (unparsedElement == "")
                            return dataError("invalid data element", "empty");

                        if (unparsedElement.front() == '\"') {
                            log("parsing string: " + unparsedElement);
                            std::optional<std::vector<UTF8::rune_t>> const
                                oRunes{Util::parseString(unparsedElement)};
                            if (!oRunes.has_value())
                                return dataError("invalid data string element",
                                    unparsedElement);
                            for (UTF8::rune_t rune : oRunes.value())
                                pushData(static_cast<word_t>(rune));
                            continue; }

                        log("parsing non-string: " + unparsedElement);

                        if (unparsedSize == "")
                            unparsedSize = std::string{"1"};
                        std::optional<word_t> const oSize{
                            Util::stringToOptionalUInt32(unparsedSize)};
                        if (!oSize.has_value())
                            return dataError("invalid data uint element size",
                                unparsedSize);
                        log("    ~> size: " + std::to_string(oSize.value()));

                        {
                            std::smatch smatch{};
                            if (std::regex_match(unparsedValue, smatch,
                                std::regex{"^runif (" + regexValue + ")$"})
                            ) {
                                unparsedValue = std::string{smatch[1]};
                                std::optional<word_t> const oValue{
                                    Util::stringToOptionalUInt32(
                                        unparsedValue)};
                                if (!oValue.has_value())
                                    return dataError("invalid data unif range "
                                        "value", unparsedValue);

                                for (word_t const&rnd : rng.unif(
                                    oSize.value(), oValue.value())
                                )
                                    pushData(rnd);
                                continue;
                            }
                            if (std::regex_match(unparsedValue, smatch,
                                std::regex{"^rperm$"})
                            ) {
                                for (word_t r : rng.perm(oSize.value()))
                                    pushData(r);
                                continue;
                            }
                        }

                        log("parsing uint: " + unparsedElement);

                        if (unparsedValue == "")
                            unparsedValue = std::string{"0"};
                        std::optional<word_t> const oValue{
                            Util::stringToOptionalUInt32(unparsedValue)};
                        if (!oValue.has_value())
                            return dataError("invalid data uint element value",
                                unparsedValue);
                        log("    ~> value: " + std::to_string(oValue.value()));

                        for (word_t j = 0; j < oSize.value(); ++j)
                            pushData(oValue.value());
                        continue;
                    }

                    return dataError("incomprehensible data element trunk",
                        commaSeparated);
                }
                return true;
            }},

            { "^include ?(" + regexString + ")$"
            , [&](std::smatch const&smatch) {
                std::string const includeFilepathString{smatch[1]};
                std::optional<std::vector<UTF8::rune_t>> oIncludeFilepathRunes{
                    Util::parseString(includeFilepathString)};
                if (!oIncludeFilepathRunes.has_value())
                    return error(filepath, lineNumber, "malformed utf-8 "
                        "include string: " + includeFilepathString);
                std::optional<std::string> oIncludeFilepath{
                    UTF8::utf8string(oIncludeFilepathRunes.value())};
                if (!oIncludeFilepath.has_value())
                    return error(filepath, lineNumber, "malformed utf-8 "
                        "include string: " + includeFilepathString);

                std::filesystem::path const includeFilepath{
                    filepath.parent_path() / oIncludeFilepath.value()};

                log("including with memPtr = " + std::to_string(memPtr));
                if (!parseFiles(includeFilepath, memPtr))
                    return error(filepath, lineNumber, "could not include "
                        "file: " + includeFilepath.u8string());
                log("included with memPtr = " + std::to_string(memPtr));
                return true;
            }},

            { "^include.*$"
            , [&](std::smatch const&_) {
                (void) _;
                return error(filepath, lineNumber,
                    "improper include: either empty or missing quotes");
            }},

            { "^profiler ([^ ]*?)(, ?(.*))?$"
            , [&](std::smatch const&smatch) {
                std::string const profilerStartStop{smatch[1]};
                if (profilerStartStop != "start" && profilerStartStop != "stop")
                    return error(filepath, lineNumber, "invalid profiler "
                        "directive (must be 'start' or 'stop'): "
                        + profilerStartStop);

                std::string profilerMessage{smatch[3]};
                profilerMessage = "file " + filepath.u8string() + ", ln "
                    + std::to_string(lineNumber)
                    + std::string{profilerMessage == "" ? "" : ": "}
                    + profilerMessage;

                if (profiler.size() < memPtr+1)
                    profiler.resize(memPtr+1);
                profiler[memPtr].push_back(std::make_tuple(
                    profilerStartStop == std::string{"start"},
                    profilerMessage));

                return true;
            }},

            { "^(" + regexIdentifier + ")( (" + regexValue + "))?$"
            , [&](std::smatch const&smatch) {
                auto oName = InstructionNameRepresentationHandler
                    ::from_string(smatch[1]);
                if (!oName.has_value())
                    return error(filepath, lineNumber, "invalid "
                        "instruction name: " + std::string{smatch[1]});
                std::optional<std::string> oArg{std::nullopt};
                if (smatch[3] != "")
                    oArg = std::optional{smatch[3]};
                log("pushing instruction: "
                    + InstructionNameRepresentationHandler
                        ::toString(oName.value())
                    + " " + oArg.value_or("(no arg.)"));

                InstructionName const name{oName.value()};
                InstructionDefinition const idef{instructionDefinitions[
                    InstructionNameRepresentationHandler::toByteCode(name)]};
                if (oArg.has_value() && !idef.doesTakeArgument())
                    return error(filepath, lineNumber,
                      "instruction takes no argument: "
                      + InstructionNameRepresentationHandler
                          ::toString(name));
                if (
                    !oArg.has_value() && idef.doesTakeArgument()
                    && !idef.optionalArgument.has_value()
                )
                    return error(filepath, lineNumber, "instruction requires "
                        "an argument: " + InstructionNameRepresentationHandler
                            ::toString(name));
                pushInstruction(name, oArg);

                return true;
            }},

        };

        auto parseLine{[&](std::string const&_ln) {
            std::string const ln{[](std::string ln) {
                ln = std::regex_replace(ln, std::regex{"^;.*$"}, "");
                ln = std::regex_replace(ln, std::regex{"([^\\\\]);.*$"}, "$1");
                ln = std::regex_replace(ln, std::regex{"\\s+"}, " ");
                ln = std::regex_replace(ln, std::regex{"^ +"}, "");
                ln = std::regex_replace(ln, std::regex{" +$"}, "");
                return ln;
            }(_ln)};
            if (ln == "")
                return true;

            log("ln " + std::to_string(lineNumber) + ": " + ln);

            for (auto const&[regexString, f] : onMatch) {
                std::smatch smatch{};
                if (std::regex_match(ln, smatch, std::regex{regexString}))
                    return f(smatch);
            }

            return false;
        }};

        for (; std::getline(f, ln); ++lineNumber)
            if (!parseLine(ln))
                return error(filepath, lineNumber, "incomprehensible");
        memorySize = memPtr;

        return true;
    }

    private: bool pragmas(std::filesystem::path const&filepath) {
        auto const flag{[&](
            uint_t const lineNumber, bool &flg, std::string const&tf
        ) {
            if (tf != "false" && tf != "true")
                return error(filepath, lineNumber, "invalid boolean: " + tf);
            flg = tf == "true";
            return true;
        }};

        std::vector<std::tuple<std::string,
            std::function<bool(uint_t, std::string const&)>
        >> const pragmaActions {

            {"pragma_memory-mode", [&](
                uint_t lineNumber, std::string const&mm
            ) {
                if (mm == "little-endian") {
                    pragmaMemoryMode = MemoryMode::LittleEndian;
                    return true; }
                if (mm == "big-endian") {
                    pragmaMemoryMode = MemoryMode::BigEndian;
                    return true; }
                return error(filepath, lineNumber,
                    "invalid pragma_memory-mode: " + mm);
            }},

            {"pragma_rng-seed", [&](
                uint_t lineNumber, std::string const&rs
            ) {
                std::optional<word_t> oRNGSeed{
                    Util::stringToOptionalUInt32(rs)};
                if (!oRNGSeed.has_value())
                    return error(filepath, lineNumber,
                        "invalid pragma_rng-seed: " + rs);
                pragmaRNGSeed = std::make_optional(oRNGSeed.value());
                return true;
            }},

            {"pragma_static-program", [&](
                uint_t lineNumber, std::string const&tf
            ) {
                return flag(lineNumber, pragmaStaticProgram, tf);
            }},

            {"pragma_static-stack-check", [&](
                uint_t lineNumber, std::string const&tf
            ) {
                return flag(lineNumber, pragmaStaticStackCheck, tf);
            }},

            {"pragma_embed-profiler-output", [&](
                uint_t lineNumber, std::string const&tf
            ) {
                return flag(lineNumber, embedProfilerOutput, tf);
            }},

            {"pragma_memory-size", [&](
                uint_t lineNumber, std::string const&ms
            ) {
                if (ms == "minimal")
                    ;
                else if (ms == "dynamic")
                    memoryIsDynamic = true;
                else {
                    std::optional<word_t> oM{Util::stringToOptionalUInt32(ms)};
                    if (!oM.has_value())
                        return error(filepath, lineNumber, "invalid memory "
                            "size: " + ms);
                    word_t m{oM.value()};
                    if (m < memorySize)
                        return error(filepath, lineNumber, "memory size "
                            "smaller than minimal required");
                    memorySize = m; }
                return true;
            }},

        };

        for (auto const&[pragma, action] : pragmaActions)
            if (Util::std20::contains(definitions, pragma)) {
                auto const&[lineNumber, definition]{definitions[pragma]};
                if (!action(lineNumber, definition))
                    return false; }

        if (pragmaRNGSeed.has_value())
            rng.seed(pragmaRNGSeed.value());

        if (!profiler.empty() && !pragmaStaticProgram)
            return error("incompatible pragmas: using the profiler forbids "
                "'pragma_static-program := false'");

        return true;
    }

    private: std::optional<std::vector<MemorySemantic>>
    constructMemorySemantics() const {
        if (!pragmaStaticProgram)
            return std::nullopt;

        std::vector<MemorySemantic> memorySemantics{};
        memorySemantics.reserve(memorySize);
        for (auto const&[filepath, lineNumber, p] : parsing) {
            if (std::holds_alternative<parsingData>(p)) {
                memorySemantics.push_back(MemorySemantic::DataHead);
                for (std::size_t j{0}; j < 3; j++)
                    memorySemantics.push_back(MemorySemantic::Data); }
            else if (std::holds_alternative<parsingInstruction>(p)) {
                memorySemantics.push_back(MemorySemantic::InstructionHead);
                for (std::size_t j{0}; j < 4; j++)
                    memorySemantics.push_back(MemorySemantic::Instruction); }
            else {
                error("internal error (memory semantics): parsing piece holds "
                    "invalid alternative");
                return std::nullopt; }
        }

        return std::make_optional(memorySemantics);
    }

    private: bool parseAssemble(ComputationState &cs) {
        bool memPtrGTStackBeginningAndNonDataOccurred{false};
        bool haltInstructionWasUsed{false};

        word_t memPtr{0};
        for (auto const&[filepath, lineNumber, p] : parsing) {
            if (std::holds_alternative<parsingData>(p)) {
                word_t data{std::get<parsingData>(p)};
                log("data value 0x" + Util::UInt32AsPaddedHex(data));
                try {
                    memPtr += cs.storeData(memPtr, data);
                } catch (std::runtime_error const&e) {
                    return error(filepath, lineNumber,
                        std::string{"failed to store data: "} + e.what());
                }

                if (!memPtrGTStackBeginningAndNonDataOccurred)
                    stackEnd = std::make_optional(memPtr);
            } else if (std::holds_alternative<parsingInstruction>(p)) {
                if (memPtr > stackBeginning)
                    memPtrGTStackBeginningAndNonDataOccurred = true;

                auto [name, oArg]{std::get<parsingInstruction>(p)};
                std::optional<word_t> oValue{std::nullopt};
                if (oArg.has_value()) {
                    if (Util::std20::contains(definitions, oArg.value())) {
                        auto const&[_, definition]{definitions[oArg.value()]};
                        oArg = std::make_optional(definition); }

                    if (oArg.value() == "")
                        return error(filepath, lineNumber,
                            "no instruction argument");
                    if (oArg.value().front() == '@') {
                        std::string label{oArg.value()};
                        label.erase(label.begin());

                        std::vector<std::string> unsortedLabels{};
                        for (auto const&[lbl, _] : definitions)
                            unsortedLabels.push_back(lbl);
                        std::vector<std::string> const sortedLabels{
                            Util::sortByLevenshteinDistanceTo(
                                unsortedLabels, label)};
                        std::string msg{"label @" + label + " was not defined; "
                            "did you possibly mean one of the following "
                            "defined labels?"};
                        for (
                            std::size_t j{0};
                            j < sortedLabels.size() && j < 3;
                            ++j
                        )
                            msg += "\n    " + std::to_string(j+1) + ") "
                                + sortedLabels[j];
                        if (sortedLabels.empty())
                            msg += "\n    (no labels have been defined)";
                        return error(filepath, lineNumber, msg);
                    }

                    if (oArg.value().front() == '\'') {
                        if (
                            oArg.value().size() < 2
                            || oArg.value().back() != '\''
                        )
                            return error(filepath, lineNumber,
                                "invalid character literal");
                        std::string s{oArg.value()};
                        s.front() = s.back() = '"';
                        std::optional<std::vector<UTF8::rune_t>> oRunes{
                            Util::parseString(s)};
                        if (oRunes.has_value() && oRunes.value().size() != 1)
                            oRunes = std::nullopt;
                        if (!oRunes.has_value())
                            return error(filepath, lineNumber,
                                "invalid character literal");
                        oValue = std::make_optional(static_cast<word_t>(
                            oRunes.value()[0]));
                    } else {
                        oValue = static_cast<std::optional<word_t>>(
                            Util::stringToOptionalUInt32(oArg.value()));
                        if (!oValue.has_value())
                            return error(filepath, lineNumber,
                                "invalid argument value: " + oArg.value());
                    }
                }

                InstructionDefinition const idef{instructionDefinitions[
                    InstructionNameRepresentationHandler::toByteCode(name)]};
                if (!idef.doesTakeArgument() && oValue.has_value())
                    return error(filepath, lineNumber, "superfluous argument: "
                        + InstructionNameRepresentationHandler::toString(name)
                        + " " + std::to_string(oValue.value()));
                if (idef.doesTakeArgument()) {
                    if (
                        !idef.optionalArgument.has_value()
                        && !oValue.has_value()
                    )
                        return error(filepath, lineNumber, "requiring "
                            "argument: " + InstructionNameRepresentationHandler
                                ::toString(name));
                    if (!oValue.has_value())
                        oValue = idef.optionalArgument; }

                word_t const argument{oValue.value_or(0x00000000)};
                Instruction instruction{name, argument};
                log("instruction " + InstructionRepresentationHandler
                    ::toString(instruction));

                try {
                    memPtr += cs.storeInstruction(memPtr, instruction);
                } catch (std::runtime_error const&e) {
                    return error(filepath, lineNumber,
                        std::string{"failed to store instruction: "}
                        + e.what());
                }

                haltInstructionWasUsed |= InstructionName::HLT == name;
                stackInstructionWasUsed |= InstructionNameRepresentationHandler
                    ::doesPointAtStack(name);
            } else
                return error("internal error: parsing piece holds invalid "
                    "alternative");
        }

        if (!haltInstructionWasUsed)
            return error("no halt instruction was used");

        if (pragmaStaticStackCheck && stackInstructionWasUsed)
            if (!Util::std20::contains(definitions, std::string{"@stack"}))
                return error("stack instructions are used yet no stack was "
                    "defined");

        if (stackBeginning.has_value() != stackEnd.has_value())
            return error("inconsistent stack boundaries");

        if (stackBeginning.has_value() && stackEnd.has_value()) {
            log("got as stack boundaries: "
                + std::to_string(stackBeginning.value()) + " and "
                + std::to_string(stackEnd.value()));

            cs.debug.stackBoundaries = std::make_tuple(
                stackBeginning.value(), stackEnd.value());
            cs.registerSC = stackBeginning.value();
        } else
            log("no stack was defined");

        return true;
    }
};

#endif
