cmake_minimum_required(VERSION 3.14)

project(FourWindsReborn VERSION 0.2.0)

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
set(FOUR_WINDS_RELEASE_TAG "")
string(TIMESTAMP FOUR_WINDS_BUILD_DATE "%Y-%m-%d")
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
        COMMAND "${GIT_EXECUTABLE}" -c "safe.directory=${CMAKE_CURRENT_SOURCE_DIR}"
            -C "${CMAKE_CURRENT_SOURCE_DIR}" describe --tags --exact-match --match "v[0-9]*"
        OUTPUT_VARIABLE FOUR_WINDS_RELEASE_TAG
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET)

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

option(FOUR_WINDS_WINDOWS_GUI
    "Build the Windows executable without an attached console"
    OFF)

set(FOUR_WINDS_BUILD_DEFINITIONS
    FOUR_WINDS_VERSION="${PROJECT_VERSION}"
    FOUR_WINDS_BUILD_DATE="${FOUR_WINDS_BUILD_DATE}"
    FOUR_WINDS_RELEASE_TAG="${FOUR_WINDS_RELEASE_TAG}"
    FOUR_WINDS_GAME_REVISION="${FOUR_WINDS_GAME_REVISION}"
    FOUR_WINDS_ENGINE_REVISION="${FOUR_WINDS_ENGINE_REVISION}")

set(FOUR_WINDS_SOURCE
    src/crashreport.cpp
    src/gameplayrng.cpp
    src/matchscore.cpp
    src/recovery.cpp
    src/savegames.cpp
    src/replay.cpp
    src/simulation.cpp
    src/developertools.cpp
    src/developerfixtures.cpp
    src/strings.cpp
    src/gamedata.cpp
    src/gamestateio.cpp
    src/runegamecommands.cpp
    src/actionvalidation.cpp
    src/adventurecommands.cpp
    src/runewars.cpp
    src/intropart.cpp
    src/mainmenu.cpp
    src/encyclopedia.cpp
    src/settingsmenu.cpp
    src/loadrecovery.cpp
    src/gametheme.cpp
    src/gameobjects.cpp
    src/runegamestate.cpp
    src/runegamescoring.cpp
    src/playerstate.cpp
    src/creaturestate.cpp
    src/battlestate.cpp
    src/battleentities.cpp
    src/battlesession.cpp
    src/battlecommands.cpp
    src/settings.cpp
    src/aiturn.cpp
    src/aiprofile.cpp
    src/aistrategy.cpp
    src/aiadventure.cpp
    src/aibattle.cpp
    src/aispell.cpp
    src/battle.cpp
    src/battleuitext.cpp
    src/battlechoicedialog.cpp
    src/dialogs.cpp
    src/adventurehints.cpp
    src/adventurepart.cpp
    src/runegameinput.cpp
    src/runegametargetdialogs.cpp
    src/runegamewidgets.cpp
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
    OUTPUT_NAME four-winds-reborn
    WIN32_EXECUTABLE ${FOUR_WINDS_WINDOWS_GUI})

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
    set_tests_properties(gameplay_regressions PROPERTIES
        ENVIRONMENT "FOUR_WINDS_RECOVERY_DIR=${CMAKE_BINARY_DIR}/recovery-test-gameplay;FOUR_WINDS_SAVE_DIR=${CMAKE_BINARY_DIR}/save-test-gameplay")
    add_test(NAME avatar_balance_metrics COMMAND gameplay_regression_tests --balance-only)
    set_tests_properties(avatar_balance_metrics PROPERTIES
        ENVIRONMENT "FOUR_WINDS_RECOVERY_DIR=${CMAKE_BINARY_DIR}/recovery-test-balance;FOUR_WINDS_SAVE_DIR=${CMAKE_BINARY_DIR}/save-test-balance")
    add_test(NAME crash_reporting COMMAND gameplay_regression_tests --crash-report-self-test)
    set_tests_properties(crash_reporting PROPERTIES
        ENVIRONMENT "FOUR_WINDS_DIAGNOSTICS_DIR=${CMAKE_BINARY_DIR}/diagnostics-test;FOUR_WINDS_RECOVERY_DIR=${CMAKE_BINARY_DIR}/recovery-test-crash;FOUR_WINDS_SAVE_DIR=${CMAKE_BINARY_DIR}/save-test-crash")
    add_test(NAME recovery_checkpointing COMMAND gameplay_regression_tests --recovery-self-test)
    set_tests_properties(recovery_checkpointing PROPERTIES
        ENVIRONMENT "FOUR_WINDS_RECOVERY_DIR=${CMAKE_BINARY_DIR}/recovery-test;FOUR_WINDS_SAVE_DIR=${CMAKE_BINARY_DIR}/save-test")
    add_test(NAME settings_persistence COMMAND gameplay_regression_tests --settings-self-test)
    set_tests_properties(settings_persistence PROPERTIES
        ENVIRONMENT "FOUR_WINDS_SETTINGS_FILE=${CMAKE_BINARY_DIR}/settings-test/settings.json;FOUR_WINDS_RECOVERY_DIR=${CMAKE_BINARY_DIR}/recovery-test-settings;FOUR_WINDS_SAVE_DIR=${CMAKE_BINARY_DIR}/save-test-settings")
    add_test(NAME fixed_seed_replay COMMAND gameplay_regression_tests --fixed-seed-replay)
    set_tests_properties(fixed_seed_replay PROPERTIES
        ENVIRONMENT "FOUR_WINDS_RECOVERY_DIR=${CMAKE_BINARY_DIR}/recovery-test-replay;FOUR_WINDS_SAVE_DIR=${CMAKE_BINARY_DIR}/save-test-replay")
    add_test(NAME headless_match_simulation
        COMMAND gameplay_regression_tests --headless-match-self-test)
    set_tests_properties(headless_match_simulation PROPERTIES
        ENVIRONMENT "FOUR_WINDS_RECOVERY_DIR=${CMAKE_BINARY_DIR}/recovery-test-headless;FOUR_WINDS_SAVE_DIR=${CMAKE_BINARY_DIR}/save-test-headless")
    if(WIN32)
        add_test(NAME windows_native_crash_reporting
            COMMAND gameplay_regression_tests --windows-crash-report-self-test)
        set_tests_properties(windows_native_crash_reporting PROPERTIES
            ENVIRONMENT "FOUR_WINDS_DIAGNOSTICS_DIR=${CMAKE_BINARY_DIR}/diagnostics-native-test;FOUR_WINDS_RECOVERY_DIR=${CMAKE_BINARY_DIR}/recovery-test-native-crash;FOUR_WINDS_SAVE_DIR=${CMAKE_BINARY_DIR}/save-test-native-crash")
    endif()
endif()
