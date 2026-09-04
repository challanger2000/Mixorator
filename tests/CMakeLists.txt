add_executable(MixoratorTests
    AnalyzerAssessmentTests.cpp
    ../src/dsp/AnalysisEngine.cpp
    ../src/analysis/AssessmentModel.cpp
)

target_include_directories(MixoratorTests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../src
)

add_test(NAME MixoratorTests COMMAND MixoratorTests)

add_executable(MixoratorLocalStereoTests
    LocalStereoFaultTests.cpp
    ../src/dsp/AnalysisEngine.cpp
)

target_include_directories(MixoratorLocalStereoTests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../src
)

add_test(NAME MixoratorLocalStereoTests COMMAND MixoratorLocalStereoTests)

add_executable(MixoratorAnalyzerIntegrityTests
    AnalyzerIntegrityTests.cpp
    ../src/dsp/AnalysisEngine.cpp
)

target_include_directories(MixoratorAnalyzerIntegrityTests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../src
)

add_test(NAME MixoratorAnalyzerIntegrityTests COMMAND MixoratorAnalyzerIntegrityTests)

add_executable(MixoratorLoudnessMetrologyTests
    LoudnessMetrologyTests.cpp
    ../src/dsp/AnalysisEngine.cpp
)

target_include_directories(MixoratorLoudnessMetrologyTests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../src
)

add_test(NAME MixoratorLoudnessMetrologyTests COMMAND MixoratorLoudnessMetrologyTests)

add_executable(MixoratorTruePeakMetrologyTests
    TruePeakMetrologyTests.cpp
    ../src/dsp/AnalysisEngine.cpp
)

target_include_directories(MixoratorTruePeakMetrologyTests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../src
)

add_test(NAME MixoratorTruePeakMetrologyTests COMMAND MixoratorTruePeakMetrologyTests)

add_executable(MixoratorProcessorTransparencyTests
    ProcessorTransparencyTests.cpp
    ../src/plugin/MixoratorProcessor.cpp
    ../src/dsp/AnalysisEngine.cpp
    ../src/analysis/AssessmentModel.cpp
)

target_include_directories(MixoratorProcessorTransparencyTests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../src
)

target_link_libraries(MixoratorProcessorTransparencyTests PRIVATE sdk)

add_test(NAME MixoratorProcessorTransparencyTests COMMAND MixoratorProcessorTransparencyTests)

add_executable(MixoratorProcessorRobustnessTests
    ProcessorRobustnessTests.cpp
    ../src/plugin/MixoratorProcessor.cpp
    ../src/dsp/AnalysisEngine.cpp
    ../src/analysis/AssessmentModel.cpp
)

target_include_directories(MixoratorProcessorRobustnessTests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../src
)

target_link_libraries(MixoratorProcessorRobustnessTests PRIVATE sdk)

add_test(NAME MixoratorProcessorRobustnessTests COMMAND MixoratorProcessorRobustnessTests)

add_executable(MixoratorMasterScenarioTests
    MasterScenarioTests.cpp
    ../src/analysis/AssessmentModel.cpp
)

target_include_directories(MixoratorMasterScenarioTests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../src
)

add_test(NAME MixoratorMasterScenarioTests COMMAND MixoratorMasterScenarioTests)
