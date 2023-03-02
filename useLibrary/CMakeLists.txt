cmake_minimum_required(VERSION 3.10)
project(MyProject)

# Add the include directory
include_directories(include)

# Add all the source files in the src directory
file(GLOB SOURCES "src/*.cpp")

# Create the executable
add_executable(MyProject ${SOURCES})