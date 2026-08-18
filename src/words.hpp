// SPDX-License-Identifier: Apache-2.0
// Pafwert — modern C++ port. Original Copyright 2001-2013 Mark Burnett (mb@xato.net)
#pragma once

#include <filesystem>
#include <map>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace pafwert {

class Rng;

class WordlistError : public std::runtime_error {
    using std::runtime_error::runtime_error;
};

// Loads and caches wordlists and the pattern file from a wordlist directory.
// Filenames are matched case-insensitively so patterns written on Windows keep
// working on case-sensitive filesystems.
class WordRepo {
public:
    explicit WordRepo(std::filesystem::path dir);

    const std::filesystem::path& dir() const { return dir_; }

    // Words from `<name>.txt` (".txt" is appended if missing). Throws
    // WordlistError if the list does not exist or is empty.
    const std::vector<std::string>& list(std::string_view name);

    std::string randomWord(std::string_view name, Rng& rng);

    // Patterns from patterns.cfg: comments and blank lines removed, and the
    // leading "Name:" labels stripped.
    const std::vector<std::string>& patterns();

private:
    std::filesystem::path resolve(std::string_view name) const;

    std::filesystem::path dir_;
    std::map<std::string, std::filesystem::path> index_;  // lowercase name -> path
    std::map<std::string, std::vector<std::string>> cache_;
    std::vector<std::string> patterns_;
    bool patternsLoaded_ = false;
};

}  // namespace pafwert
