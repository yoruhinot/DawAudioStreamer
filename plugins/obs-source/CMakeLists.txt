add_library(das_obs_source MODULE Source/obs-plugin.cpp)
target_include_directories(das_obs_source PRIVATE
  "${CMAKE_CURRENT_SOURCE_DIR}/Source"
  "${obs_headers_SOURCE_DIR}/libobs"
)
target_link_libraries(das_obs_source PRIVATE das_transport)
set_target_properties(das_obs_source PROPERTIES
  PREFIX ""
  OUTPUT_NAME "das-obs-source"
)

if(WIN32)
  set(obs_import_library "${CMAKE_CURRENT_BINARY_DIR}/obs.lib")
  add_custom_command(
    OUTPUT "${obs_import_library}"
    COMMAND "${CMAKE_AR}" /nologo /machine:x64
            "/def:${CMAKE_CURRENT_SOURCE_DIR}/obs-import.def"
            "/out:${obs_import_library}"
    DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/obs-import.def"
    VERBATIM
  )
  add_custom_target(das_obs_import_library DEPENDS "${obs_import_library}")
  configure_file(Source/version.rc.in "${CMAKE_CURRENT_BINARY_DIR}/version.rc" @ONLY)
  target_sources(das_obs_source PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/version.rc")
  add_dependencies(das_obs_source das_obs_import_library)
  target_link_libraries(das_obs_source PRIVATE "${obs_import_library}")
  install(TARGETS das_obs_source
          RUNTIME DESTINATION OBS/bin/64bit
          LIBRARY DESTINATION OBS/bin/64bit)
elseif(APPLE)
  set(DAS_OBS_FRAMEWORK_DIR "/Applications/OBS.app/Contents/Frameworks" CACHE PATH
      "Directory containing libobs.framework")
  find_library(DAS_OBS_LIBRARY NAMES libobs
    PATHS "${DAS_OBS_FRAMEWORK_DIR}"
    NO_DEFAULT_PATH
    REQUIRED)
  target_link_libraries(das_obs_source PRIVATE "${DAS_OBS_LIBRARY}")
  target_sources(das_obs_source PRIVATE data/locale/ja-JP.ini)
  set_source_files_properties(data/locale/ja-JP.ini PROPERTIES
    MACOSX_PACKAGE_LOCATION "Resources/locale")
  set_target_properties(das_obs_source PROPERTIES
    BUNDLE TRUE
    BUNDLE_EXTENSION plugin
    MACOSX_BUNDLE_GUI_IDENTIFIER "org.dawaudiostreamer.obs-source"
    MACOSX_BUNDLE_BUNDLE_NAME "DAS Audio"
    MACOSX_BUNDLE_BUNDLE_VERSION "${PROJECT_VERSION}"
    MACOSX_BUNDLE_SHORT_VERSION_STRING "${DAS_DISPLAY_VERSION}"
    XCODE_ATTRIBUTE_GENERATE_INFOPLIST_FILE YES
    XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER "org.dawaudiostreamer.obs-source"
    XCODE_ATTRIBUTE_INFOPLIST_KEY_CFBundleDisplayName "DAS Audio"
    XCODE_ATTRIBUTE_CODE_SIGNING_ALLOWED NO
  )
  install(TARGETS das_obs_source LIBRARY
          DESTINATION "Library/Application Support/obs-studio/plugins")
else()
  message(FATAL_ERROR "DAS OBS source currently supports Windows and macOS")
endif()
