juce_add_gui_app(DawAudioStreamerEngine
  PRODUCT_NAME "DAS Engine"
  VERSION "${PROJECT_VERSION}"
  COMPANY_NAME "${DAS_PUBLISHER}"
  COMPANY_WEBSITE "${DAS_REPOSITORY_URL}"
  COMPANY_COPYRIGHT "${DAS_COPYRIGHT}"
  BUNDLE_ID "org.dawaudiostreamer.engine"
)

target_sources(DawAudioStreamerEngine PRIVATE
  Source/Main.cpp
  Source/MainComponent.cpp
  Source/MainComponent.h
)

target_compile_definitions(DawAudioStreamerEngine PRIVATE
  JUCE_APPLICATION_NAME_STRING="$<TARGET_PROPERTY:DawAudioStreamerEngine,JUCE_PRODUCT_NAME>"
  JUCE_APPLICATION_VERSION_STRING="${DAS_DISPLAY_VERSION}"
  JUCE_WEB_BROWSER=0
  JUCE_USE_CURL=0
)

target_link_libraries(DawAudioStreamerEngine PRIVATE
  das_audio_core
  das_transport
  juce::juce_audio_utils
  juce::juce_gui_extra
)

install(TARGETS DawAudioStreamerEngine RUNTIME DESTINATION bin)
