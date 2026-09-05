set(ANALYSATOR_SOURCES
    analysis/AssessmentInput.h
    analysis/AssessmentModel.cpp
    analysis/AssessmentModel.h
    dsp/AnalysisEngine.cpp
    dsp/AnalysisEngine.h
    dsp/AnalysisSnapshot.h
    plugin/AnalysisExchange.h
    plugin/entry.cpp
    plugin/MixoratorIDs.h
    plugin/MixoratorProcessor.cpp
    plugin/MixoratorProcessor.h
    plugin/MixoratorController.cpp
    plugin/MixoratorController.h
)

smtg_add_vst3plugin(Analysator ${ANALYSATOR_SOURCES})

target_link_libraries(Analysator
    PRIVATE
        sdk
        vstgui_support
)

if(WIN32)
    target_sources(Analysator PRIVATE
        ${vst3sdk_SOURCE_DIR}/public.sdk/source/main/dllmain.cpp
    )
endif()

smtg_target_add_plugin_resources(Analysator
    RESOURCES
        resource/mixorator.uidesc
        resource/analysator_main_bg.png
)

target_compile_definitions(Analysator
    PRIVATE
        $<$<CONFIG:Debug>:VSTGUI_LIVE_EDITING=1>
)

smtg_target_configure_version_file(Analysator)
