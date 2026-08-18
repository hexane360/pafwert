// SPDX-License-Identifier: Apache-2.0
#include "engine.hpp"

#include <cctype>
#include <chrono>
#include <ctime>

#include "random.hpp"
#include "text.hpp"
#include "words.hpp"

namespace pafwert {

// While a pattern is being assembled, special characters that should end up in
// the password literally are carried as tokens so they can't be re-parsed as
// pattern syntax. The tokens are resolved at the very end.
static constexpr std::pair<char, std::string_view> kEscapes[] = {
    {'\\', "#sla#"}, {'+', "#pls#"}, {'{', "#lbr#"}, {'}', "#rbr#"},
    {'[', "#lba#"},  {']', "#rba#"}, {'(', "#lpa#"}, {')', "#rpa#"},
    {'|', "#pip#"},
};

// Backslash escapes in the pattern source -> tokens.
static std::string escapePatternSource(std::string s) {
    for (auto [ch, token] : kEscapes)
        s = replaceAll(std::move(s), std::string("\\") + ch, token);
    return s;
}

// Raw special characters that appear in generated words -> tokens.
static std::string escapeSpecials(const std::string& s) {
    std::string out;
    out.reserve(s.size());
    for (char c : s) {
        bool special = false;
        for (auto [ch, token] : kEscapes) {
            if (c == ch) {
                out += token;
                special = true;
                break;
            }
        }
        if (!special) out += c;
    }
    return out;
}

static std::string unescapeTokens(std::string s) {
    for (auto [ch, token] : kEscapes)
        s = replaceAll(std::move(s), token, std::string(1, ch));
    return s;
}

Engine::Engine(WordRepo& repo, Rng& rng) : repo_(repo), rng_(rng) {
    // The original seeded these from wall-clock/uptime quirks; the shapes are
    // kept (e1 clustered around 50, e2 mid-range, e3 spread evenly).
    entropy1_ = rng_.range(60, 40);
    entropy2_ = rng_.range(70, 30);
    entropy3_ = rng_.range(100, 1);
}

// --------------------------------------------------------------------------
// Parsing
// --------------------------------------------------------------------------

std::vector<Engine::Node> Engine::parse(std::string_view pattern) const {
    std::vector<Node> nodes;

    // Recursive descent over the escaped pattern text. Nodes are appended when
    // their closing brace is reached, which reproduces the original
    // closing-brace numbering that {$Wn} back-references rely on.
    auto parseNode = [&](auto&& self, std::size_t& pos, bool isRoot) -> std::size_t {
        Node node;
        std::string literal;
        auto flush = [&] {
            if (!literal.empty()) {
                node.segments.push_back({.literal = std::move(literal)});
                literal.clear();
            }
        };
        while (pos < pattern.size()) {
            char c = pattern[pos];
            if (c == '{') {
                ++pos;
                std::size_t child = self(self, pos, false);
                flush();
                node.segments.push_back({.child = child});
            } else if (c == '}') {
                if (isRoot) throw PatternError("mismatched braces in pattern");
                ++pos;
                flush();
                nodes.push_back(std::move(node));
                return nodes.size() - 1;
            } else {
                literal += c;
                ++pos;
            }
        }
        if (!isRoot) throw PatternError("mismatched braces in pattern");
        flush();
        nodes.push_back(std::move(node));
        return nodes.size() - 1;
    };

    std::size_t pos = 0;
    parseNode(parseNode, pos, true);
    return nodes;
}

// --------------------------------------------------------------------------
// Evaluation
// --------------------------------------------------------------------------

std::string Engine::evaluate(std::vector<Node>& nodes, std::string_view keywords) {
    for (std::size_t i = 0; i < nodes.size(); ++i) {
        Node& node = nodes[i];
        std::string text;
        for (const Node::Segment& seg : node.segments)
            text += seg.isChild() ? nodes[seg.child].completed : seg.literal;

        // {$Wn} repeats the completed value of placeholder n (a single digit,
        // as in the original).
        if (text.size() >= 3 && text[0] == '$' && text[1] == 'W' &&
            std::isdigit(static_cast<unsigned char>(text[2]))) {
            std::size_t ref = static_cast<std::size_t>(text[2] - '0');
            node.completed = (ref >= 1 && ref <= i) ? nodes[ref - 1].completed : "";
        } else {
            node.completed = getWord(std::move(text), keywords);
        }
    }
    return nodes.back().completed;
}

std::string Engine::getWord(std::string placeholder, std::string_view keywords) {
    using namespace charsets;

    // Modifiers hang off the last '+' (everything earlier is placeholder text,
    // exactly as in the original parser).
    std::vector<std::string> modifiers;
    if (std::size_t pos = placeholder.rfind('+'); pos != std::string::npos) {
        std::string mod = placeholder.substr(pos + 1);
        if (!mod.empty()) modifiers.push_back(std::move(mod));
        placeholder.erase(pos);
    }

    // [NN] qualifier: keep the placeholder only NN% of the time.
    if (std::size_t pos = placeholder.find('['); pos != std::string::npos) {
        long q = toLong(placeholder.substr(pos + 1));
        if (q < rng_.range(99, 0)) return "";
        placeholder.erase(pos);
    }

    // (a,b,c) parameters.
    std::vector<std::string> params(4);
    if (std::size_t pos = placeholder.find('('); pos != std::string::npos) {
        std::size_t close = placeholder.find(')', pos);
        std::string inner = close == std::string::npos
                                ? placeholder.substr(pos + 1)
                                : placeholder.substr(pos + 1, close - pos - 1);
        inner = replaceAll(std::move(inner), ", ", ",");
        params = split(inner, ',');
        if (params.size() < 4) params.resize(4);
        placeholder.erase(pos);
    }

    // a|b|c selection group: pick one and return it as-is (modifiers on the
    // same level are intentionally skipped, matching the original).
    if (placeholder.find('|') != std::string::npos)
        return rng_.pickOne(placeholder, 1, '|');

    std::string name = toLower(trim(placeholder));
    std::string word;

    if (name == "word") {
        if (!keywords.empty() && rng_.chance(entropy1_)) {
            word = rng_.pickOne(keywords);
        } else {
            // Allow alternation inside the parameter: word(verb|noun)
            std::string file = params[0];
            if (file.find('|') != std::string::npos) file = rng_.pickOne(file, 1, '|');
            word = repo_.randomWord(file, rng_);
        }
    } else if (name == "sp" || name == "space") {
        word = " ";
    } else if (name == "vowel") {
        word = rng_.pickChar(VOWELS, toLong(params[0]));
    } else if (name == "consonant") {
        word = rng_.pickChar(CONSONANTS, toLong(params[0]));
    } else if (name == "symbol") {
        word = rng_.pickOne(SYMBOLS);
    } else if (name == "endpunctuation") {
        word = rng_.pickOne(ENDPUNCTUATION);
    } else if (name == "sentencepunctuation") {
        word = rng_.pickChar(SENTENCEPUNCTUATION);
    } else if (name == "number") {
        word = rng_.rangeText(toLong(params[0]), toLong(params[1]), toLong(params[2]),
                              toLong(params[3]));
    } else if (name == "letter") {
        word = rng_.pickChar(LETTERS, toLong(params[0]));
    } else if (name == "smiley") {
        word = rng_.pickOne(SMILEYS);
    } else if (name == "keyboard") {
        word = rng_.pickChar(KEYBOARD);
    } else if (name == "numrow") {
        word = rng_.pickChar(NUMROW);
    } else if (name == "numrowfull") {
        word = rng_.pickChar(NUMROWFULL);
    } else if (name == "row1") {
        word = rng_.pickChar(ROW1);
    } else if (name == "row1full") {
        word = rng_.pickChar(ROW1FULL);
    } else if (name == "row2") {
        word = rng_.pickChar(ROW2);
    } else if (name == "row2full") {
        word = rng_.pickChar(ROW2FULL);
    } else if (name == "row3") {
        word = rng_.pickChar(ROW3);
    } else if (name == "row3full") {
        word = rng_.pickChar(ROW3FULL);
    } else if (name == "lefthand") {
        word = rng_.pickChar(LEFTHAND);
    } else if (name == "righthand") {
        word = rng_.pickChar(RIGHTHAND);
    } else if (name == "sequence") {
        word = sequence(rng_, toLong(params[0]));
    } else if (name == "ordinal") {
        word = ordinal(toLong(params[0]));
    } else if (name == "phonetic") {
        word = phonetic(params[0], toLong(params[1]));
    } else if (name == "pronounceable") {
        word = pronounceableWord(rng_);
    } else if (name == "numberpattern") {
        long len = toLong(params[0]);
        word = numberPattern(rng_, len == 0 ? 3 : len);
    } else if (name == "entropy1") {
        word = std::to_string(entropy1_);
    } else if (name == "entropy2") {
        word = std::to_string(entropy2_);
    } else if (name == "entropy3") {
        word = std::to_string(entropy3_);
    } else if (name == "asc") {
        word = params[0].empty()
                   ? ""
                   : std::to_string(static_cast<unsigned char>(params[0][0]));
    } else if (name == "chr") {
        long code = toLong(params[0]);
        if (code > 0 && code < 256) word = std::string(1, static_cast<char>(code));
    } else if (name == "now") {
        std::time_t now = std::time(nullptr);
        char buf[32];
        std::tm tmv{};
#ifdef _WIN32
        localtime_s(&tmv, &now);
#else
        localtime_r(&now, &tmv);
#endif
        std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &tmv);
        word = buf;
    } else if (name == "longmonth") {
        word = rng_.pickOne(LONGMONTHS);
    } else if (name == "shortmonth") {
        word = rng_.pickOne(SHORTMONTHS);
    } else if (name == "longday") {
        word = rng_.pickOne(LONGDAYS);
    } else if (name == "shortday") {
        word = rng_.pickOne(SHORTDAYS);
    } else if (name == "numbercode") {
        word = numberCode(rng_);
    } else {
        // Unknown placeholder: the text passes through unchanged. This is also
        // the path every fully-assembled parent pattern takes.
        word = placeholder;
    }

