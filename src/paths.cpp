// SPDX-License-Identifier: Apache-2.0
#include "paths.hpp"

#include <cstdlib>
#include <string>

#include "text.hpp"

namespace pafwert {

namespace fs = std::filesystem;

static std::optional<fs::path> envPath(const char* name) {
    if (const char* value = std::getenv(name); value && *value) return fs::path(value);
    return std::nullopt;
}

static fs::path homeDir() {
    if (auto home = envPath("HOME")) return *home;
    if (auto profile = envPath("USERPROFILE")) return *profile;
    return fs::current_path();
}

fs::path xdgConfigHome() {
    if (auto dir = envPath("XDG_CONFIG_HOME")) return *dir;
#ifdef _WIN32
    if (auto dir = envPath("APPDATA")) return *dir;
#endif
    return homeDir() / ".config";
}

fs::path xdgDataHome() {
    if (auto dir = envPath("XDG_DATA_HOME")) return *dir;
#ifdef _WIN32
    if (auto dir = envPath("LOCALAPPDATA")) return *dir;
#endif
    return homeDir() / ".local" / "share";
}

std::vector<fs::path> xdgDataDirs() {
    std::vector<fs::path> dirs;
    if (auto value = envPath("XDG_DATA_DIRS")) {
        for (const std::string& part : split(value->string(), ':'))
            if (!part.empty()) dirs.emplace_back(part);
    }
    if (dirs.empty()) {
        dirs.emplace_back("/usr/local/share");
        dirs.emplace_back("/usr/share");
    }
    return dirs;
}

fs::path userConfigFile() {
    return xdgConfigHome() / "pafwert" / "pafwert.conf";
}

bool isWordlistDir(const fs::path& dir) {
    std::error_code ec;
    if (!fs::is_directory(dir, ec)) return false;
    for (const auto& entry : fs::directory_iterator(dir, ec)) {
        if (entry.is_regular_file(ec) &&
            equalsIgnoreCase(entry.path().filename().string(), "patterns.cfg"))
            return true;
    }
    return false;
}

std::optional<fs::path> findWordlistDir(
    const std::optional<fs::path>& override_,
    const std::optional<fs::path>& configuredDir) {
    std::vector<fs::path> candidates;
    if (override_) candidates.push_back(*override_);
    if (auto dir = envPath("PAFWERT_WORDLIST_DIR")) candidates.push_back(*dir);
    if (configuredDir) candidates.push_back(*configuredDir);
    candidates.push_back(xdgDataHome() / "pafwert" / "wordlists");
    for (const fs::path& dir : xdgDataDirs())
        candidates.push_back(dir / "pafwert" / "wordlists");
#ifdef PAFWERT_DATADIR
    candidates.push_back(fs::path(PAFWERT_DATADIR) / "wordlists");
#endif
    candidates.push_back(fs::current_path() / "Wordlists");
    candidates.push_back(fs::current_path() / "wordlists");

    for (const fs::path& dir : candidates)
        if (isWordlistDir(dir)) return dir;
    return std::nullopt;
}

}  // namespace pafwert
