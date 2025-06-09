cmake_minimum_required(VERSION 3.10)

project(sssp LANGUAGES CXX)

find_package(MPI REQUIRED)

set(SOURCES
    src/main.cpp src/Node_comm.cpp
    src/Node_base.cpp src/Node_baseline.cpp
    src/Node_optimise.cpp src/Node_pull.cpp
)

add_executable(${PROJECT_NAME} ${SOURCES})

target_link_libraries(${PROJECT_NAME} MPI::MPI_CXX)

target_compile_options(${PROJECT_NAME} PUBLIC -std=c++17)
if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    if (CMAKE_CXX_COMPILER_VERSION VERSION_LESS 9)
        target_link_libraries(sssp stdc++fs)
    endif()
endif()
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -march=native -funroll-loops -fomit-frame-pointer")
