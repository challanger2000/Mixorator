cmake_minimum_required(VERSION 3.25)

project(Mixorator VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# We only need the SDK libraries, validator and VSTGUI support.
# Do not configure/build Steinberg's plug-in examples in CI.
set(SMTG_ENABLE_VST3_PLUGIN_EXAMPLES OFF CACHE BOOL "" FORCE)
set(SMTG_ENABLE_VST3_HOSTING_EXAMPLES ON CACHE BOOL "" FORCE)
set(SMTG_ENABLE_VSTGUI_SUPPORT ON CACHE BOOL "" FORCE)

include(FetchContent)

FetchContent_Declare(
    vst3sdk
    GIT_REPOSITORY https://github.com/steinbergmedia/vst3sdk.git
    GIT_TAG v3.8.1_build_84
)

FetchContent_MakeAvailable(vst3sdk)

enable_testing()

add_subdirectory(src)
add_subdirectory(tests)
