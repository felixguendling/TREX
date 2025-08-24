cmake_minimum_required(VERSION 3.10)
project(TREX)

include(cmake/pkg.cmake)

find_package(OpenMP)
if (OPENMP_FOUND)
    set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
    set (CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${OpenMP_EXE_LINKER_FLAGS}")
endif()

add_executable(Network Runnables/Network.cpp)
target_compile_features(Network PRIVATE cxx_std_23)
target_include_directories(Network PRIVATE .)
target_link_libraries(Network PRIVATE TBB::tbb atomic)

add_executable(ULTRA Runnables/ULTRA.cpp)
target_compile_features(ULTRA PRIVATE cxx_std_23)
target_include_directories(ULTRA PRIVATE .)
target_link_libraries(ULTRA PRIVATE TBB::tbb atomic)

add_executable(TP Runnables/TP.cpp)
target_compile_features(TP PRIVATE cxx_std_23)
target_include_directories(TP PRIVATE .)
target_link_libraries(TP PRIVATE TBB::tbb atomic)

add_executable(TREX Runnables/TREX.cpp)
target_compile_features(TREX PRIVATE cxx_std_23)
target_include_directories(TREX PRIVATE .)
target_link_libraries(TREX PRIVATE TBB::tbb atomic)
