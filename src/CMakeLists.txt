set(MIXORATOR_SOURCES
    plugin/entry.cpp
    plugin/MixoratorIDs.h
    plugin/MixoratorProcessor.cpp
    plugin/MixoratorProcessor.h
    plugin/MixoratorController.cpp
    plugin/MixoratorController.h
)

smtg_add_vst3plugin(Mixorator ${MIXORATOR_SOURCES})

target_link_libraries(Mixorator
    PRIVATE
        sdk
)

smtg_target_configure_version_file(Mixorator)
