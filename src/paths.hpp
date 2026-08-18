// SPDX-License-Identifier: Apache-2.0
// Pafwert — modern C++ port. Original Copyright 2001-2013 Mark Burnett (mb@xato.net)
#pragma once

#include <filesystem>
#include <optional>
#include <vector>

namespace pafwert {

// XDG base directories (with sensible fallbacks when the variables are unset,
// including on Windows where APPDATA/LOCALAPPDATA stand in for the defaults).
std::filesystem::path xdgConfigHome();
std::filesystem::path xdgDataHome();
std::vector<std::filesystem::path> xdgDataDirs();

// Path of the user configuration file: $XDG_CONFIG_HOME/pafwert/pafwert.conf
std::filesystem::path userConfigFile();

// True if `dir` looks like a Pafwert wordlist directory (contains patterns.cfg,
// matched case-insensitively).
bool isWordlistDir(const std::filesystem::path& dir);

// Locate the wordlist directory. Search order:
//   1. explicit override (--wordlists flag)
//   2. $PAFWERT_WORDLIST_DIR
//   3. `configuredDir` (value read from the config file, if any)
//   4. $XDG_DATA_HOME/pafwert/wordlists
//   5. each of $XDG_DATA_DIRS + /pafwert/wordlists
//   6. the compiled-in install location
//   7. ./Wordlists (running from a source checkout)
std::optional<std::filesystem::path> findWordlistDir(
    const std::optional<std::filesystem::path>& override_,
    const std::optional<std::filesystem::path>& configuredDir);

}  // namespace pafwert