    if (!modifiers.empty()) word = applyModifiers(std::move(word), modifiers);
    return escapeSpecials(word);
}

std::string Engine::applyModifiers(std::string word,
                                   const std::vector<std::string>& modifiers) {
    if (word.empty()) return word;

    for (std::string modifier : modifiers) {
        // [NN] qualifier: NN% chance to apply; a failed roll stops modifier
        // processing entirely (as in the original).
        if (std::size_t pos = modifier.find('['); pos != std::string::npos) {
            long q = toLong(modifier.substr(pos + 1));
            if (q < rng_.range(99, 0)) return word;
            modifier.erase(pos);
        }

        std::vector<std::string> params(4);
        if (std::size_t pos = modifier.find('('); pos != std::string::npos) {
            std::size_t close = modifier.find(')', pos);
            std::string inner = close == std::string::npos
                                    ? modifier.substr(pos + 1)
                                    : modifier.substr(pos + 1, close - pos - 1);
            params = split(inner, ',');
            for (std::string& p : params) {
                p = trim(p);
                if (p.size() >= 2 && p.front() == '"' && p.back() == '"')
                    p = p.substr(1, p.size() - 2);
            }
            if (params.size() < 4) params.resize(4);
            modifier.erase(pos);
        }

        std::string name = toLower(trim(modifier));
        if (name == "random")
            name = rng_.pickOne(
                "bracket num2words randomcase reverse obscure piglatin scramble swap");

        if (name == "a") {
            word = (std::string_view("aeiou").find(static_cast<char>(std::tolower(
                        static_cast<unsigned char>(word[0])))) != std::string_view::npos
                        ? "an "
                        : "a ") +
                   word;
        } else if (name == "bracket") {
            word = bracket(rng_, word, params[0]);
        } else if (name == "num2word" || name == "num2words") {
            word = trim(sentenceCase(numberAsText(word)));
        } else if (name == "reverse") {
            word.assign(word.rbegin(), word.rend());
        } else if (name == "ucase" || name == "uppercase") {
            word = toUpper(word);
        } else if (name == "lcase" || name == "lowercase") {
            word = toLower(word);
        } else if (name == "propercase") {
            word = properCase(std::move(word));
        } else if (name == "sentencecase") {
            word = sentenceCase(std::move(word));
        } else if (name == "obscure") {
            word = obscure(rng_, std::move(word));
        } else if (name == "replace") {
            word = replaceAll(std::move(word), params[0], params[1]);
        } else if (name == "randomcase") {
            word = randomCase(rng_, std::move(word));
        } else if (name == "scramble") {
            word = scrambleWord(rng_, std::move(word), toLong(params[0]));
        } else if (name == "piglatin") {
            word = pigLatin(rng_, word);
        } else if (name == "repeat") {
            long times = toLong(params[0]);
            if (times <= 0) times = 1;
            std::string repeated = word;
            for (long i = 0; i < times; ++i) repeated += word;
            word = std::move(repeated);
        } else if (name == "right") {
            long n = toLong(params[0]);
            if (n > 0 && n < static_cast<long>(word.size()))
                word = word.substr(word.size() - static_cast<std::size_t>(n));
        } else if (name == "left") {
            long n = toLong(params[0]);
            if (n > 0 && n < static_cast<long>(word.size()))
                word = word.substr(0, static_cast<std::size_t>(n));
        } else if (name == "trim") {
            word = trim(word);
        } else if (name == "format") {
            word = formatNumber(word, params[0]);
        } else if (name == "mid") {
            long start = toLong(params[0]);
            long len = toLong(params[1]);
            if (start <= 0) start = 1;
            if (len <= 0) len = 1;
            if (start <= static_cast<long>(word.size()))
                word = word.substr(static_cast<std::size_t>(start - 1),
                                   static_cast<std::size_t>(len));
        } else if (name == "swap") {
            std::vector<std::string> parts = split(word, ' ');
            if (parts.size() > 1 && !parts[0].empty() && !parts[1].empty())
                std::swap(parts[0][0], parts[1][0]);
            word.clear();
            for (std::size_t i = 0; i < parts.size(); ++i) {
                if (i) word += ' ';
                word += parts[i];
            }
        } else if (name == "romannumeral") {
            word = toRoman(toLong(word));
        } else if (name == "hide") {
            word.clear();
        } else if (name == "quote") {
            word = "\"" + word + "\"";
        } else if (name == "stutter") {
            word = stutter(rng_, word);
        }
        // Unknown modifiers are ignored (the original raised an error and
        // discarded the whole password; several shipped patterns trip that).

        if (word.empty()) return word;
    }
    return word;
}

