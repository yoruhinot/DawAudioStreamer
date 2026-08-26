add_library(das_audio_core STATIC
  src/Processor.cpp
  src/SampleRateConverter.cpp
)
set_target_properties(das_audio_core PROPERTIES POSITION_INDEPENDENT_CODE ON)

target_include_directories(das_audio_core
  PUBLIC include
  PRIVATE "${r8brain_SOURCE_DIR}"
)
target_compile_features(das_audio_core PUBLIC cxx_std_20)
