set(MIXORATOR_SOURCES
    plugin/entry.cpp
    plugin/MixoratorProcessor.cpp
    plugin/MixoratorProcessor.h
)

smtg_add_vst3plugin(Mixorator ${MIXORATOR_SOURCES})

target_link_libraries(Mixorator
    PRIVATE
        sdk
)

smtg_target_configure_version_file(Mixorator)
