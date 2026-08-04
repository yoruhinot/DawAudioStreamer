add_library(das_transport STATIC
  src/NamedSharedMemory_win.cpp
  src/RingBuffer.cpp
)

target_include_directories(das_transport PUBLIC include)
target_compile_features(das_transport PUBLIC cxx_std_20)
