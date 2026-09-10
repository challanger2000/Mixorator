add_executable(AnalysatorTests
    AnalyzerAssessmentTests.cpp
    ../src/dsp/AnalysisEngine.cpp
    ../src/analysis/AssessmentModel.cpp
)

target_include_directories(AnalysatorTests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../src
)

add_test(NAME AnalysatorTests COMMAND AnalysatorTests)

add_executable(AnalysatorLocalStereoTests
    LocalStereoFaultTests.cpp
    ../src/dsp/AnalysisEngine.cpp
)

target_include_directories(AnalysatorLocalStereoTests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../src
)

add_test(NAME AnalysatorLocalStereoTests COMMAND AnalysatorLocalStereoTests)

add_executable(AnalysatorAnalyzerIntegrityTests
    AnalyzerIntegrityTests.cpp
    ../src/dsp/AnalysisEngine.cpp
)

target_include_directories(AnalysatorAnalyzerIntegrityTests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../src
)

add_test(NAME AnalysatorAnalyzerIntegrityTests COMMAND AnalysatorAnalyzerIntegrityTests)

add_executable(AnalysatorLoudnessMetrologyTests
    LoudnessMetrologyTests.cpp
    ../src/dsp/AnalysisEngine.cpp
)

target_include_directories(AnalysatorLoudnessMetrologyTests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../src
)

add_test(NAME AnalysatorLoudnessMetrologyTests COMMAND AnalysatorLoudnessMetrologyTests)

add_executable(AnalysatorTruePeakMetrologyTests
    TruePeakMetrologyTests.cpp
    ../src/dsp/AnalysisEngine.cpp
)

target_include_directories(AnalysatorTruePeakMetrologyTests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../src
)

add_test(NAME AnalysatorTruePeakMetrologyTests COMMAND AnalysatorTruePeakMetrologyTests)

add_executable(AnalysatorProcessorTransparencyTests
    ProcessorTransparencyTests.cpp
    ../src/plugin/AnalysatorProcessor.cpp
    ../src/dsp/AnalysisEngine.cpp
    ../src/analysis/AssessmentModel.cpp
)

target_include_directories(AnalysatorProcessorTransparencyTests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../src
)

target_link_libraries(AnalysatorProcessorTransparencyTests PRIVATE sdk)

add_test(NAME AnalysatorProcessorTransparencyTests COMMAND AnalysatorProcessorTransparencyTests)

add_executable(AnalysatorProcessorRobustnessTests
    ProcessorRobustnessTests.cpp
    ../src/plugin/AnalysatorProcessor.cpp
    ../src/dsp/AnalysisEngine.cpp
    ../src/analysis/AssessmentModel.cpp
)

target_include_directories(AnalysatorProcessorRobustnessTests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../src
)

target_link_libraries(AnalysatorProcessorRobustnessTests PRIVATE sdk)

add_test(NAME AnalysatorProcessorRobustnessTests COMMAND AnalysatorProcessorRobustnessTests)

add_executable(AnalysatorMasterScenarioTests
    MasterScenarioTests.cpp
    ../src/analysis/AssessmentModel.cpp
)

target_include_directories(AnalysatorMasterScenarioTests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../src
)

add_test(NAME AnalysatorMasterScenarioTests COMMAND AnalysatorMasterScenarioTests)

add_executable(AnalysatorRealWorldReferenceTests
    RealWorldReferenceTests.cpp
    ../src/analysis/AssessmentModel.cpp
)

target_include_directories(AnalysatorRealWorldReferenceTests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../src
)

add_test(NAME AnalysatorRealWorldReferenceTests COMMAND AnalysatorRealWorldReferenceTests)

add_executable(AnalysatorDetailedTonalCalibrationTests
    DetailedTonalCalibrationTests.cpp
    ../src/analysis/AssessmentModel.cpp
)

target_include_directories(AnalysatorDetailedTonalCalibrationTests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../src
)

add_test(NAME AnalysatorDetailedTonalCalibrationTests COMMAND AnalysatorDetailedTonalCalibrationTests)

add_executable(AnalysatorAssessmentDiagnosisTests
    AssessmentDiagnosisTests.cpp
)

target_include_directories(AnalysatorAssessmentDiagnosisTests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../src
)

add_test(NAME AnalysatorAssessmentDiagnosisTests COMMAND AnalysatorAssessmentDiagnosisTests)

add_executable(AnalysatorControllerStateCodecTests
    ControllerStateCodecTests.cpp
    ../src/plugin/ControllerStateCodec.cpp
    ${vst3sdk_SOURCE_DIR}/public.sdk/source/common/memorystream.cpp
)

target_include_directories(AnalysatorControllerStateCodecTests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../src
)

target_link_libraries(AnalysatorControllerStateCodecTests PRIVATE base pluginterfaces)

add_test(NAME AnalysatorControllerStateCodecTests COMMAND AnalysatorControllerStateCodecTests)
