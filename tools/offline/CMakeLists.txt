include(FetchContent)

# miniaudio provides a small cross-platform decoder for WAV, FLAC and MP3.
# It is used only by the offline calibration utility; the VST3 target is unchanged.
FetchContent_Declare(
    miniaudio
    GIT_REPOSITORY https://github.com/mackron/miniaudio.git
    GIT_TAG 0.11.23
)
FetchContent_MakeAvailable(miniaudio)

add_executable(AnalysatorOfflineAnalyzer
    main.cpp
    ../../src/dsp/AnalysisEngine.cpp
)

target_include_directories(AnalysatorOfflineAnalyzer PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../../src
    ${miniaudio_SOURCE_DIR}
)
