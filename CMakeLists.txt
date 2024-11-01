cmake_minimum_required(VERSION 3.5)
project(Viking)

# Set the name of the executable
set(EXE_NAME ecsTest)

# Set compiler options
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fdiagnostics-color=always -Wall -g -Wno-deprecated")

include_directories(./) 

# Add executable target
file(GLOB_RECURSE SOURCES
    ecs.cpp
    test.cpp
)

# Add executable target
add_executable(${EXE_NAME} ${SOURCES})
