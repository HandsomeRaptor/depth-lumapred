cmake_minimum_required(VERSION 3.2)

project(depth-lumapred)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_INSTALL_PREFIX ${PROJECT_SOURCE_DIR})

file(GLOB lumapred_src "src/*.cpp")

set(CMAKE_CXX_FLAGS_RELEASE "-DNDEBUG -Wall -Ofast")

include(CheckCXXCompilerFlag)
CHECK_CXX_COMPILER_FLAG("-march=native" COMPILER_SUPPORTS_MARCH_NATIVE)
if(COMPILER_SUPPORTS_MARCH_NATIVE)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=native")
endif()

add_executable(lumapred-test main.cpp ${lumapred_src})
target_include_directories(lumapred-test PRIVATE include)


add_executable(lumapred-decbench decoderbench.cpp ${lumapred_src})
target_include_directories(lumapred-decbench PRIVATE include)