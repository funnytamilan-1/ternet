cmake_minimum_required(VERSION 3.16)
project(ternet LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(tnc
    Programs/tnc/main.cpp
    Parser/lexer.cpp
    Parser/parser.cpp
    Ternet/vm.cpp
    Tools/package.cpp
    Modules/telegram.cpp
)
target_include_directories(tnc PRIVATE Include)

if(MSVC)
    target_compile_options(tnc PRIVATE /W4)
else()
    target_compile_options(tnc PRIVATE -Wall -Wextra -Wpedantic)
endif()

enable_testing()
add_test(NAME ternet_core COMMAND tnc run ${CMAKE_SOURCE_DIR}/tests/core.trn)
add_test(NAME ternet_loop COMMAND tnc run ${CMAKE_SOURCE_DIR}/tests/loop.trn)
add_test(NAME ternet_errors COMMAND tnc run ${CMAKE_SOURCE_DIR}/tests/errors.trn)
add_test(NAME ternet_block_comments COMMAND tnc run ${CMAKE_SOURCE_DIR}/tests/block_comments.trn)
add_test(NAME ternet_web COMMAND tnc web build ${CMAKE_SOURCE_DIR}/tests/web.trn)

add_executable(ternet_telegram_test
    tests/telegram_client.cpp
    Modules/telegram.cpp
)
target_include_directories(ternet_telegram_test PRIVATE Include)
add_test(NAME ternet_telegram COMMAND ternet_telegram_test)
