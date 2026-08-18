// SPDX-License-Identifier: Apache-2.0
// Pafwert — modern C++ port. Original Copyright 2001-2013 Mark Burnett (mb@xato.net)
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <optional>
#include <string>

#include "engine.hpp"
#include "paths.hpp"
#include "random.hpp"
#include "text.hpp"
#include "words.hpp"

namespace fs = std::filesystem;
using namespace pafwert;

namespace {

constexpr std::string_view kVersion = "2.0.0";

void printUsage(std::ostream& out) {
    out << "Usage: pafwert [options]\n"
           "\n"
           "Generate strong but memorable passwords using patterns and wordlists.\n"
           "\n"
           "Options:\n"
           "  -n, --count N        number of passwords to generate (default 12)\n"
           "  -p, --pattern PAT    generate from a specific pattern instead of a\n"
           "                       random one from patterns.cfg\n"
           "  -k, --keywords LIST  space-separated keywords to mix into passwords\n"
           "  -d, --wordlists DIR  wordlist directory (containing patterns.cfg)\n"
           "  -v, --verbose        also show the pattern used for each password\n"
           "      --check PAT      validate a pattern and exit\n"
           "  -h, --help           show this help\n"
           "      --version        show version information\n"
           "\n"
           "Configuration is read from $XDG_CONFIG_HOME/pafwert/pafwert.conf\n"
           "(key = value; recognized keys: wordlists, count). Wordlists are searched\n"
           "in $PAFWERT_WORDLIST_DIR, $XDG_DATA_HOME/pafwert/wordlists, then each\n"
           "$XDG_DATA_DIRS entry and the install location.\n";
}

std::map<std::string, std::string> readConfig(const fs::path& file) {
    std::map<std::string, std::string> config;
    std::ifstream in(file);
    std::string line;
    while (std::getline(in, line)) {
        std::string t = trim(line);
        if (t.empty() || t[0] == '#') continue;
        if (std::size_t eq = t.find('='); eq != std::string::npos)
            config[toLower(trim(t.substr(0, eq)))] = trim(t.substr(eq + 1));
    }
    return config;
}

}  // namespace

int main(int argc, char* argv[]) {
    long count = 12;
    std::string pattern;
    std::string keywords;
    std::optional<fs::path> wordlistOverride;
    std::optional<std::string> checkArg;
    bool verbose = false;

    auto config = readConfig(userConfigFile());
    if (auto it = config.find("count"); it != config.end())
        if (long n = toLong(it->second); n > 0) count = n;
    std::optional<fs::path> configuredDir;
    if (auto it = config.find("wordlists"); it != config.end() && !it->second.empty())
        configuredDir = fs::path(it->second);

    for (int i = 1; i < argc; ++i) {
        std::string_view arg = argv[i];
        auto needsValue = [&](std::string_view name) -> std::string {
            if (i + 1 >= argc) {
                std::cerr << "pafwert: option " << name << " requires a value\n";
                std::exit(2);
            }
            return argv[++i];
        };
        if (arg == "-h" || arg == "--help") {
            printUsage(std::cout);
            return 0;
        } else if (arg == "--version") {
            std::cout << "pafwert " << kVersion
                      << " (C++ port of Pafwert by Mark Burnett)\n";
            return 0;
        } else if (arg == "-n" || arg == "--count") {
            count = toLong(needsValue(arg));
            if (count <= 0) {
                std::cerr << "pafwert: invalid count\n";
                return 2;
            }
        } else if (arg == "-p" || arg == "--pattern") {
            pattern = needsValue(arg);
        } else if (arg == "-k" || arg == "--keywords") {
            keywords = needsValue(arg);
        } else if (arg == "-d" || arg == "--wordlists") {
            wordlistOverride = fs::path(needsValue(arg));
        } else if (arg == "-v" || arg == "--verbose") {
            verbose = true;
        } else if (arg == "--check") {
            checkArg = needsValue(arg);
        } else {
            std::cerr << "pafwert: unknown option '" << arg << "'\n\n";
            printUsage(std::cerr);
            return 2;
        }
    }

    auto dir = findWordlistDir(wordlistOverride, configuredDir);
    if (!dir) {
        std::cerr << "pafwert: could not find a wordlist directory containing "
                     "patterns.cfg.\n"
                     "Searched --wordlists, $PAFWERT_WORDLIST_DIR, the config file, "
                     "$XDG_DATA_HOME/pafwert/wordlists and $XDG_DATA_DIRS.\n";
        return 1;
    }

    try {
        Rng rng;
        WordRepo repo(*dir);
        Engine engine(repo, rng);

        if (checkArg) {
            std::string error = engine.checkPattern(*checkArg);
            if (error.empty()) {
                std::cout << "Pattern OK\n";
                return 0;
            }
            std::cerr << error << '\n';
            return 1;
        }

        for (long i = 0; i < count; ++i) {
            Engine::Result result = engine.generate(pattern, keywords);
            if (verbose)
                std::cout << result.password << "\t# " << result.pattern << '\n';
            else
                std::cout << result.password << '\n';
        }
    } catch (const std::exception& e) {
        std::cerr << "pafwert: " << e.what() << '\n';
        return 1;
    }
    return 0;
}
