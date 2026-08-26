add_library(das_transport STATIC
  src/RingBuffer.cpp
)
set_target_properties(das_transport PROPERTIES POSITION_INDEPENDENT_CODE ON)

if(WIN32)
  target_sources(das_transport PRIVATE src/NamedSharedMemory_win.cpp)
elseif(APPLE)
  target_sources(das_transport PRIVATE src/NamedSharedMemory_posix.cpp)
else()
  message(FATAL_ERROR "das_transport currently supports Windows and macOS")
endif()

target_include_directories(das_transport PUBLIC include)
target_compile_features(das_transport PUBLIC cxx_std_20)