// --------------------------------------------------------------------------
// Public API
// --------------------------------------------------------------------------

Engine::Result Engine::generate(std::string_view pattern, std::string_view keywords) {
    constexpr int kMaxAttempts = 10;
    constexpr std::size_t kMinLen = 4;

    std::string password;
    std::string usedPattern;
    for (int attempt = 0; attempt < kMaxAttempts; ++attempt) {
        usedPattern = pattern.empty()
                          ? repo_.patterns()[rng_.index(repo_.patterns().size())]
                          : std::string(pattern);
        try {
            std::vector<Node> nodes = parse(escapePatternSource(trim(usedPattern)));
            password = evaluate(nodes, keywords);
        } catch (const std::runtime_error&) {
            if (!pattern.empty()) throw;  // a fixed pattern won't get better
            continue;                     // pick a different random pattern
        }
        password = trim(password);
        password = unescapeTokens(std::move(password));
        password = replaceAll(std::move(password), "\r", "");
        password = replaceAll(std::move(password), "\n", "");
        while (password.find("  ") != std::string::npos)
            password = replaceAll(std::move(password), "  ", " ");
        if (password.size() >= kMinLen) break;
        if (!pattern.empty()) break;
    }

    // Failsafe if something went wrong somewhere
    if (password.size() < kMinLen) {
        std::string filler = std::string(charsets::VOWELS2) +
                             " ! @ # % $ ^ & * : ' / ` ~ * - < > + = . . , , ; ; ? ? " +
                             std::string(charsets::CONSONANTS2) + " " +
                             std::string(charsets::THREELETTERWORDS) +
                             " 1 2 3 4 5 6 7 8 9 0";
        for (int i = 0; i <= 6; ++i) password += rng_.pickOne(filler);
    }

    return {.password = std::move(password), .pattern = std::move(usedPattern)};
}

std::string Engine::checkPattern(std::string_view pattern) {
    std::string p = trim(pattern);
    if (p.empty()) return "Error: Empty pattern";

    std::string escaped = escapePatternSource(p);
    long braces = 0, brackets = 0, parens = 0;
    for (char c : escaped) {
        switch (c) {
        case '{': ++braces; break;
        case '}': --braces; break;
        case '[': ++brackets; break;
        case ']': --brackets; break;
        case '(': ++parens; break;
        case ')': --parens; break;
        }
    }
    if (braces != 0) return "Error: Unmatched braces in pattern";
    if (brackets != 0) return "Error: Unmatched brackets in pattern";
    if (parens != 0) return "Error: Unmatched parenthesis in pattern";

    try {
        generate(pattern);
    } catch (const std::exception& e) {
        return std::string("Error: ") + e.what();
    }
    return "";
}

}  // namespace pafwert
