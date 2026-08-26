cmake_minimum_required(VERSION 3.25)

foreach(required DAS_SOURCE_DIR DAS_BINARY_DIR DAS_INSTALL_ROOT DAS_OUTPUT_DIR
                 DAS_VERSION DAS_COMMIT)
  if(NOT DEFINED ${required} OR "${${required}}" STREQUAL "")
    message(FATAL_ERROR "${required} is required")
  endif()
endforeach()

get_filename_component(source_root "${DAS_SOURCE_DIR}" ABSOLUTE)
get_filename_component(binary_root "${DAS_BINARY_DIR}" ABSOLUTE)
get_filename_component(install_root "${DAS_INSTALL_ROOT}" ABSOLUTE)
get_filename_component(output_root "${DAS_OUTPUT_DIR}" ABSOLUTE)
set(package_name "DawAudioStreamer-${DAS_VERSION}-macOS-AppleSilicon")
set(package_root "${output_root}/${package_name}")
set(payload_root "${package_root}/payload")

string(FIND "${package_root}" "${output_root}/" package_is_inside)
if(NOT package_is_inside EQUAL 0)
  message(FATAL_ERROR "Unsafe package directory: ${package_root}")
endif()

set(vst3 "${install_root}/Library/Audio/Plug-Ins/VST3/DAS Send.vst3")
set(au "${install_root}/Library/Audio/Plug-Ins/Components/DAS Send.component")
set(obs "${install_root}/Library/Application Support/obs-studio/plugins/das-obs-source.plugin")
foreach(bundle "${vst3}" "${au}" "${obs}")
  if(NOT EXISTS "${bundle}")
    message(FATAL_ERROR "Missing macOS bundle: ${bundle}")
  endif()
endforeach()

file(REMOVE_RECURSE "${package_root}")
file(MAKE_DIRECTORY "${payload_root}" "${package_root}/licenses")
file(COPY "${vst3}" "${au}" "${obs}" DESTINATION "${payload_root}")
file(COPY
  "${source_root}/installer/macos/Install.command"
  "${source_root}/installer/macos/Uninstall.command"
  "${source_root}/installer/macos/はじめに.txt"
  DESTINATION "${package_root}")
file(CHMOD
  "${package_root}/Install.command"
  "${package_root}/Uninstall.command"
  PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE
              WORLD_READ WORLD_EXECUTE)
file(CHMOD
  "${payload_root}/DAS Send.vst3/Contents/MacOS/DAS Send"
  "${payload_root}/DAS Send.component/Contents/MacOS/DAS Send"
  "${payload_root}/das-obs-source.plugin/Contents/MacOS/das-obs-source"
  PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE
              WORLD_READ WORLD_EXECUTE)

file(COPY "${source_root}/LICENSE" "${source_root}/THIRD_PARTY_NOTICES.md"
     DESTINATION "${package_root}/licenses")
file(COPY "${source_root}/LICENSES/" DESTINATION "${package_root}/licenses/LICENSES")
configure_file("${source_root}/libs/transport/LICENSE"
               "${package_root}/licenses/MIT-transport.txt" COPYONLY)
if(EXISTS "${binary_root}/_deps/juce-src/LICENSE.md")
  file(COPY "${binary_root}/_deps/juce-src/LICENSE.md"
       DESTINATION "${package_root}/licenses")
endif()
if(EXISTS "${binary_root}/_deps/juce-src/modules/juce_audio_processors/format_types/VST3_SDK/LICENSE.txt")
  file(COPY
    "${binary_root}/_deps/juce-src/modules/juce_audio_processors/format_types/VST3_SDK/LICENSE.txt"
    DESTINATION "${package_root}/licenses/VST3-SDK")
endif()
if(EXISTS "${binary_root}/_deps/obs_headers-src/COPYING")
  file(COPY "${binary_root}/_deps/obs_headers-src/COPYING"
       DESTINATION "${package_root}/licenses/OBS")
endif()
if(EXISTS "${binary_root}/_deps/simde-src/COPYING")
  file(COPY "${binary_root}/_deps/simde-src/COPYING"
       DESTINATION "${package_root}/licenses/SIMDe")
endif()

file(WRITE "${package_root}/SOURCE.txt"
"この検証版に対応するソース：\n"
"https://github.com/yoruhinot/DawAudioStreamer/tree/${DAS_COMMIT}\n\n"
"ライセンス条件はlicenses内を参照してください。\n")

foreach(bundle
    "${payload_root}/DAS Send.vst3"
    "${payload_root}/DAS Send.component"
    "${payload_root}/das-obs-source.plugin")
  execute_process(COMMAND /usr/bin/codesign --force --deep --sign - "${bundle}"
                  RESULT_VARIABLE sign_result)
  if(NOT sign_result EQUAL 0)
    message(FATAL_ERROR "Ad-hoc signing failed: ${bundle}")
  endif()
  execute_process(COMMAND /usr/bin/codesign --verify --deep --strict "${bundle}"
                  RESULT_VARIABLE verify_result)
  if(NOT verify_result EQUAL 0)
    message(FATAL_ERROR "Code signature verification failed: ${bundle}")
  endif()
endforeach()

file(MAKE_DIRECTORY "${output_root}")
set(archive "${output_root}/${package_name}.zip")
file(REMOVE "${archive}")
execute_process(
  COMMAND /usr/bin/ditto -c -k --sequesterRsrc --keepParent
          "${package_root}" "${archive}"
  RESULT_VARIABLE archive_result)
if(NOT archive_result EQUAL 0 OR NOT EXISTS "${archive}")
  message(FATAL_ERROR "Failed to create ${archive}")
endif()
message(STATUS "Created ${archive}")
