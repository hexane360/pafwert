// SPDX-License-Identifier: Apache-2.0
#include "words.hpp"

#include <fstream>

#include "random.hpp"
#include "text.hpp"

namespace pafwert {

namespace fs = std::filesystem;

static std::vector<std::string> readLines(const fs::path& file) {
    std::ifstream in(file, std::ios::binary);
    if (!in) throw WordlistError("cannot open " + file.string());
    std::vector<std::string> lines;
    std::string line;
    while (std::getline(in, line)) {
        while (!line.empty() && (line.back() == '\r' || line.back() == '\n'))
            line.pop_back();
        if (!trim(line).empty()) lines.push_back(line);
    }
    return lines;
}

WordRepo::WordRepo(fs::path dir) : dir_(std::move(dir)) {
    std::error_code ec;
    for (const auto& entry : fs::directory_iterator(dir_, ec)) {
        if (entry.is_regular_file(ec))
            index_.emplace(toLower(entry.path().filename().string()), entry.path());
    }
    if (index_.empty())
        throw WordlistError("wordlist directory is empty or unreadable: " + dir_.string());
}

fs::path WordRepo::resolve(std::string_view name) const {
    std::string file = trim(name);
    if (file.empty() || file.find("..") != std::string::npos ||
        file.find('/') != std::string::npos || file.find('\\') != std::string::npos)
        throw WordlistError("invalid wordlist name: '" + file + "'");
    std::string key = toLower(file);
    if (!key.ends_with(".txt")) key += ".txt";
    if (auto it = index_.find(key); it != index_.end()) return it->second;
    throw WordlistError("wordlist not found: " + file);
}

const std::vector<std::string>& WordRepo::list(std::string_view name) {
    fs::path path = resolve(name);
    std::string key = path.string();
    if (auto it = cache_.find(key); it != cache_.end()) return it->second;
    std::vector<std::string> lines = readLines(path);
    if (lines.empty()) throw WordlistError("wordlist is empty: " + path.string());
    return cache_.emplace(key, std::move(lines)).first->second;
}

std::string WordRepo::randomWord(std::string_view name, Rng& rng) {
    const std::vector<std::string>& words = list(name);
    return trim(words[rng.index(words.size())]);
}

const std::vector<std::string>& WordRepo::patterns() {
    if (patternsLoaded_) return patterns_;
    fs::path cfg;
    if (auto it = index_.find("patterns.cfg"); it != index_.end())
        cfg = it->second;
    else
        throw WordlistError("patterns.cfg not found in " + dir_.string());

    for (std::string line : readLines(cfg)) {
        std::string t = trim(line);
        if (t.empty() || t[0] == '#') continue;
        // Strip the "Pattern Name:" label if present
        if (std::size_t colon = t.find(':'); colon != std::string::npos)
            t = trim(t.substr(colon + 1));
        if (!t.empty()) patterns_.push_back(t);
    }
    if (patterns_.empty())
        throw WordlistError("no patterns found in " + cfg.string());
    patternsLoaded_ = true;
    return patterns_;
}

}  // namespace pafwert
