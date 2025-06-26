cmake_minimum_required(VERSION 3.28)  # At least 3.12 for C++20 support and good target handling

project(sssp LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(MPI REQUIRED)

add_executable(${PROJECT_NAME} ${SOURCES})

target_sources(${PROJECT_NAME}
    PRIVATE
        src/main.cpp
)

target_sources(${PROJECT_NAME}
    PRIVATE
    FILE_SET CXX_MODULES
    TYPE CXX_MODULES
    BASE_DIRS src
    FILES src/Definitions.cppm
        src/Lookup.cppm
        src/Node.cppm
        src/Node.interface.cppm
        src/Node.comm.cppm
        src/Node.base.cppm
        src/Node.baseline.cppm
        src/Node.optimise.cppm
)



target_link_libraries(${PROJECT_NAME} PRIVATE MPI::MPI_CXX)

target_compile_options(${PROJECT_NAME} PRIVATE
    $<$<CONFIG:Release>:-O3 -march=native -funroll-loops -fomit-frame-pointer>
)

# Add compiler-specific flags AFTER defining the target
if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    target_compile_options(${PROJECT_NAME} PRIVATE
        -fmodules
        -fcxx-modules
        -stdlib=libc++
        -fmodules
        -fbuiltin-module-map
        -fimplicit-module-maps
    )
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_options(${PROJECT_NAME} PRIVATE
        -fmodules-ts
    )
endif()

