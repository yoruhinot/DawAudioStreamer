cmake_minimum_required(VERSION 3.25)

project(DawAudioStreamer VERSION 0.4.0 LANGUAGES CXX)

set(DAS_DISPLAY_VERSION "0.4.0-beta.3" CACHE STRING
    "Public version shown to users and used in release artifact names")
set(DAS_PUBLISHER "yoruhinot" CACHE STRING
    "Publisher shown in Windows file metadata")
set(DAS_COPYRIGHT "Copyright (c) 2026 yoruhinot" CACHE STRING
    "Copyright shown in Windows file metadata")
set(DAS_REPOSITORY_URL "https://github.com/yoruhinot/DawAudioStreamer" CACHE STRING
    "Public source repository")

option(DAS_BUILD_TESTS "単体テストとストレステストをビルドする" ON)
option(DAS_ENABLE_JUCE "JUCEを取得し、JUCE依存ターゲットを有効にする" OFF)
option(DAS_BUILD_OBS_PLUGIN "OBS音声ソースプラグインをビルドする" OFF)
if(WIN32)
  option(DAS_BUILD_ENGINE "開発者向けDAS Engineをビルドする" ON)
else()
  option(DAS_BUILD_ENGINE "開発者向けDAS Engineをビルドする" OFF)
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
if(MSVC)
  set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

# The release source archive carries the exact third-party revisions in this
# directory. Prefer those copies so recipients can rebuild without a network.
set(DAS_BUNDLED_THIRD_PARTY_DIR "${CMAKE_CURRENT_SOURCE_DIR}/third-party")
if(EXISTS "${DAS_BUNDLED_THIRD_PARTY_DIR}/juce-src/CMakeLists.txt")
  set(FETCHCONTENT_SOURCE_DIR_JUCE "${DAS_BUNDLED_THIRD_PARTY_DIR}/juce-src"
      CACHE PATH "Bundled JUCE source" FORCE)
endif()
if(EXISTS "${DAS_BUNDLED_THIRD_PARTY_DIR}/r8brain-src/CDSPResampler.h")
  set(FETCHCONTENT_SOURCE_DIR_R8BRAIN "${DAS_BUNDLED_THIRD_PARTY_DIR}/r8brain-src"
      CACHE PATH "Bundled r8brain source" FORCE)
endif()
if(EXISTS "${DAS_BUNDLED_THIRD_PARTY_DIR}/obs_headers-src/libobs/obs.h")
  set(FETCHCONTENT_SOURCE_DIR_OBS_HEADERS "${DAS_BUNDLED_THIRD_PARTY_DIR}/obs_headers-src"
      CACHE PATH "Bundled OBS source" FORCE)
endif()
if(EXISTS "${DAS_BUNDLED_THIRD_PARTY_DIR}/simde-src/simde/x86/sse2.h")
  set(FETCHCONTENT_SOURCE_DIR_SIMDE "${DAS_BUNDLED_THIRD_PARTY_DIR}/simde-src"
      CACHE PATH "Bundled SIMDe source" FORCE)
endif()

if(MSVC)
  add_compile_options(/W4 /permissive- /Zc:__cplusplus /utf-8)
else()
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

if(DAS_ENABLE_JUCE)
  include(FetchContent)
  FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 3af3ce009f6a02f6fa651008fffb5b41743a9fab
    GIT_SHALLOW TRUE
    EXCLUDE_FROM_ALL
  )
  FetchContent_MakeAvailable(JUCE)
endif()

include(FetchContent)
FetchContent_Declare(
  r8brain
  GIT_REPOSITORY https://github.com/avaneev/r8brain-free-src.git
  GIT_TAG 8fff6f3db26f14a8f5e8fb871000613673db5753
  GIT_SHALLOW TRUE
  EXCLUDE_FROM_ALL
)
FetchContent_MakeAvailable(r8brain)

if(DAS_BUILD_OBS_PLUGIN)
  include(FetchContent)
  FetchContent_Declare(
    obs_headers
    GIT_REPOSITORY https://github.com/obsproject/obs-studio.git
    GIT_TAG c17423ce05899ecb93f678601b3feaa8a469b180
    GIT_SHALLOW TRUE
    SOURCE_SUBDIR headers-only-no-cmake-project
    EXCLUDE_FROM_ALL
  )
  FetchContent_MakeAvailable(obs_headers)

  if(APPLE)
    FetchContent_Declare(
      simde
      GIT_REPOSITORY https://github.com/simd-everywhere/simde.git
      GIT_TAG 71fd833d9666141edcd1d3c109a80e228303d8d7
      GIT_SHALLOW TRUE
      SOURCE_SUBDIR headers-only-no-cmake-project
      EXCLUDE_FROM_ALL
    )
    FetchContent_MakeAvailable(simde)
  endif()
endif()

add_subdirectory(libs/audio-core)
add_subdirectory(libs/transport)

if(DAS_ENABLE_JUCE)
  if(DAS_BUILD_ENGINE)
    add_subdirectory(apps/engine)
  endif()
  add_subdirectory(plugins/send-vst3)
endif()

if(DAS_BUILD_OBS_PLUGIN)
  add_subdirectory(plugins/obs-source)
endif()

if(DAS_BUILD_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()

if(WIN32 AND DAS_ENABLE_JUCE AND DAS_BUILD_OBS_PLUGIN)
  set(DAS_SOURCE_ARCHIVE
      "${CMAKE_SOURCE_DIR}/build/source/DawAudioStreamer-${DAS_DISPLAY_VERSION}-source.zip")
  add_custom_target(das_source_package
    COMMAND "${CMAKE_COMMAND}"
      -DDAS_SOURCE_DIR=${CMAKE_SOURCE_DIR}
      -DDAS_BINARY_DIR=${CMAKE_BINARY_DIR}
      -DDAS_VERSION=${DAS_DISPLAY_VERSION}
      -DDAS_OUTPUT=${DAS_SOURCE_ARCHIVE}
      -P "${CMAKE_SOURCE_DIR}/cmake/CreateSourcePackage.cmake"
    BYPRODUCTS "${DAS_SOURCE_ARCHIVE}"
    COMMENT "対応ソースアーカイブを作成しています"
    VERBATIM
  )
endif()

if(WIN32 AND DAS_ENABLE_JUCE AND DAS_BUILD_OBS_PLUGIN)
  find_program(DAS_ISCC_EXECUTABLE ISCC.exe
    HINTS "$ENV{LOCALAPPDATA}/Programs/Inno Setup 6")
  if(DAS_ISCC_EXECUTABLE)
    add_custom_target(das_installer
      COMMAND "${DAS_ISCC_EXECUTABLE}" "${CMAKE_SOURCE_DIR}/installer/DawAudioStreamer.iss"
      WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
      DEPENDS DasSend_VST3 das_obs_source das_virtual_audio_check das_source_package
      COMMENT "DawAudioStreamerセットアップを生成しています"
      VERBATIM
    )
  endif()
endif()
