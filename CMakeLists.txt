cmake_minimum_required(VERSION 3.14)

project(FourWindsReborn VERSION 20260716.2)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(SWE_WITH_JSON ON CACHE BOOL "enable json" FORCE)
set(SWE_WITH_LUA OFF CACHE BOOL "disable lua" FORCE)
set(SWE_EXAMPLES OFF CACHE BOOL "disable examples" FORCE)
set(SWE_DISABLE_TERMGUI OFF CACHE BOOL "enable termwin_gui" FORCE)

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DBUILD_DEBUG")

add_subdirectory(engine)

set(FOUR_WINDS_GAME_REVISION "unknown")
set(FOUR_WINDS_ENGINE_REVISION "unknown")
find_package(Git QUIET)
if(GIT_FOUND)
    execute_process(
        COMMAND "${GIT_EXECUTABLE}" -c "safe.directory=${CMAKE_CURRENT_SOURCE_DIR}"
            -C "${CMAKE_CURRENT_SOURCE_DIR}" rev-parse HEAD
        OUTPUT_VARIABLE FOUR_WINDS_GAME_REVISION
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET)
    execute_process(
        COMMAND "${GIT_EXECUTABLE}" -c "safe.directory=${CMAKE_CURRENT_SOURCE_DIR}"
            -C "${CMAKE_CURRENT_SOURCE_DIR}" diff --quiet --ignore-space-at-eol
            --ignore-submodules=dirty
        RESULT_VARIABLE FOUR_WINDS_GAME_DIRTY
        ERROR_QUIET)
    if(FOUR_WINDS_GAME_DIRTY EQUAL 1)
        string(APPEND FOUR_WINDS_GAME_REVISION "-dirty")
    endif()

    execute_process(
        COMMAND "${GIT_EXECUTABLE}" -c "safe.directory=${CMAKE_CURRENT_SOURCE_DIR}/engine"
            -C "${CMAKE_CURRENT_SOURCE_DIR}/engine" rev-parse HEAD
        OUTPUT_VARIABLE FOUR_WINDS_ENGINE_REVISION
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET)
    execute_process(
        COMMAND "${GIT_EXECUTABLE}" -c "safe.directory=${CMAKE_CURRENT_SOURCE_DIR}/engine"
            -C "${CMAKE_CURRENT_SOURCE_DIR}/engine" diff --quiet --ignore-space-at-eol
        RESULT_VARIABLE FOUR_WINDS_ENGINE_DIRTY
        ERROR_QUIET)
    if(FOUR_WINDS_ENGINE_DIRTY EQUAL 1)
        string(APPEND FOUR_WINDS_ENGINE_REVISION "-dirty")
    endif()
endif()
if(NOT FOUR_WINDS_GAME_REVISION)
    set(FOUR_WINDS_GAME_REVISION "unknown")
endif()
if(NOT FOUR_WINDS_ENGINE_REVISION)
    set(FOUR_WINDS_ENGINE_REVISION "unknown")
endif()

set(FOUR_WINDS_BUILD_DEFINITIONS
    FOUR_WINDS_VERSION="${PROJECT_VERSION}"
    FOUR_WINDS_GAME_REVISION="${FOUR_WINDS_GAME_REVISION}"
    FOUR_WINDS_ENGINE_REVISION="${FOUR_WINDS_ENGINE_REVISION}")

set(FOUR_WINDS_SOURCE
    src/crashreport.cpp
    src/gameplayrng.cpp
    src/matchscore.cpp
    src/recovery.cpp
    src/replay.cpp
    src/simulation.cpp
    src/strings.cpp
    src/gamedata.cpp
    src/runewars.cpp
    src/gametheme.cpp
    src/gameobjects.cpp
    src/settings.cpp
    src/aiturn.cpp
    src/aiprofile.cpp
    src/aistrategy.cpp
    src/aiadventure.cpp
    src/aibattle.cpp
    src/aispell.cpp
    src/battle.cpp
    src/dialogs.cpp
    src/adventurepart.cpp
    src/battlesummarypart.cpp
    src/mahjongsummarypart.cpp
    src/gamesummarypart.cpp
    src/mahjongpart.cpp
    src/jsongui.cpp
    src/showplayers.cpp
    src/selectperson.cpp)

include_directories(engine src)
add_executable(four_winds_reborn ${FOUR_WINDS_SOURCE})
target_compile_definitions(four_winds_reborn PRIVATE ${FOUR_WINDS_BUILD_DEFINITIONS})

target_link_libraries(four_winds_reborn libswe)
if(WIN32)
    target_link_libraries(four_winds_reborn dbghelp)
endif()
set_target_properties(four_winds_reborn PROPERTIES
    OUTPUT_NAME four-winds-reborn)

option(BUILD_TESTING "Build gameplay regression tests" OFF)
include(CTest)
if(BUILD_TESTING)
    find_package(Python3 REQUIRED COMPONENTS Interpreter)
    add_test(NAME theme_data_validation
        COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/scripts/validate-json.py")

    add_executable(gameplay_regression_tests
        tests/gameplay_regression_tests.cpp
        tests/avatar_balance_tests.cpp
        src/tournament.cpp
        ${FOUR_WINDS_SOURCE})
    target_compile_definitions(gameplay_regression_tests PRIVATE
        RUNEWARS_NO_MAIN
        ${FOUR_WINDS_BUILD_DEFINITIONS}
        FOUR_WINDS_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
    target_link_libraries(gameplay_regression_tests libswe)
    if(WIN32)
        target_link_libraries(gameplay_regression_tests dbghelp)
    endif()
    add_test(NAME gameplay_regressions COMMAND gameplay_regression_tests)
    add_test(NAME avatar_balance_metrics COMMAND gameplay_regression_tests --balance-only)
    add_test(NAME crash_reporting COMMAND gameplay_regression_tests --crash-report-self-test)
    set_tests_properties(crash_reporting PROPERTIES
        ENVIRONMENT "FOUR_WINDS_DIAGNOSTICS_DIR=${CMAKE_BINARY_DIR}/diagnostics-test")
    add_test(NAME recovery_checkpointing COMMAND gameplay_regression_tests --recovery-self-test)
    set_tests_properties(recovery_checkpointing PROPERTIES
        ENVIRONMENT "FOUR_WINDS_RECOVERY_DIR=${CMAKE_BINARY_DIR}/recovery-test")
    add_test(NAME fixed_seed_replay COMMAND gameplay_regression_tests --fixed-seed-replay)
    add_test(NAME headless_match_simulation
        COMMAND gameplay_regression_tests --headless-match-self-test)
    if(WIN32)
        add_test(NAME windows_native_crash_reporting
            COMMAND gameplay_regression_tests --windows-crash-report-self-test)
        set_tests_properties(windows_native_crash_reporting PROPERTIES
            ENVIRONMENT "FOUR_WINDS_DIAGNOSTICS_DIR=${CMAKE_BINARY_DIR}/diagnostics-native-test")
    endif()
endif()
