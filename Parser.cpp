#ifndef JOY_ASSEMBLER__PARSE_CPP
#define JOY_ASSEMBLER__PARSE_CPP

#include <random>

#include "Types.hh"

class Parser {
    private:
        std::vector<std::filesystem::path> filepaths;
        bool ok;

        std::random_device randomDevice;
        std::optional<uint32_t> oRandomSeed;

        ComputationState cs;

    public: Parser(std::filesystem::path filepath) :
        filepaths{},
        ok{true},

        randomDevice{},
        oRandomSeed{std::nullopt},

        cs{0x10000}
    {
        filepaths.push_back(filepath);
    }

    public: bool commandlineArg(std::string const&arg) {
        if (arg == "visualize")
            cs.enableVisualization();
        else if (arg == "step")
            cs.enableStepping();
        else if (arg == "cycles")
            cs.enableFinalCycles();
        else
            return err("unkown commandline argument: " + arg);
        return true; }

    public: bool parse() {
        if (filepaths.size() <= 0)
            return err("no filepath to parse");
        ParsingState ps{filepaths.back()};
        if (!parse1(ps))
            return err("parsing failed at stage one");
        if (!parse2(ps, cs))
            return err("parsing failed at stage two");
        return true; }

    public: std::tuple<ComputationState, bool> finish() {
        return std::make_tuple(cs, ok); }

    private: bool err(std::string const&msg) {
        std::cerr << "Parser: " << msg << std::endl;
        return ok = false; }


    private: bool parse1(ParsingState &ps) {
        if (!is_regular_file(ps.filepath))
            return ps.error(0, "not a regular file");
        if (Util::std20::contains(ps.parsedFilepaths, ps.filepath))
            return ps.error(0, "recursive file inclusion; not parsing file twice");
        ps.parsedFilepaths.insert(ps.filepath);

        std::ifstream is{ps.filepath};
        if (!is.is_open())
            return ps.error(0, "unable to read file");

        std::string const&regexIdentifier{"[.$_[:alpha:]-][.$_[:alnum:]-]*"};
        std::string const&regexValue{"[@.$'_[:alnum:]+-][.$'_[:alnum:]\\\\-]*"};
        std::string const&regexString{"\"([^\"]|\\\")*?\""};

        for (
            auto [lineNumber, ln]
                = std::make_tuple<ParsingState::line_number_t, std::string>(1, "");
            std::getline(is, ln);
            ++lineNumber
        ) {

            auto pushData = [&](uint32_t const data) {
                log("pushing data: " + Util::UInt32AsPaddedHex(data));
                ps.parsing.push_back(std::make_tuple(ps.filepath, lineNumber,
                    ParsingState::parsingData{data}));
                ps.memPtr += 4;
            };

            auto pushInstruction = [&](InstructionName const&name, std::optional<std::string> const&oArg) {
                log("pushing instruction: " + InstructionNameRepresentationHandler::to_string(name) + oArg.value_or(" (no arg.)"));
                ps.parsing.push_back(std::make_tuple(ps.filepath, lineNumber,
                    ParsingState::parsingInstruction{std::make_tuple(name, oArg)}));
                ps.memPtr += 5;
            };

            ln = std::regex_replace(ln, std::regex{";.*$"}, "");
            ln = std::regex_replace(ln, std::regex{"\\s+"}, " ");
            ln = std::regex_replace(ln, std::regex{"^ +"}, "");
            ln = std::regex_replace(ln, std::regex{" +$"}, "");

            if (ln == "")
                continue;

            log("ln " + std::to_string(lineNumber) + ": " + ln);

            auto define = [&ps, lineNumber]
                (std::string k, std::string v)
            {
                log("defining " + k + " to be " + v);
                if (Util::std20::contains(ps.definitions, k))
                    return ps.error(lineNumber, "duplicate definition: " + k);
                ps.definitions[k] = v;
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
                    return define(std::string{smatch[1]}, std::string{smatch[2]}); }))

            ON_MATCH("^(" + regexIdentifier + "):$", (
                [define, &ps](std::smatch const&smatch) {
                    std::string label{smatch[1]};
                    if (!define("@" + label, std::to_string(ps.memPtr)))
                        return false;
                    if (label == "stack" && !ps.stackBeginning.has_value())
                        ps.stackBeginning = std::make_optional(ps.memPtr);
                    return true; }))

