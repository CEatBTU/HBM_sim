cmake_minimum_required(VERSION 3.10)
# Project Name:
project(MAOSys)
# Configuration:
set(CMAKE_CXX_STANDARD 17 CACHE STRING "C++ version")
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# --------------------------------------------- START DRAMSYS ------------------------
# Add DRAMSysLibrary:
# to enable pthreads for systemc, other threads don't go well with valgrind
option (ENABLE_PTHREADS "Use POSIX threads for SystemC processes instead of 
QuickThreads on Unix or Fiber on Windows." ON)
add_subdirectory(extern/dramSys/DRAMSys/library/)
# --------------------------------------------- START MAOSYS -------------------------
file(GLOB_RECURSE SRC_FILES src/*.cpp)
set(SRC_FILES ${SRC_FILES} ${CMAKE_CURRENT_SOURCE_DIR}/extern/dramSys/DRAMSys/simulator/LengthConverter.cpp)

include_directories(PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
include_directories(SYSTEM ${CMAKE_CURRENT_SOURCE_DIR}/extern/dramSys/DRAMSys/library/src/ ${CMAKE_CURRENT_SOURCE_DIR}/extern/dramSys/DRAMSys/simulator 
    ${CMAKE_CURRENT_SOURCE_DIR}/extern/dramSys/DRAMSys/library/src/common/third_party/nlohmann/single_include/nlohmann/)

list(APPEND MY_COMPILE_FLAGS "-g" "-Wall" "-Wshadow" "-Wextra" "-O0" "-pedantic" "-Wconversion" "-Wsign-conversion")
# --------------------------------------------- START BENCHMARK --------------------------
add_executable(benchmark benchmarks/benchmark.cpp ${SRC_FILES} ${CMAKE_CURRENT_SOURCE_DIR}/extern/dramSys/DRAMSys/simulator/LengthConverter.cpp)
target_compile_options(benchmark PUBLIC ${MY_COMPILE_FLAGS})
target_link_libraries(benchmark PRIVATE DRAMSysLibrary)
# --------------------------------------------- START 8x8 BUTTERFLY BENCHMARK --------------------------
add_executable(butterfly8x8 benchmarks/butterfly8x8Benchmark.cpp ${SRC_FILES} ${CMAKE_CURRENT_SOURCE_DIR}/extern/dramSys/DRAMSys/simulator/LengthConverter.cpp)
target_compile_options(butterfly8x8 PUBLIC ${MY_COMPILE_FLAGS})
target_link_libraries(butterfly8x8 PRIVATE DRAMSysLibrary)
if(FALSE) # fake a block comment
# --------------------------------------------- START 2x2 BUTTERFLY BENCHMARK --------------------------
add_executable(butterfly2x2 benchmarks/butterfly2x2Benchmark.cpp ${SRC_FILES} ${CMAKE_CURRENT_SOURCE_DIR}/extern/dramSys/DRAMSys/simulator/LengthConverter.cpp)
target_compile_options(butterfly2x2 PUBLIC ${MY_COMPILE_FLAGS})
target_link_libraries(butterfly2x2 PRIVATE DRAMSysLibrary)
endif()
