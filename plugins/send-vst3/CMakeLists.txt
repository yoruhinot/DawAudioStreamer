add_library(das_discord_bridge STATIC
  Source/DiscordBridge.cpp
  Source/DiscordBridge.h
  Source/VirtualAudioEndpoint.cpp
  Source/VirtualAudioEndpoint.h
)
target_include_directories(das_discord_bridge PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/Source")
target_link_libraries(das_discord_bridge PUBLIC das_transport)
if(WIN32)
  target_link_libraries(das_discord_bridge PRIVATE ole32 uuid)
endif()

if(WIN32)
  configure_file(Source/VirtualAudioCheckVersion.rc.in
                 "${CMAKE_CURRENT_BINARY_DIR}/VirtualAudioCheckVersion.rc" @ONLY)
  add_executable(das_virtual_audio_check
    Source/VirtualAudioCheckMain.cpp
    "${CMAKE_CURRENT_BINARY_DIR}/VirtualAudioCheckVersion.rc"
  )
  target_link_libraries(das_virtual_audio_check PRIVATE das_discord_bridge ole32 uuid)
  set_target_properties(das_virtual_audio_check PROPERTIES
    OUTPUT_NAME "das-virtual-audio-check"
  )
endif()

juce_add_plugin(DasSend
  PRODUCT_NAME "DAS Send"
  VERSION "${PROJECT_VERSION}"
  COMPANY_NAME "${DAS_PUBLISHER}"
  COMPANY_WEBSITE "${DAS_REPOSITORY_URL}"
  COMPANY_COPYRIGHT "${DAS_COPYRIGHT}"
  BUNDLE_ID "org.dawaudiostreamer.send"
  PLUGIN_MANUFACTURER_CODE Daud
  PLUGIN_CODE Ds01
  FORMATS VST3
  VST3_CATEGORIES Fx
  VST3_AUTO_MANIFEST FALSE
  IS_SYNTH FALSE
  NEEDS_MIDI_INPUT FALSE
  NEEDS_MIDI_OUTPUT FALSE
  IS_MIDI_EFFECT FALSE
  EDITOR_WANTS_KEYBOARD_FOCUS FALSE
  COPY_PLUGIN_AFTER_BUILD FALSE
)

target_sources(DasSend PRIVATE
  Source/PluginEditor.cpp
  Source/PluginEditor.h
  Source/PluginProcessor.cpp
  Source/PluginProcessor.h
)
target_compile_definitions(DasSend PUBLIC
  JUCE_WEB_BROWSER=0
  JUCE_USE_CURL=0
  JUCE_IGNORE_VST3_MISMATCHED_PARAMETER_ID_WARNING=1
)
target_link_libraries(DasSend PRIVATE
  das_audio_core
  das_discord_bridge
  das_transport
  juce::juce_audio_utils
  juce::juce_audio_processors
)
if(WIN32)
  target_link_libraries(DasSend PRIVATE shell32 advapi32)
endif()

install(DIRECTORY "$<TARGET_FILE_DIR:DasSend_VST3>/DAS Send.vst3" DESTINATION VST3)
