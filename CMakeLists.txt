cmake_minimum_required(VERSION 3.22)

project(Mixorator VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

FetchContent_Declare(
    vst3sdk
    GIT_REPOSITORY https://github.com/steinbergmedia/vst3sdk.git
    GIT_TAG v3.8.0_build_66
)

FetchContent_MakeAvailable(vst3sdk)

add_subdirectory(src)
