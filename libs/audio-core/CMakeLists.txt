add_library(das_audio_core STATIC
  src/Processor.cpp
  src/SampleRateConverter.cpp
)

target_include_directories(das_audio_core
  PUBLIC include
  PRIVATE "${r8brain_SOURCE_DIR}"
)
target_compile_features(das_audio_core PUBLIC cxx_std_20)
