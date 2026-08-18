// SPDX-License-Identifier: Apache-2.0
#include "text.hpp"

#include <algorithm>
#include <array>
#include <cctype>
#include <cstdlib>

#include "random.hpp"

namespace pafwert {

// --------------------------------------------------------------------------
// Generic string utilities
// --------------------------------------------------------------------------

std::vector<std::string> split(std::string_view s, char delim) {
    return split(s, std::string_view(&delim, 1));
}

std::vector<std::string> split(std::string_view s, std::string_view delim) {
    std::vector<std::string> out;
    if (delim.empty()) {
        out.emplace_back(s);
        return out;
    }
    std::size_t start = 0;
    while (true) {
        std::size_t pos = s.find(delim, start);
        if (pos == std::string_view::npos) {
            out.emplace_back(s.substr(start));
            break;
        }
        out.emplace_back(s.substr(start, pos - start));
        start = pos + delim.size();
    }
    return out;
}

std::string trim(std::string_view s) {
    std::size_t b = 0, e = s.size();
    while (b < e && std::isspace(static_cast<unsigned char>(s[b]))) ++b;
    while (e > b && std::isspace(static_cast<unsigned char>(s[e - 1]))) --e;
    return std::string(s.substr(b, e - b));
}

std::string toLower(std::string_view s) {
    std::string out(s);
    std::transform(out.begin(), out.end(), out.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return out;
}

std::string toUpper(std::string_view s) {
    std::string out(s);
    std::transform(out.begin(), out.end(), out.begin(),
                   [](unsigned char c) { return static_cast<char>(std::toupper(c)); });
    return out;
}

std::string replaceAll(std::string s, std::string_view from, std::string_view to) {
    if (from.empty()) return s;
    std::size_t pos = 0;
    while ((pos = s.find(from, pos)) != std::string::npos) {
        s.replace(pos, from.size(), to);
        pos += to.size();
    }
    return s;
}

bool equalsIgnoreCase(std::string_view a, std::string_view b) {
    return a.size() == b.size() &&
           std::equal(a.begin(), a.end(), b.begin(), [](unsigned char x, unsigned char y) {
               return std::tolower(x) == std::tolower(y);
           });
}

long toLong(std::string_view s) {
    std::string t = trim(s);
    return std::strtol(t.c_str(), nullptr, 10);
}

static bool isVowel(char c) {
    return std::string_view("aeiou").find(
               static_cast<char>(std::tolower(static_cast<unsigned char>(c)))) !=
           std::string_view::npos;
}

// --------------------------------------------------------------------------
// Word transforms
// --------------------------------------------------------------------------

std::string bracket(Rng& rng, const std::string& word, std::string_view bracketList) {
    static constexpr std::string_view kDefault =
        "[ ] < > ( ) ( ) ( ) ( ) ( ) ( ) ( ) ( ) [ ] [ ] | | \\ / * * [ ] { } "
        "/ / \\ / / \\ \\ \\ <- -> -> <-";
    std::string_view list = bracketList.empty() ? kDefault : bracketList;
    std::vector<std::string> brackets = split(list, ' ');
    if (brackets.size() < 2) return word;
    std::size_t pairs = brackets.size() / 2;
    std::size_t x = rng.index(pairs) * 2;
    return brackets[x] + word + brackets[x + 1];
}

std::string properCase(std::string word) {
    bool atStart = true;
    for (char& c : word) {
        unsigned char u = static_cast<unsigned char>(c);
        if (std::isalpha(u)) {
            c = static_cast<char>(atStart ? std::toupper(u) : std::tolower(u));
            atStart = false;
        } else if (std::isspace(u)) {
            atStart = true;
        }
    }
    return word;
}

std::string sentenceCase(std::string sentence) {
    sentence = toLower(sentence);
    if (!sentence.empty())
        sentence[0] = static_cast<char>(std::toupper(static_cast<unsigned char>(sentence[0])));
    return sentence;
}

std::string randomCase(Rng& rng, std::string word) {
    if (word.empty()) return word;
    auto upperAt = [&](std::size_t i) {
        word[i] = static_cast<char>(std::toupper(static_cast<unsigned char>(word[i])));
    };
    std::size_t len = word.size();

    switch (rng.range(14, 0)) {
    case 0:
        break;
    case 1:
        word = toUpper(word);
        break;
    case 2:
        word = toLower(word);
        break;
    case 3:
        word = properCase(word);
        break;
    case 4: {  // uppercase every occurrence of one random letter
        char letter = word[static_cast<std::size_t>(rng.range(static_cast<long>(len), 1)) - 1];
        std::string upper(1, static_cast<char>(std::toupper(static_cast<unsigned char>(letter))));
        word = replaceAll(word, std::string(1, letter), upper);
        break;
    }
    case 5:  // totally random
        for (std::size_t i = 0; i < len; ++i)
            if (rng.range(1)) upperAt(i);
        break;
    case 6:
    case 7:  // one random character
        upperAt(static_cast<std::size_t>(rng.range(static_cast<long>(len), 1)) - 1);
        break;
    case 8:  // vowels uppercase
        for (std::size_t i = 0; i < len; ++i)
            if (isVowel(word[i])) upperAt(i);
        break;
    case 9:  // consonants uppercase
        for (std::size_t i = 0; i < len; ++i)
            if (std::isalpha(static_cast<unsigned char>(word[i])) && !isVowel(word[i])) upperAt(i);
        break;
    case 10:  // two consecutive letters
        if (len > 1) {
            std::size_t i =
                static_cast<std::size_t>(rng.range(static_cast<long>(len) - 1, 1)) - 1;
            upperAt(i);
            upperAt(i + 1);
        }
        break;
    case 11:  // last letter
        upperAt(len - 1);
        break;
    case 12:  // first and last letters
        upperAt(0);
        upperAt(len - 1);
        break;
    case 13: {  // first x letters
        std::size_t i = static_cast<std::size_t>(rng.range(static_cast<long>(len), 1, 2));
        for (std::size_t j = 0; j < i && j < len; ++j) upperAt(j);
        break;
    }
    case 14:  // every other letter
        for (std::size_t i = 0; i < len; i += 2) upperAt(i);
        break;
    default:
        break;
    }
    return word;
}

std::string obscure(Rng& rng, std::string word) {
    const std::string original = word;
    long tries = rng.range(20, 2, 2);
    for (long i = 1; i <= tries; ++i) {
        long roll = rng.range(120);
        switch (roll) {
        case 1: word = replaceAll(word, "ate", "8"); break;
        case 2: word = replaceAll(word, "for", "4"); break;
        case 3: word = replaceAll(word, "e", "3"); break;
        case 4: word = replaceAll(word, "l", "1"); break;
        case 5: word = replaceAll(word, "s", "z"); break;
        case 6: word = replaceAll(word, "o", "0"); break;
        case 7: word = replaceAll(word, "a", "@"); break;
        case 8: word = replaceAll(word, "s", "$"); break;
        case 9: word = replaceAll(word, "l", "|"); break;
        case 10: word = replaceAll(word, "ait", "8"); break;
        case 11: word = replaceAll(word, "a", ""); break;
        case 12: word = replaceAll(word, "e", ""); break;
        case 13: word = replaceAll(word, "ou", "u"); break;
        case 14: word = replaceAll(word, "cc", "x"); break;
        case 15: word = replaceAll(word, "oo", "ew"); break;
        case 16: word = replaceAll(word, "and", "&"); break;
        case 17: word = replaceAll(word, "are", "r"); break;
        case 18: word = replaceAll(word, "ks", "x"); break;
        case 19: word = replaceAll(word, "f", "ph"); break;
        case 20: word = replaceAll(word, "ph", "f"); break;
        case 21: word = replaceAll(word, "won", "1"); break;
        case 22: word = replaceAll(word, "l", "r"); break;
        case 23: word = replaceAll(word, "ee", "eee"); break;
        case 24: word = replaceAll(word, "000", "k"); break;
        case 25: word = replaceAll(word, "er", "r"); break;
        case 26: word = replaceAll(word, "ex", "x"); break;
        case 27: word = replaceAll(word, "ecs", "x"); break;
        case 28: word = replaceAll(word, "m", "mm"); break;
        case 29: word = replaceAll(word, "cke", "x0"); break;
        case 30: word = replaceAll(word, "qu", "kw"); break;
        case 31: word = replaceAll(word, "a", "'"); break;
        case 32: word = replaceAll(word, "u", "'"); break;
        case 33: word = replaceAll(word, "ei", "ee"); break;
        case 34: word = replaceAll(word, "one", "own"); break;
        case 35: word = replaceAll(word, "oi", "oy"); break;
        case 36: word = replaceAll(word, "om", "um"); break;
        case 37: word = replaceAll(word, "a", "aa"); break;
        case 38: word = replaceAll(word, "ew", "u"); break;
        case 39: word = replaceAll(word, "us", "is"); break;
        case 40: word = replaceAll(word, "y", "ee"); break;
        case 41: word = replaceAll(word, "sh", "ch"); break;
        case 42: word = replaceAll(word, "to", "2"); break;
        case 43: word = replaceAll(word, "s", "th"); break;
        case 44: word = replaceAll(word, "ck", "q"); break;
        case 45: word = replaceAll(word, "ci", "si"); break;
        case 46: word = replaceAll(word, "ie", "iye"); break;
        case 47: word = replaceAll(word, "tion", "shun"); break;
        case 48: word = replaceAll(word, "r", "w"); break;
        case 49: word = replaceAll(word, "come", "cum"); break;
        case 50: word = replaceAll(word, "cks", "x"); break;
        case 51: word = replaceAll(word, "ight", "ite"); break;
        case 52: word = replaceAll(word, "ing", "'n"); break;
        case 53: word = replaceAll(word, "th", "f"); break;
        case 54: word = replaceAll(word, "tion", "shun"); break;
        case 55: word = replaceAll(word, "too", "2"); break;
        case 56: word = replaceAll(word, "why", "y"); break;
        case 57: word = replaceAll(word, "won", "1"); break;
        case 58: word = replaceAll(word, "your", "yor"); break;
        case 59: word = replaceAll(word, "sc", "sh"); break;
        case 60: word = replaceAll(word, "sh", "th"); break;
        case 61: word = replaceAll(word, "ly", "lee"); break;
        case 62: word = replaceAll(word, "er", "uh"); break;
        case 63: word = replaceAll(word, "er", "a"); break;
        case 64: word = replaceAll(word, "the", "da"); break;
        case 65: word = replaceAll(word, "you", "ya"); break;
        case 66: word = replaceAll(word, "l", "w"); break;
        case 67: word = replaceAll(word, "th", "d"); break;
        case 68: word = replaceAll(word, "a", "u"); break;
        case 69: word = replaceAll(word, "th", "'"); break;
        case 70: word = replaceAll(word, "your", "yer"); break;
        case 71: word = replaceAll(word, "ned", "nt"); break;
        case 72: word = replaceAll(word, "e", "_"); break;
        case 73: word = replaceAll(word, "t", "+"); break;
        case 74: word = replaceAll(word, "e", "="); break;
        case 75: word = replaceAll(word, "can", "kin"); break;
        case 76: word = replaceAll(word, "t", "'"); break;
        case 77: word = replaceAll(word, "ng", "n'"); break;
        case 78: word = replaceAll(word, "red", "hed"); break;
        case 79: word = replaceAll(word, "th", "d"); break;
        case 80: word = replaceAll(word, "he", "eh"); break;
        case 81: word = replaceAll(word, "h", ""); break;
        case 82: word = replaceAll(word, "f", "v"); break;
        case 83: word = replaceAll(word, "ha", "o"); break;
        case 84: word = replaceAll(word, "v", "f"); break;
        case 85: word = replaceAll(word, "v", "b"); break;
        case 86: word = replaceAll(word, "N", "|\\|"); break;
        case 87: word = replaceAll(word, "ll", "dd"); break;
        case 88: word = replaceAll(word, "ll", "tt"); break;
        case 89: word = replaceAll(word, "dd", "tt"); break;
        case 90: word = replaceAll(word, "h", "'"); break;
        case 91: word = replaceAll(word, "o", "a"); break;
        case 92: word = replaceAll(word, "e", "a"); break;
        case 93: word = replaceAll(word, "a", "uh"); break;
        case 94: word = replaceAll(word, "a", "u"); break;
        case 95: word = replaceAll(word, "oo", "u"); break;
        case 96: word = replaceAll(word, "i", "ih"); break;
        case 97: word = replaceAll(word, "a ", "ah"); break;
        case 98: word = replaceAll(word, "s", "ss"); break;
        case 99: word = replaceAll(word, "t", "tt"); break;
        case 100: word = replaceAll(word, "d", "dd"); break;
        case 101: word = replaceAll(word, "at", "@"); break;
        case 102: word = replaceAll(word, " ", ""); break;
        case 103: word = replaceAll(word, "with", "w/"); break;
        case 104: word = replaceAll(word, "t", "d"); break;
        case 105: word = replaceAll(word, "t", "dd"); break;
        case 106: word = replaceAll(word, "d", "t"); break;
        case 107: word = replaceAll(word, "d", "tt"); break;
        case 108: word = replaceAll(word, "cks", "x"); break;
        case 109: word = replaceAll(word, "er", "ah"); break;
        default:
            if (roll >= 110) {
                constexpr std::string_view delims = "-.></:+=\\";
                char delim = delims[rng.index(delims.size())];
                word = replaceAll(
                    word, " ",
                    std::string(static_cast<std::size_t>(rng.range(3, 1)), delim));
            }
            break;
        }
        // If we have done some stuff already, randomly bail out
        if (i >= 2 && word != original && rng.chance(75)) break;
    }
    return word;
}

std::string pigLatin(Rng& rng, const std::string& words) {
    (void)rng;
    std::vector<std::string> parts = split(words, ' ');
    for (std::string& w : parts) {
        if (w.empty()) continue;
        std::string converted;
        if (isVowel(w[0]))
            converted = w + "yay";
        else
            converted = w.substr(1) + w.substr(0, 1) + "ay";
        // keep capitalization of the original word
        if (std::isupper(static_cast<unsigned char>(w[0])) && !converted.empty())
            converted[0] =
                static_cast<char>(std::toupper(static_cast<unsigned char>(converted[0])));
        w = converted;
    }
    std::string out;
    for (std::size_t i = 0; i < parts.size(); ++i) {
        if (i) out += ' ';
        out += parts[i];
    }
    return out;
}

std::string scrambleWord(Rng& rng, std::string word, long times) {
    if (word.size() < 2) return word;
    if (times <= 0) times = 1;
    for (long i = 0; i < times; ++i) {
        std::size_t x1 = rng.index(word.size());
        std::size_t x2 = rng.index(word.size());
        std::swap(word[x1], word[x2]);
    }
    return word;
}

std::string stutter(Rng& rng, const std::string& word) {
    std::string_view markers = rng.range(100) > 20 ? "aeiou" : "hywrtnaeiou";
    for (std::size_t i = 0; i < word.size(); ++i) {
        char c = static_cast<char>(std::tolower(static_cast<unsigned char>(word[i])));
        if (markers.find(c) == std::string_view::npos) continue;
        std::string firstPart = word.substr(0, i + 1);
        std::string stuttered = word;
        if (rng.range(100) < 5) firstPart += "...";
        if (rng.range(100) < 10) firstPart += ' ';
        long reps = rng.range(4, 1, -2);
        for (long j = 0; j < reps; ++j) stuttered = firstPart + stuttered;
        return stuttered;
    }
    return word;
}

std::string pronounceableWord(Rng& rng) {
    static constexpr std::string_view kVowelEndings =
        "ing ers ance ence le ness ings ment ize ate ive ute acy ous ify ought "
        "some edness ed es ly less ment able ible les led ious ant ary iety ist "
        "ism ial ate act ure iac ice aint ent ant ure ide ify les";
    static constexpr std::string_view kConsonantEndings =
        "cked cker tor ter ly rer tic nst lyst onic ght nge nce zer cy ly ny lic "
        "dged red ate ndle ching tching lent ged zen ted nnial lic rly stic se les";
    static constexpr std::string_view kAfterT =
        "ion ity ient ment ance ly less ter tor";

    std::string word;
    bool vowelNext = rng.range(1) != 0;
    long parts = rng.range(5, 4);
    for (long i = 1; i <= parts; ++i) {
        std::size_t len = word.size();
        if (vowelNext) {
            if (rng.range(3) == 0 && len > 1) {
                word += rng.pickOne(kVowelEndings);
                break;
            }
            word += rng.pickOne(charsets::VOWELS2, 2);
        } else {
            if (rng.range(3) == 0 && len) {
                word += rng.pickOne(kConsonantEndings);
                break;
            }
            if (rng.range(3) == 0 && len)
                word += rng.pickOne(charsets::CONSONANTS3);
            else
                word += rng.pickOne(charsets::CONSONANTS2, 2);
            if (!word.empty() && word.back() == 't' && rng.range(2) == 0 && len > 1) {
                word += rng.pickOne(kAfterT);
                break;
            }
        }
        vowelNext = !vowelNext;
    }

    // Some letters shouldn't ever be doubled
    for (auto [from, to] : std::initializer_list<std::pair<const char*, const char*>>{
             {"aa", "a"}, {"hh", "h"}, {"ii", "i"}, {"jj", "j"}, {"kk", "k"},
             {"qq", "qu"}, {"uu", "u"}, {"vv", "v"}, {"ww", "w"}, {"xx", "x"},
             {"yy", "y"}, {"cie", "cei"}})
        word = replaceAll(word, from, to);

    // Don't start a word with double letters
    if (word.size() > 1 && word[0] == word[1]) word.erase(0, 1);
    return word;
}

std::string numberPattern(Rng& rng, long length) {
    if (length <= 0) length = 3;
    std::vector<std::string> digits(static_cast<std::size_t>(length) + 1);
    digits[1] = std::to_string(rng.range(9));
    for (long i = 2; i <= length; ++i) {
        std::size_t idx = static_cast<std::size_t>(i);
        long prev = toLong(digits[idx - 1]);
        switch (rng.range(3, 0)) {
        case 0: digits[idx] = std::to_string(rng.range(9)); break;
        case 1: digits[idx] = digits[static_cast<std::size_t>(rng.range(i))]; break;
        case 2:
            if (prev > 1) digits[idx] = std::to_string(prev - 1);
            break;
        case 3:
            if (prev < 9) digits[idx] = std::to_string(prev + 1);
            break;
        }
    }
    std::string joined;
    for (const std::string& d : digits) joined += d;
    // zero-pad to the requested length, like VB's Format(..., "000...")
    while (joined.size() < static_cast<std::size_t>(length)) joined.insert(0, "0");
    return joined;
}

std::string numberCode(Rng& rng) {
    long repeatDigit = rng.range(9, 0);
    std::string delim = rng.pickOne("- - - - - - - - . . . , / \\ :");
    std::string code;
    while (true) {
        long x = rng.range(9, 0);
        do {
            code += std::to_string(x);
            if (rng.chance(30))
                code += std::to_string(repeatDigit);
            else if (rng.chance(40))
                code += delim;
            if (code.size() > 2) break;
        } while (rng.chance(30));
        if (static_cast<long>(code.size()) > rng.range(4, 3)) break;
        if (rng.chance(10)) code = bracket(rng, code);
        if (rng.chance(15) && code.size() > 2) break;
    }
    if (!code.empty() && !std::isdigit(static_cast<unsigned char>(code.back())))
        code.pop_back();
    return code;
}

std::string sequence(Rng& rng, long length) {
    static constexpr std::string_view kLetters = "abcdefghijklmnopqrstuvwxyz";
    static constexpr std::string_view kNumbers = "1234567890";
    static constexpr std::string_view kKey1 = "qwertyuiop";
    static constexpr std::string_view kKey2 = "asdfghjkl";
    static constexpr std::string_view kKey3 = "zxcvbnm";
    static constexpr std::string_view kKey4 = "poiuytrewq";
    static constexpr std::string_view kKey5 = "lkjhgfdsa";
    static constexpr std::string_view kKey6 = "mnbvcxz";

    if (length <= 0) length = 3;
    std::size_t len = static_cast<std::size_t>(length);

    auto slice = [&](std::string_view s) {
        if (len >= s.size()) return std::string(s);
        std::size_t start = rng.index(s.size() - len + 1);
        return std::string(s.substr(start, len));
    };
    auto zip = [&](std::string_view a, std::string_view b) {
        std::size_t maxStart = std::min(a.size(), b.size());
        std::size_t i = rng.index(maxStart);
        std::string seq;
        do {
            seq += a[i % a.size()];
            seq += b[i % b.size()];
        } while (seq.size() < len);
        return seq;
    };
    auto mirror = [&](std::string_view a) {
        std::size_t i = rng.index(a.size());
        std::string seq;
        do {
            seq += a[i];
            seq += a[a.size() - 1 - i];
        } while (seq.size() < len);
        return seq;
    };

    std::string seq;
    switch (rng.range(19)) {
    case 1: seq = slice(kLetters); break;
    case 2: seq = slice(kNumbers); break;
    case 3: seq = slice(kKey1); break;
    case 4: seq = slice(kKey2); break;
    case 5: seq = slice(kKey3); break;
    case 6: seq = slice(kKey4); break;
    case 7: seq = slice(kKey5); break;
    case 8: seq = slice(kKey6); break;
    case 9: {
        std::size_t i = rng.index(7);
        std::size_t third = std::max<std::size_t>(1, len / 3);
        seq = std::string(kKey1.substr(std::min(i, kKey1.size() - 1), third)) +
              std::string(kKey2.substr(std::min(i, kKey2.size() - 1), third)) +
              std::string(kKey3.substr(std::min(i, kKey3.size() - 1), third));
        break;
    }
    case 10: {
        std::size_t i = rng.index(7);
        std::size_t third = std::max<std::size_t>(1, len / 3);
        seq = std::string(kKey3.substr(std::min(i, kKey3.size() - 1), third)) +
              std::string(kKey2.substr(std::min(i, kKey2.size() - 1), third)) +
              std::string(kKey1.substr(std::min(i, kKey1.size() - 1), third));
        break;
    }
    case 11: seq = zip(kKey1, kKey2); break;
    case 12: seq = zip(kKey2, kKey3); break;
    case 13: seq = zip(kKey2, kKey1); break;
    case 14: seq = zip(kKey1, kNumbers); break;
    case 15: seq = zip(kKey4, kKey5); break;
    case 16: seq = zip(kKey5, kKey6); break;
    case 17: seq = mirror(kKey1); break;
    case 18: seq = mirror(kKey2); break;
    default: seq = mirror(kKey3); break;
    }
    if (seq.size() > len) seq.resize(len);
    return seq;
}

std::string ordinal(long num) {
    std::string n = std::to_string(num);
    long lastTwo = num % 100;
    long last = num % 10;
    if (lastTwo == 11 || lastTwo == 12 || lastTwo == 13) return n + "th";
    switch (last) {
    case 1: return n + "st";
    case 2: return n + "nd";
    case 3: return n + "rd";
    default: return n + "th";
    }
}

std::string phonetic(const std::string& word, long style) {
    static constexpr std::array<std::string_view, 26> kNato = {
        "Alpha", "Bravo", "Charlie", "Delta", "Echo", "Foxtrot", "Golf",
        "Hotel", "India", "Juliet", "Kilo", "Lima", "Mike", "November",
        "Oscar", "Papa", "Quebec", "Romeo", "Sierra", "Tango", "Uniform",
        "Victor", "Whiskey", "X-Ray", "Yankee", "Zulu"};
    static constexpr std::array<std::string_view, 26> kPolice = {
        "Adam", "Baker", "Charles", "David", "Edward", "Frank", "George",
        "Henry", "Ida", "John", "King", "Lincoln", "Mary", "Nora", "Ocean",
        "Paul", "Queen", "Robert", "Sam", "Tom", "Union", "Victor",
        "William", "X-Ray", "Young", "Zebra"};
    const auto& words = (style == 0 || style == 1) ? kNato : kPolice;

    std::string out;
    for (char c : word) {
        unsigned char u = static_cast<unsigned char>(c);
        if (!std::isalpha(u)) continue;
        if (!out.empty()) out += ' ';
        out += words[static_cast<std::size_t>(std::toupper(u) - 'A')];
    }
    return out;
}

std::string toRoman(long num) {
    if (num <= 0) return "No Roman value For 0";
    static constexpr std::pair<long, std::string_view> kValues[] = {
        {1000, "M"}, {900, "CM"}, {500, "D"}, {400, "CD"}, {100, "C"},
        {90, "XC"},  {50, "L"},   {40, "XL"}, {10, "X"},   {9, "IX"},
        {5, "V"},    {4, "IV"},   {1, "I"}};
    std::string out;
    for (auto [value, glyph] : kValues) {
        while (num >= value) {
            out += glyph;
            num -= value;
        }
    }
    return out;
}

static const char* kOnes[] = {"Zero", "One", "Two", "Three", "Four", "Five",
                              "Six", "Seven", "Eight", "Nine", "Ten", "Eleven",
                              "Twelve", "Thirteen", "Fourteen", "Fifteen",
                              "Sixteen", "Seventeen", "Eighteen", "Nineteen"};
static const char* kTens[] = {"", "", "Twenty", "Thirty", "Forty", "Fifty",
                              "Sixty", "Seventy", "Eighty", "Ninety"};

static std::string hundredsTensUnits(long value) {
    std::string out;
    if (value > 99) {
        out += std::string(kOnes[value / 100]) + " Hundred ";
        value %= 100;
    }
    if (value >= 20) {
        out += std::string(kTens[value / 10]) + " ";
        value %= 10;
    }
    if (value > 0) out += std::string(kOnes[value]) + " ";
    return out;
}

std::string numberAsText(const std::string& numberIn) {
    std::string in = trim(numberIn);
    in = replaceAll(in, ",", "");
    if (in.empty()) return "Error - Number improperly formed";

    std::string sign;
    if (in[0] == '+' || in[0] == '-') {
        sign = in[0] == '-' ? "Minus " : "Plus ";
        in.erase(0, 1);
    }

    std::string wholePart = in, decimalPart;
    if (std::size_t dot = in.find('.'); dot != std::string::npos) {
        wholePart = in.substr(0, dot);
        decimalPart = in.substr(dot + 1);
    }
    if (wholePart.empty()) wholePart = "0";
    if (wholePart.size() > 18 ||
        wholePart.find_first_not_of("0123456789") != std::string::npos ||
        decimalPart.find_first_not_of("0123456789") != std::string::npos)
        return "Error - Number improperly formed";

    long long value = std::strtoll(wholePart.c_str(), nullptr, 10);
    std::string out;
    if (value == 0) out = "Zero ";

    static constexpr std::pair<long long, std::string_view> kScales[] = {
        {1000000000000000LL, "Quadrillion"},
        {1000000000000LL, "Trillion"},
        {1000000000LL, "Billion"},
        {1000000LL, "Million"},
        {1000LL, "Thousand"}};
    for (auto [scale, name] : kScales) {
        if (value >= scale) {
            out += hundredsTensUnits(static_cast<long>(value / scale)) +
                   std::string(name) + " ";
            value %= scale;
        }
    }
    if (value > 0) out += hundredsTensUnits(static_cast<long>(value));

    if (!decimalPart.empty()) {
        out += "Point";
        for (char c : decimalPart) out += std::string(" ") + kOnes[c - '0'];
    }
    return sign + out;
}

std::string formatNumber(const std::string& word, const std::string& fmt) {
    if (fmt.empty()) return word;
    if (fmt.find_first_not_of('0') == std::string::npos) {
        // "00" style: zero-pad to the format's width
        std::string digits = trim(word);
        bool negative = !digits.empty() && digits[0] == '-';
        if (negative) digits.erase(0, 1);
        if (digits.find_first_not_of("0123456789") != std::string::npos) return word;
        while (digits.size() < fmt.size()) digits.insert(0, "0");
        return (negative ? "-" : "") + digits;
    }
    if (fmt.find('#') != std::string::npos) {
        // "#,##0" style: thousands grouping
        std::string digits = trim(word);
        bool negative = !digits.empty() && digits[0] == '-';
        if (negative) digits.erase(0, 1);
        if (digits.empty() || digits.find_first_not_of("0123456789") != std::string::npos)
            return word;
        std::string grouped;
        int count = 0;
        for (std::size_t i = digits.size(); i-- > 0;) {
            grouped.insert(grouped.begin(), digits[i]);
            if (++count % 3 == 0 && i > 0) grouped.insert(grouped.begin(), ',');
        }
        return (negative ? "-" : "") + grouped;
    }
    return word;
}

}  // namespace pafwert