            ON_MATCH("^data ?(.+)$", (
                [this, pushData, lineNumber, regexValue, regexString, &ps](std::smatch const&smatch) {
                    log("parsing `data` ...");
                    std::string commaSeparated{std::string{smatch[1]} + ","};
                    for (uint64_t elementNumber{1}; commaSeparated != ""; ++elementNumber) {
                        auto error = [lineNumber, elementNumber, &ps](std::string const&msg, std::string const&detail) {
                            return ps.error(lineNumber, msg + " (element number " + std::to_string(elementNumber) + "): " + detail); };

                        std::smatch smatch{};
                        if (std::regex_match(commaSeparated, smatch, std::regex{"^(" + ("(\\[(" + regexValue + ")\\])? ?(" + regexValue + "|" + "unif " + regexValue + ")?") + "|" + regexString + ") ?, ?(.*)$"})) {

                            log("smatch of size " + std::to_string(smatch.size()));
                            for (auto j = 0u; j < smatch.size(); ++j)
                                log("smatch[" + std::to_string(j) + "]: @@@" + std::string{smatch[j]} + "@@@");

                            std::string unparsedElement{smatch[1]};
                            std::string unparsedSize{smatch[3]};
                            std::string unparsedValue{smatch[4]};
                            commaSeparated = std::string{smatch[6]};

                            if (unparsedElement == "")
                                return error("invalid data element", "empty element");

                            if (unparsedElement.front() == '\"') {
                                log("parsing string: " + unparsedElement);
                                std::optional<std::vector<UTF8::rune_t>> oRunes = Util::parseString(unparsedElement);
                                if (!oRunes.has_value())
                                    return error("invalid data string element", unparsedElement);
                                for (UTF8::rune_t rune : oRunes.value())
                                    pushData(static_cast<word_t>(rune));
                                continue;
                            }

                            log("parsing non-string: " + unparsedElement);

                            if (unparsedSize == "")
                                unparsedSize = std::string{"1"};
                            std::optional<word_t> oSize{Util::stringToUInt32(unparsedSize)};
                            if (!oSize.has_value())
                                return error("invalid data uint element size", unparsedSize);
                            log("    ~> size: " + std::to_string(oSize.value()));

                            /* TODO */
                            {
                                std::smatch smatch{};
                                if (std::regex_match(unparsedValue, smatch, std::regex{"^unif (" + regexValue + ")$"})) {
                                    unparsedValue = std::string{smatch[1]};
                                    std::optional<word_t> oValue{Util::stringToUInt32(unparsedValue)};
                                    if (!oValue.has_value())
                                        return error("invalid data unif range value", unparsedValue);

                                    if (!oRandomSeed.has_value())
                                        oRandomSeed = std::make_optional(randomDevice());
                                    std::mt19937 rng{oRandomSeed.value()};
                                    std::uniform_int_distribution<uint32_t> unif{0, oValue.value()};
                                    for (word_t j = 0; j < oSize.value(); ++j)
                                        pushData(unif(rng));
                                    continue;
                                }
                            }

                            log("parsing uint: " + unparsedElement);

                            if (unparsedValue == "")
                                unparsedValue = std::string{"0"};
                            std::optional<word_t> oValue{Util::stringToUInt32(unparsedValue)};
                            if (!oValue.has_value())
                                return error("invalid data uint element value", unparsedValue);
                            log("    ~> value: " + std::to_string(oValue.value()));

                            for (word_t j = 0; j < oSize.value(); ++j)
                                pushData(oValue.value());
                            continue;
                        }
                        return error("incomprehensible data element trunk", commaSeparated);
                    }
                    return true; }))

            ON_MATCH("^include ?(" + regexString + ")$", (
                [this, lineNumber, &ps](std::smatch const&smatch) {
                    std::optional<std::vector<UTF8::rune_t>> oIncludeFilepathRunes
                        = Util::parseString(std::string{smatch[1]});
                    std::optional<std::string> oIncludeFilepath{std::nullopt};
                    if (oIncludeFilepathRunes.has_value())
                        oIncludeFilepath = UTF8::runeVectorToOptionalString(
                            oIncludeFilepathRunes.value());
                    if (oIncludeFilepath.has_value())
                        oIncludeFilepath = Util::
                            startingFilepathFilepathToOptionalResolvedFilepath(
                                ps.filepath.parent_path(),
                                oIncludeFilepath.value());
                    if (!oIncludeFilepath.has_value())
                        return ps.error(lineNumber, "malformed include string");

                    std::filesystem::path includeFilepath{oIncludeFilepath.value()};
                    bool success{};

                    log("including with memPtr = " + std::to_string(ps.memPtr));
                    /* TODO remove */ LOCALLY_CHANGE(ps.filepath, includeFilepath, (
                        success = parse1(ps)))
                    log("included with memPtr = " + std::to_string(ps.memPtr));

                    if (!success)
                        return ps.error(lineNumber, "could not include file: "
                            + std::string{includeFilepath});
                    return true; }))

            ON_MATCH("^include.*$", (
                [lineNumber, &ps](std::smatch const&_) {
                    (void) _;
                    return ps.error(lineNumber,
                        "improper include: either empty or missing quotes"); }))

            ON_MATCH("^(" + regexIdentifier + ")( (" + regexValue + "))?$", (
                [pushInstruction, lineNumber, &ps](std::smatch const&smatch) {
                    auto oName = InstructionNameRepresentationHandler
                        ::from_string(smatch[1]);
                    if (!oName.has_value())
                        return ps.error(lineNumber, "invalid instruction name: "
                            + std::string{smatch[1]});
                    std::optional<std::string> oArg{std::nullopt};
                    if (smatch[3] != "")
                        oArg = std::optional{smatch[3]};
                    pushInstruction(oName.value(), oArg);
                    return true; }))

