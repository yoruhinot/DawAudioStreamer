if(NOT WIN32)
  message(FATAL_ERROR "DAS OBS source currently supports Windows only")
endif()

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

add_library(das_obs_source MODULE Source/obs-plugin.cpp)
configure_file(Source/version.rc.in "${CMAKE_CURRENT_BINARY_DIR}/version.rc" @ONLY)
target_sources(das_obs_source PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/version.rc")
add_dependencies(das_obs_source das_obs_import_library)
target_include_directories(das_obs_source PRIVATE
  "${CMAKE_CURRENT_SOURCE_DIR}/Source"
  "${obs_headers_SOURCE_DIR}/libobs"
)
target_link_libraries(das_obs_source PRIVATE das_transport "${obs_import_library}")
set_target_properties(das_obs_source PROPERTIES PREFIX "" OUTPUT_NAME "das-obs-source")

install(TARGETS das_obs_source RUNTIME DESTINATION OBS/bin/64bit LIBRARY DESTINATION OBS/bin/64bit)
