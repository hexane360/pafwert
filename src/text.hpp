// SPDX-License-Identifier: Apache-2.0
// Pafwert — modern C++ port. Original Copyright 2001-2013 Mark Burnett (mb@xato.net)
#pragma once

#include <string>
#include <string_view>
#include <vector>

namespace pafwert {

class Rng;

// --- Character-set constants (ordered by letter frequency where relevant) ---
namespace charsets {
inline constexpr std::string_view VOWELS = "eaoiu";
inline constexpr std::string_view CONSONANTS = "tnshrdlcmfgypwbvkxjqz";
inline constexpr std::string_view SYMBOLS =
    "! @ # % $ ^ & * #lpa# #rpa# #lbr# #rbr# : ' / ` ~ * - < > #pls# = _ #pip# "
    "#sla# #sla# . . , , ; ; ? ? #lba# #rba#";
inline constexpr std::string_view SENTENCEPUNCTUATION = "!;:?.,";
inline constexpr std::string_view ENDPUNCTUATION =
    "! ! ! ! . . . . . . . . . . . . . . . ... ... ? ? ? ? ? ? ?";
inline constexpr std::string_view LETTERS = "etaoinshrdlucmfgypwbvkxjqz";
inline constexpr std::string_view SMILEYS =
    ":) :( :-) :-( :D :0 ;-) ;) :/ 8-) 8-( :-D :-0 :-p :^)";
inline constexpr std::string_view VOWELS2 =
    "a a a a a a a a a e e e e e e e e e e e i i i u u o o ay ea ee ia io oa oi "
    "oo er on re he ha in es io ou";
inline constexpr std::string_view CONSONANTS2 =
    "b b c d d d f g j k m m m n n p p qu r r r s s s s t t t t v w x z z th st "
    "sh ph ch th sh for has tis men";
inline constexpr std::string_view CONSONANTS3 =
    "nd rt dd zz rg ng tt ss mm nn pp nt nc nl ft";
inline constexpr std::string_view KEYBOARD =
    "1234567890`~!@#$%^&*()-_=+]}[{\\|'\";:/?.>,<abcdefghijklmnopqrstuvwxyz"
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
inline constexpr std::string_view NUMROW = "1234567890";
inline constexpr std::string_view NUMROWFULL = "1234567890`~!@#$%^&*()_-+=";
inline constexpr std::string_view ROW1 = "QWERTYUIOP";
inline constexpr std::string_view ROW1FULL = "QWERTYUIOP{[}]|\\";
inline constexpr std::string_view ROW2 = "ASDFGHJKL";
inline constexpr std::string_view ROW2FULL = "ASDFGHJKL;:'\"";
inline constexpr std::string_view ROW3 = "ZXCVBNM";
inline constexpr std::string_view ROW3FULL = "ZXCVBNM,<.>/?";
inline constexpr std::string_view LEFTHAND = "qwertasdfgzxcvb";
inline constexpr std::string_view RIGHTHAND = "yuiophjknm";
inline constexpr std::string_view DIGRAPHS =
    "th er on an re he in ed nd ha at en es of or nt ea ti to it st io le is ou "
    "ar as de rt ve";
inline constexpr std::string_view TWOLETTERWORDS =
    "of to in it is be as at so we he by or on do if me my up an go no us am";
inline constexpr std::string_view THREELETTERWORDS =
    "the and for are but not you all any can had her was one our out day get "
    "has him his how man new now old see two way who boy did its let put say "
    "she too use";
inline constexpr std::string_view LONGMONTHS =
    "January February March April May June July August September October "
    "November December";
inline constexpr std::string_view SHORTMONTHS =
    "Jan Feb Mar Apr May Jun Jul Aug Sep Oct Nov Dec";
inline constexpr std::string_view LONGDAYS =
    "Monday Tuesday Wednesday Thursday Friday Saturday Sunday";
inline constexpr std::string_view SHORTDAYS = "Mon Tue Wed Thu Fri Sat Sun";
}  // namespace charsets

// --- Generic string utilities ---
std::vector<std::string> split(std::string_view s, char delim);
std::vector<std::string> split(std::string_view s, std::string_view delim);
std::string trim(std::string_view s);
std::string toLower(std::string_view s);
std::string toUpper(std::string_view s);
std::string replaceAll(std::string s, std::string_view from, std::string_view to);
bool equalsIgnoreCase(std::string_view a, std::string_view b);
long toLong(std::string_view s);  // VB Val(): leading number or 0

// --- Word transforms (ports of Main.bas / NumText.bas helpers) ---
std::string bracket(Rng& rng, const std::string& word, std::string_view bracketList = {});
std::string properCase(std::string word);
std::string sentenceCase(std::string sentence);
std::string randomCase(Rng& rng, std::string word);
std::string obscure(Rng& rng, std::string word);
std::string pigLatin(Rng& rng, const std::string& words);
std::string scrambleWord(Rng& rng, std::string word, long times = 1);
std::string stutter(Rng& rng, const std::string& word);
std::string pronounceableWord(Rng& rng);
std::string numberPattern(Rng& rng, long length);
std::string numberCode(Rng& rng);
std::string sequence(Rng& rng, long length);
std::string ordinal(long num);
std::string phonetic(const std::string& word, long style = 1);
std::string toRoman(long num);
std::string numberAsText(const std::string& numberIn);
std::string formatNumber(const std::string& word, const std::string& fmt);

}  // namespace pafwert
