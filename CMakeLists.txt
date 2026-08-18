cmake_minimum_required(VERSION 3.21)

project(pafwert
    VERSION 2.0.0
    DESCRIPTION "Pattern-based memorable password generator"
    HOMEPAGE_URL "https://github.com/mburnett/pafwert"
    LANGUAGES CXX)

include(GNUInstallDirs)

option(PAFWERT_BUILD_TESTS "Build the pafwert smoke tests" ${PROJECT_IS_TOP_LEVEL})

add_library(pafwert_core STATIC
    src/engine.cpp
    src/paths.cpp
    src/random.cpp
    src/text.cpp
    src/words.cpp)
target_compile_features(pafwert_core PUBLIC cxx_std_20)
target_include_directories(pafwert_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_compile_definitions(pafwert_core PRIVATE
    PAFWERT_DATADIR="${CMAKE_INSTALL_FULL_DATADIR}/pafwert")

add_executable(pafwert src/main.cpp)
target_link_libraries(pafwert PRIVATE pafwert_core)

install(TARGETS pafwert RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

# Wordlists and the pattern list are runtime data, installed under
# <prefix>/share/pafwert/wordlists where the XDG data-dir search finds them.
# Slurs.txt and _DictCheck.dat are only used by the GUI's password *checker*
# (not ported), so they are not installed.
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/Wordlists/
    DESTINATION ${CMAKE_INSTALL_DATADIR}/pafwert/wordlists
    FILES_MATCHING
        PATTERN "*.txt"
        PATTERN "*.cfg"
        PATTERN "Slurs.txt" EXCLUDE)

if(PAFWERT_BUILD_TESTS)
    enable_testing()
    add_test(NAME generate
        COMMAND pafwert -n 25 --wordlists ${CMAKE_CURRENT_SOURCE_DIR}/Wordlists)
    add_test(NAME fixed_pattern
        COMMAND pafwert -n 5 -p "{Word(adjective)} {Word(noun)}{number(99)[50]}"
                --wordlists ${CMAKE_CURRENT_SOURCE_DIR}/Wordlists)
    add_test(NAME check_valid_pattern
        COMMAND pafwert --check "{Word(noun)+propercase}{symbol}{number(99)}"
                --wordlists ${CMAKE_CURRENT_SOURCE_DIR}/Wordlists)
    add_test(NAME check_invalid_pattern
        COMMAND pafwert --check "{Word(noun)"
                --wordlists ${CMAKE_CURRENT_SOURCE_DIR}/Wordlists)
    set_tests_properties(check_invalid_pattern PROPERTIES WILL_FAIL TRUE)
endif()
