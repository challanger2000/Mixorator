set(ANALYSATOR_SOURCES
    analysis/AssessmentInput.h
    analysis/AssessmentModel.cpp
    analysis/AssessmentModel.h
    dsp/AnalysisEngine.cpp
    dsp/AnalysisEngine.h
    dsp/AnalysisSnapshot.h
    plugin/AnalysisExchange.h
    plugin/Localization.h
    plugin/entry.cpp
    plugin/AnalysatorIDs.h
    plugin/AnalysatorProcessor.cpp
    plugin/AnalysatorProcessor.h
    plugin/AnalysatorController.cpp
    plugin/AnalysatorController.h
    plugin/ControllerState.cpp
    plugin/ControllerStateCodec.cpp
    plugin/ControllerStateCodec.h
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
elseif(APPLE)
    target_sources(Analysator PRIVATE
        ${vst3sdk_SOURCE_DIR}/public.sdk/source/main/macmain.cpp
    )
endif()

smtg_target_add_plugin_resources(Analysator
    RESOURCES
        resource/analysator.uidesc
        resource/analysator_main_bg.png
        resource/analysator_details_bg.png
        resource/125a_gui.png
)

target_compile_definitions(Analysator
    PRIVATE
        $<$<CONFIG:Debug>:VSTGUI_LIVE_EDITING=1>
)

smtg_target_configure_version_file(Analysator)
