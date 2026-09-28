cmake_minimum_required(VERSION 3.16)
project(ternet LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(tnc
    src/main.cpp
    src/lexer.cpp
    src/parser.cpp
    src/compiler.cpp
    src/vm.cpp
)

target_include_directories(tnc PRIVATE include)

if (MSVC)
    target_compile_options(tnc PRIVATE /W4)
else()
    target_compile_options(tnc PRIVATE -Wall -Wextra -Wpedantic)
endif()
