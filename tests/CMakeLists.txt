add_executable(MixoratorTests
    AnalyzerAssessmentTests.cpp
    ../src/dsp/AnalysisEngine.cpp
    ../src/analysis/AssessmentModel.cpp
)

target_include_directories(MixoratorTests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../src
)

add_test(NAME MixoratorTests COMMAND MixoratorTests)
