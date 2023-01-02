cmake_minimum_required(VERSION 3.13.0)
project(MIPS-CPU-simulator VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(MIPS-CPU-simulator)

# source files
target_sources(MIPS-CPU-simulator
        PRIVATE
        main.cpp
        )