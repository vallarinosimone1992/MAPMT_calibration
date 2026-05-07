cmake_minimum_required(VERSION 3.18)

project(MAPMT_calibration VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(ROOT REQUIRED COMPONENTS Core Hist Tree RIO)

add_library(mapmt_calibration_lib
  src/DetectorConfig.cpp
  src/HardwareMap.cpp
  src/ScalerPedestalAnalyzer.cpp
  src/TimeCalibration.cpp
)

target_include_directories(mapmt_calibration_lib
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(mapmt_calibration_lib
  PUBLIC
    ROOT::Core
    ROOT::Hist
    ROOT::Tree
    ROOT::RIO
)

target_compile_options(mapmt_calibration_lib
  PRIVATE
    $<$<CXX_COMPILER_ID:AppleClang,Clang,GNU>:-Wall -Wextra -Wpedantic>
)

add_executable(mapmt_calibrate src/main.cpp)
target_link_libraries(mapmt_calibrate PRIVATE mapmt_calibration_lib)
target_compile_options(mapmt_calibrate
  PRIVATE
    $<$<CXX_COMPILER_ID:AppleClang,Clang,GNU>:-Wall -Wextra -Wpedantic>
)

install(TARGETS mapmt_calibrate RUNTIME DESTINATION bin)
