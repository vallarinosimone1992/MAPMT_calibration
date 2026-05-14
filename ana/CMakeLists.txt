cmake_minimum_required(VERSION 3.18)

project(MAPMT_calibration VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(ROOT REQUIRED COMPONENTS Core Hist Tree RIO)

option(MAPMT_STRIP_ROOT_CXX_STANDARD
  "Remove -std=... flags injected by ROOT; MAPMT_calibration controls the C++ standard"
  ON
)

function(mapmt_strip_std_flag_from_variable variable_name)
  if(DEFINED ${variable_name})
    string(REGEX REPLACE "(^|[ \t])-std=(c|gnu)\\+\\+[^ \t]+" " "
      filtered_value "${${variable_name}}"
    )
    string(STRIP "${filtered_value}" filtered_value)
    set(${variable_name} "${filtered_value}" PARENT_SCOPE)
  endif()
endfunction()

function(mapmt_strip_std_flag_from_target target_name)
  if(NOT TARGET "${target_name}")
    return()
  endif()

  get_target_property(interface_options "${target_name}" INTERFACE_COMPILE_OPTIONS)
  if(NOT interface_options OR interface_options STREQUAL "interface_options-NOTFOUND")
    return()
  endif()

  set(filtered_options)
  foreach(option IN LISTS interface_options)
    if(NOT option MATCHES "std=(c|gnu)\\+\\+")
      list(APPEND filtered_options "${option}")
    endif()
  endforeach()
  set_target_properties("${target_name}" PROPERTIES
    INTERFACE_COMPILE_OPTIONS "${filtered_options}"
  )
endfunction()

if(MAPMT_STRIP_ROOT_CXX_STANDARD)
  foreach(flag_variable
      CMAKE_CXX_FLAGS
      CMAKE_CXX_FLAGS_DEBUG
      CMAKE_CXX_FLAGS_RELEASE
      CMAKE_CXX_FLAGS_RELWITHDEBINFO
      CMAKE_CXX_FLAGS_MINSIZEREL)
    mapmt_strip_std_flag_from_variable("${flag_variable}")
  endforeach()

  foreach(root_target ROOT::Core ROOT::Hist ROOT::Tree ROOT::RIO)
    mapmt_strip_std_flag_from_target("${root_target}")
  endforeach()
endif()

message(STATUS "Using ROOT ${ROOT_VERSION}")
message(STATUS "Using C++ standard ${CMAKE_CXX_STANDARD}")

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

target_compile_features(mapmt_calibration_lib PUBLIC cxx_std_17)

target_link_libraries(mapmt_calibration_lib
  PUBLIC
    ROOT::Core
    ROOT::Hist
    ROOT::Tree
    ROOT::RIO
)

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND
   CMAKE_CXX_COMPILER_VERSION VERSION_LESS 9)
  target_link_libraries(mapmt_calibration_lib PUBLIC stdc++fs)
endif()

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

add_executable(mapmt_bin2root src/mapmt_bin2root.cpp)
target_link_libraries(mapmt_bin2root PRIVATE mapmt_calibration_lib)
target_compile_options(mapmt_bin2root
  PRIVATE
    $<$<CXX_COMPILER_ID:AppleClang,Clang,GNU>:-Wall -Wextra -Wpedantic>
)

install(TARGETS mapmt_calibrate mapmt_bin2root RUNTIME DESTINATION bin)
