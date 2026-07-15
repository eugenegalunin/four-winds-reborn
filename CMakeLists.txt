cmake_minimum_required(VERSION 3.14)

project(FourWindsReborn VERSION 20260715.1)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(SWE_WITH_JSON ON CACHE BOOL "enable json" FORCE)
set(SWE_WITH_LUA OFF CACHE BOOL "disable lua" FORCE)
set(SWE_EXAMPLES OFF CACHE BOOL "disable examples" FORCE)
set(SWE_DISABLE_TERMGUI OFF CACHE BOOL "enable termwin_gui" FORCE)

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DBUILD_DEBUG")

add_subdirectory(engine)

set(FOUR_WINDS_SOURCE
    src/strings.cpp
    src/gamedata.cpp
    src/runewars.cpp
    src/gametheme.cpp
    src/gameobjects.cpp
    src/settings.cpp
    src/aiturn.cpp
    src/aiprofile.cpp
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

target_link_libraries(four_winds_reborn libswe)
set_target_properties(four_winds_reborn PROPERTIES
    OUTPUT_NAME four-winds-reborn
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

option(BUILD_TESTING "Build gameplay regression tests" OFF)
include(CTest)
if(BUILD_TESTING)
    add_executable(gameplay_regression_tests
        tests/gameplay_regression_tests.cpp
        tests/avatar_balance_tests.cpp
        ${FOUR_WINDS_SOURCE})
    target_compile_definitions(gameplay_regression_tests PRIVATE
        RUNEWARS_NO_MAIN
        FOUR_WINDS_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
    target_link_libraries(gameplay_regression_tests libswe)
    add_test(NAME gameplay_regressions COMMAND gameplay_regression_tests)
    add_test(NAME avatar_balance_metrics COMMAND gameplay_regression_tests --balance-only)
endif()