            #undef ON_MATCH

            return ps.error(lineNumber, "incomprehensible");
        }

        return true;
    }

    private: bool parse2(ParsingState &ps, ComputationState &cs) {
        log("@@@ parse2 @@@");

        bool memPtrGTStackBeginningAndNonDataOccurred{false};
        bool haltInstructionWasUsed{false};

        word_t memPtr{0};
        for (auto const&[filepath, lineNumber, p] : ps.parsing) {
            if (std::holds_alternative<ParsingState::parsingData>(p)) {
                word_t data{std::get<ParsingState::parsingData>(p)};
                log("data value " + Util::UInt32AsPaddedHex(data));
                memPtr += cs.storeData(memPtr, data);

                if (!memPtrGTStackBeginningAndNonDataOccurred)
                    ps.stackEnd = std::make_optional(memPtr);
            }
            else if (std::holds_alternative<ParsingState::parsingInstruction>(p)) {
                if (memPtr > ps.stackBeginning)
                    memPtrGTStackBeginningAndNonDataOccurred = true;

                ParsingState::parsingInstruction instruction{
                    std::get<ParsingState::parsingInstruction>(p)};
                InstructionName name = std::get<0>(instruction);
                auto oArg = std::get<1>(instruction);
                std::optional<word_t> oValue{std::nullopt};
                if (oArg.has_value()) {
                    if (Util::std20::contains(ps.definitions, oArg.value()))
                        oArg = std::make_optional(ps.definitions.at(oArg.value()));

                    if (oArg.value() == "")
                        return ps.error(lineNumber, "no instruction argument");
                    if (oArg.value().front() == '@') {
                        std::string label{oArg.value()};
                        label.erase(label.begin());

                        std::vector<std::string> labels{};
                        for (auto const&[lbl, _] : ps.definitions)
                            labels.push_back(lbl);
                        labels = Util::sortByLevenshteinDistanceTo(labels, label);
                        std::string msg{"label @" + label + " was not defined; "
                            + "did you possibly mean one of the following defined "
                            + "labels?"};
                        for (std::size_t j = 0; j < 3 && j < labels.size(); ++j)
                            msg += "\n    " + std::to_string(j+1) + ") "
                                + labels[j];
                        if (labels.size() <= 0)
                            msg += "\n    (no labels have been defined)";
                        return ps.error(lineNumber, msg);
                    }


                    if (oArg.value().front() == '\'') {
                        if (oArg.value().size() < 2 || oArg.value().back() != '\'')
                            return ps.error(lineNumber, "invalid character literal");
                        std::string s{oArg.value()};
                        s.front() = s.back() = '"';
                        std::optional<std::vector<UTF8::rune_t>> oRunes{Util::parseString(s)};
                        if (oRunes.has_value() && oRunes.value().size() != 1)
                            oRunes = std::nullopt;
                        if (!oRunes.has_value())
                            return ps.error(lineNumber, "invalid character literal");
                        oValue = static_cast<std::optional<word_t>>(std::make_optional(oRunes.value().at(0)));
                    } else {
                        oValue = static_cast<std::optional<word_t>>(
                            Util::stringToUInt32(oArg.value()));
                        if (!oValue.has_value())
                            return ps.error(lineNumber, "invalid argument value: "
                                + oArg.value());
                    }
                }

                auto [hasArgument, optionalValue] =
                    InstructionNameRepresentationHandler::argumentType[name];
                if (!hasArgument && oValue.has_value())
                    return ps.error(lineNumber, "superfluous argument: "
                        + InstructionNameRepresentationHandler::to_string(name)
                        + " " + std::to_string(oValue.value()));
                if (hasArgument) {
                    if (!optionalValue.has_value() && !oValue.has_value())
                        return ps.error(lineNumber, "requiring argument: "
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
                ps.stackInstructionWasUsed |=
                    InstructionNameRepresentationHandler::isStackInstruction(name);
            }
        }

        if (!haltInstructionWasUsed)
            return ps.error(0, "no halt instruction was used");

        if (ps.stackInstructionWasUsed)
            if (!Util::std20::contains(ps.definitions, std::string{"@stack"}))
                return ps.error(0, std::string{"stack instructions are used "}
                    + "yet no stack was defined");

        if (ps.stackBeginning.has_value() != ps.stackEnd.has_value())
            return ps.error(0, "inconsistent stack boundaries");

        if (ps.stackBeginning.has_value() && ps.stackEnd.has_value()) {
            log("got as stack boundaries: "
                + std::to_string(ps.stackBeginning.value()) + " and "
                + std::to_string(ps.stackEnd.value()));

            cs.initializeStack(ps.stackBeginning.value(), ps.stackEnd.value());
        } else
            log("no stack was defined");

        return true;
    }
};


#endif
