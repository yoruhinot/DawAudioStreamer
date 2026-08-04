cmake_minimum_required(VERSION 3.25)

foreach(required DAS_SOURCE_DIR DAS_BINARY_DIR DAS_VERSION DAS_OUTPUT)
  if(NOT DEFINED ${required} OR "${${required}}" STREQUAL "")
    message(FATAL_ERROR "${required} is required")
  endif()
endforeach()

get_filename_component(source_root "${DAS_SOURCE_DIR}" ABSOLUTE)
get_filename_component(binary_root "${DAS_BINARY_DIR}" ABSOLUTE)
set(package_name "DawAudioStreamer-${DAS_VERSION}-source")
set(staging_root "${binary_root}/source-package")
set(package_root "${staging_root}/${package_name}")

string(FIND "${staging_root}" "${binary_root}/" staging_is_inside)
if(NOT staging_is_inside EQUAL 0)
  message(FATAL_ERROR "Unsafe staging directory: ${staging_root}")
endif()

file(REMOVE_RECURSE "${staging_root}")
file(MAKE_DIRECTORY "${package_root}" "${package_root}/third-party")

foreach(directory apps cmake docs installer libs plugins tests LICENSES)
  file(COPY "${source_root}/${directory}" DESTINATION "${package_root}")
endforeach()

foreach(file
    .gitignore
    CHANGELOG.md
    CMakeLists.txt
    CMakePresets.json
    HANDOFF.md
    LICENSE
    PRIVACY.md
    README.md
    THIRD_PARTY_NOTICES.md)
  file(COPY "${source_root}/${file}" DESTINATION "${package_root}")
endforeach()

foreach(dependency juce-src r8brain-src obs_headers-src)
  set(dependency_source "${binary_root}/_deps/${dependency}")
  if(NOT EXISTS "${dependency_source}")
    message(FATAL_ERROR "Missing fetched source: ${dependency_source}")
  endif()
  file(COPY "${dependency_source}" DESTINATION "${package_root}/third-party"
       PATTERN ".git" EXCLUDE
       PATTERN ".github" EXCLUDE)
endforeach()

file(WRITE "${package_root}/SOURCE_PACKAGE_README.txt"
"DawAudioStreamer ${DAS_VERSION} 対応ソース\n"
"========================================\n\n"
"このアーカイブには、配布バイナリに対応するDawAudioStreamer、JUCE、\n"
"r8brain、OBS Studioのソースが含まれます。third-party内のソースは\n"
"CMakeが自動検出するため、通常のリリース手順でオフライン再ビルドできます。\n\n"
"  cmake --preset windows-msvc-release\n"
"  cmake --build --preset windows-msvc-release\n"
"  ctest --preset windows-msvc-release\n\n"
"各コンポーネントの条件はLICENSE、LICENSES、THIRD_PARTY_NOTICES.mdを\n"
"参照してください。\n")

get_filename_component(output_directory "${DAS_OUTPUT}" DIRECTORY)
file(MAKE_DIRECTORY "${output_directory}")
file(REMOVE "${DAS_OUTPUT}")
execute_process(
  COMMAND "${CMAKE_COMMAND}" -E tar cf "${DAS_OUTPUT}" --format=zip "${package_name}"
  WORKING_DIRECTORY "${staging_root}"
  RESULT_VARIABLE archive_result
)
if(NOT archive_result EQUAL 0 OR NOT EXISTS "${DAS_OUTPUT}")
  message(FATAL_ERROR "Failed to create ${DAS_OUTPUT}")
endif()
message(STATUS "Created ${DAS_OUTPUT}")
