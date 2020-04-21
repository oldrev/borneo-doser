# The following lines of boilerplate have to be in your project's CMakeLists
# in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.5)

include(./cmake/ClangFormat.cmake REQUIRED)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)

project(borneo-doser)


FILE(GLOB_RECURSE ALL_C_SOURCES 
    "./main/src/*.c" 
    "./components/*/src/*.c" 
)

FILE(GLOB_RECURSE ALL_CPP_SOURCES 
    "./main/src/*.cpp" 
    "./components/*/src/*.cpp" 
)

clangformat_setup(${ALL_C_SOURCES} ${ALL_CPP_SOURCES})