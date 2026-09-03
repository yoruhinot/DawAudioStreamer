find_package(Threads REQUIRED)

add_executable(das_audio_core_tests audio_core_tests.cpp)
target_link_libraries(das_audio_core_tests PRIVATE das_audio_core)
add_test(NAME audio_core COMMAND das_audio_core_tests)

add_executable(das_transport_tests transport_tests.cpp)
target_link_libraries(das_transport_tests PRIVATE das_transport Threads::Threads)
add_test(NAME transport COMMAND das_transport_tests)

if(WIN32)
  add_executable(das_transport_ipc_tests transport_ipc_tests.cpp)
  target_link_libraries(das_transport_ipc_tests PRIVATE das_transport)
  add_test(NAME transport_ipc COMMAND das_transport_ipc_tests)
  set_tests_properties(transport_ipc PROPERTIES TIMEOUT 20)
elseif(APPLE)
  add_executable(das_transport_ipc_tests transport_ipc_tests_posix.cpp)
  target_link_libraries(das_transport_ipc_tests PRIVATE das_transport)
  add_test(NAME transport_ipc COMMAND das_transport_ipc_tests)
  set_tests_properties(transport_ipc PROPERTIES TIMEOUT 20)
endif()

if(TARGET DasSend_VST3)
  add_executable(das_discord_bridge_tests discord_bridge_tests.cpp)
  target_link_libraries(das_discord_bridge_tests PRIVATE das_discord_bridge)
  add_test(NAME discord_bridge COMMAND das_discord_bridge_tests)

  juce_add_gui_app(das_discord_bridge_host PRODUCT_NAME "DAS Discord Bridge Test")
  target_sources(das_discord_bridge_host PRIVATE discord_bridge_host.cpp)
  target_compile_definitions(das_discord_bridge_host PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
  )
  target_link_libraries(das_discord_bridge_host PRIVATE
    das_discord_bridge
    juce::juce_gui_extra
  )

  juce_add_console_app(das_vst3_smoke PRODUCT_NAME "DAS VST3 Smoke Test")
  target_sources(das_vst3_smoke PRIVATE vst3_smoke.cpp)
  target_compile_definitions(das_vst3_smoke PRIVATE
    JUCE_PLUGINHOST_VST3=1
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
  )
  target_link_libraries(das_vst3_smoke PRIVATE
    das_transport
    juce::juce_audio_processors
    juce::juce_audio_utils
  )
  if(WIN32)
    target_link_libraries(das_vst3_smoke PRIVATE ole32 uuid)
  endif()
  add_dependencies(das_vst3_smoke DasSend_VST3)
  add_test(NAME vst3_smoke COMMAND das_vst3_smoke
    "$<TARGET_FILE_DIR:DasSend_VST3>/../..")
  set_tests_properties(vst3_smoke PROPERTIES TIMEOUT 30)

  juce_add_gui_app(das_vst3_editor_host PRODUCT_NAME "DAS Send UI Test")
  target_sources(das_vst3_editor_host PRIVATE vst3_editor_host.cpp)
  target_compile_definitions(das_vst3_editor_host PRIVATE
    JUCE_PLUGINHOST_VST3=1
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
  )
  target_link_libraries(das_vst3_editor_host PRIVATE
    juce::juce_audio_processors
    juce::juce_gui_extra
  )
  add_dependencies(das_vst3_editor_host DasSend_VST3)
endif()
