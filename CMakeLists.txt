cmake_minimum_required(VERSION 3.5)
project(BasicECS)

# Set the name of the executable
set(EXE_NAME testBasicECS)

# Set compiler options
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fdiagnostics-color=always -Wall -g -Wno-deprecated")

include_directories(./src) 
include_directories(./include) 

# Add executable target
file(GLOB_RECURSE SOURCES
    src/*.cpp
    test.cpp
)

# Add executable target
add_executable(${EXE_NAME} ${SOURCES})
