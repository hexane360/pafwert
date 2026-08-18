// SPDX-License-Identifier: Apache-2.0
// Pafwert — modern C++ port. Original Copyright 2001-2013 Mark Burnett (mb@xato.net)
#pragma once

#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace pafwert {

class Rng;
class WordRepo;

class PatternError : public std::runtime_error {
    using std::runtime_error::runtime_error;
};

// Pattern language quick reference (see Docs/Pattern Guide for the original):
//   {word(noun)}          random word from Wordlists/Noun.txt
//   {a|b|c}               pick one alternative (empty alternatives allowed)
//   {number(99,10)}       random number; also letter, vowel, symbol, smiley,
//                         sequence, numberpattern, pronounceable, ...
//   {...+obscure}         apply a modifier (obscure, propercase, piglatin, ...)
//   {...[75]}             keep the item only 75% of the time
//   {$W2}                 repeat the completed value of placeholder #2
//   \{ \} \[ \] \( \) \| \+ \\   literal special characters
class Engine {
public:
    Engine(WordRepo& repo, Rng& rng);

    struct Result {
        std::string password;
        std::string pattern;  // the pattern that produced it
    };

    // Generate one password. With an empty pattern a random pattern from
    // patterns.cfg is used each attempt.
    Result generate(std::string_view pattern = {}, std::string_view keywords = {});

    // Returns an empty string when the pattern is valid, otherwise an error
    // message (port of PafwertLib.CheckPattern).
    std::string checkPattern(std::string_view pattern);

private:
    struct Node {
        // Concatenation of literal text and completed child values, children
        // referenced by their index in nodes_.
        struct Segment {
            std::string literal;
            std::size_t child = SIZE_MAX;
            bool isChild() const { return child != SIZE_MAX; }
        };
        std::vector<Segment> segments;
        std::string completed;
    };

    // Nodes ordered by closing brace (post-order); the root spans the whole
    // pattern and comes last. Placeholder N of the pattern language is
    // nodes[N-1].
    std::vector<Node> parse(std::string_view escapedPattern) const;
    std::string evaluate(std::vector<Node>& nodes, std::string_view keywords);
    std::string getWord(std::string placeholder, std::string_view keywords);
    std::string applyModifiers(std::string word, const std::vector<std::string>& modifiers);

    WordRepo& repo_;
    Rng& rng_;
    long entropy1_;
    long entropy2_;
    long entropy3_;
};

}  // namespace pafwert
