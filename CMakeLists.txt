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
    compiler/bytecode.cpp
    compiler/types.cpp
)
target_include_directories(tnc PRIVATE Include compiler)

if(MSVC)
    target_compile_options(tnc PRIVATE /W4)
else()
    target_compile_options(tnc PRIVATE -Wall -Wextra -Wpedantic -Wno-free-nonheap-object)
endif()

enable_testing()
add_test(NAME ternet_core COMMAND tnc run ${CMAKE_SOURCE_DIR}/tests/core.trn)
add_test(NAME ternet_loop COMMAND tnc run ${CMAKE_SOURCE_DIR}/tests/loop.trn)
add_test(NAME ternet_errors COMMAND tnc run ${CMAKE_SOURCE_DIR}/tests/errors.trn)
add_test(NAME ternet_block_comments COMMAND tnc run ${CMAKE_SOURCE_DIR}/tests/block_comments.trn)
add_test(NAME ternet_range_decimal COMMAND tnc run ${CMAKE_SOURCE_DIR}/tests/range_decimal.trn)
add_test(NAME ternet_web COMMAND tnc web build ${CMAKE_SOURCE_DIR}/tests/web.trn)
add_test(NAME ternet_web_bytecode_build COMMAND tnc build ${CMAKE_SOURCE_DIR}/tests/web.trn -o ${CMAKE_CURRENT_BINARY_DIR}/web.tbc)
add_test(NAME ternet_web_bytecode_exec COMMAND tnc exec ${CMAKE_CURRENT_BINARY_DIR}/web.tbc)
set_tests_properties(ternet_web_bytecode_exec PROPERTIES DEPENDS ternet_web_bytecode_build)

add_executable(ternet_telegram_test
    tests/telegram_client.cpp
    Modules/telegram.cpp
)
target_include_directories(ternet_telegram_test PRIVATE Include)
add_test(NAME ternet_telegram COMMAND ternet_telegram_test)

add_test(NAME ternet_bytecode_build COMMAND tnc build ${CMAKE_SOURCE_DIR}/tests/bytecode.trn -o ${CMAKE_CURRENT_BINARY_DIR}/bytecode.tbc)
add_test(NAME ternet_bytecode_exec COMMAND tnc exec ${CMAKE_CURRENT_BINARY_DIR}/bytecode.tbc)
set_tests_properties(ternet_bytecode_exec PROPERTIES DEPENDS ternet_bytecode_build)
add_test(NAME ternet_bytecode_arrays_build COMMAND tnc build ${CMAKE_SOURCE_DIR}/tests/bytecode_arrays.trn -o ${CMAKE_CURRENT_BINARY_DIR}/bytecode_arrays.tbc)
add_test(NAME ternet_bytecode_arrays_exec COMMAND tnc exec ${CMAKE_CURRENT_BINARY_DIR}/bytecode_arrays.tbc)
set_tests_properties(ternet_bytecode_arrays_exec PROPERTIES DEPENDS ternet_bytecode_arrays_build)
add_test(NAME ternet_bytecode_control_build COMMAND tnc build ${CMAKE_SOURCE_DIR}/tests/bytecode_control.trn -o ${CMAKE_CURRENT_BINARY_DIR}/bytecode_control.tbc)
add_test(NAME ternet_bytecode_control_exec COMMAND tnc exec ${CMAKE_CURRENT_BINARY_DIR}/bytecode_control.tbc)
set_tests_properties(ternet_bytecode_control_exec PROPERTIES DEPENDS ternet_bytecode_control_build)
add_test(NAME ternet_bytecode_short_circuit_build COMMAND tnc build ${CMAKE_SOURCE_DIR}/tests/bytecode_short_circuit.trn -o ${CMAKE_CURRENT_BINARY_DIR}/bytecode_short_circuit.tbc)
add_test(NAME ternet_bytecode_short_circuit_exec COMMAND tnc exec ${CMAKE_CURRENT_BINARY_DIR}/bytecode_short_circuit.trb)
set_tests_properties(ternet_bytecode_short_circuit_exec PROPERTIES DEPENDS ternet_bytecode_short_circuit_build)
add_test(NAME ternet_bytecode_functions_build COMMAND tnc build ${CMAKE_SOURCE_DIR}/tests/bytecode_functions.trn -o ${CMAKE_CURRENT_BINARY_DIR}/bytecode_functions.tbc)
add_test(NAME ternet_bytecode_functions_exec COMMAND tnc exec ${CMAKE_CURRENT_BINARY_DIR}/bytecode_functions.tbc)
set_tests_properties(ternet_bytecode_functions_exec PROPERTIES DEPENDS ternet_bytecode_functions_build)
add_test(NAME ternet_lit_function COMMAND tnc run ${CMAKE_SOURCE_DIR}/tests/lit_function.trn)
add_test(NAME ternet_option_result COMMAND tnc run ${CMAKE_SOURCE_DIR}/tests/option_result.trn)
add_test(NAME ternet_option_result_bytecode_build COMMAND tnc build ${CMAKE_SOURCE_DIR}/tests/option_result.trn -o ${CMAKE_CURRENT_BINARY_DIR}/option_result.tbc)
add_test(NAME ternet_option_result_bytecode_exec COMMAND tnc exec ${CMAKE_CURRENT_BINARY_DIR}/option_result.tbc)
set_tests_properties(ternet_option_result_bytecode_exec PROPERTIES DEPENDS ternet_option_result_bytecode_build)
add_test(NAME ternet_structs COMMAND tnc run ${CMAKE_SOURCE_DIR}/tests/structs.trn)
add_test(NAME ternet_enums_match COMMAND tnc run ${CMAKE_SOURCE_DIR}/tests/enums_match.trn)
add_test(NAME ternet_classes COMMAND tnc run ${CMAKE_SOURCE_DIR}/tests/classes.trn)
add_test(NAME ternet_imports COMMAND tnc run ${CMAKE_SOURCE_DIR}/tests/imports.trn)
add_test(NAME ternet_imports_bytecode_build COMMAND tnc build ${CMAKE_SOURCE_DIR}/tests/imports.trn -o ${CMAKE_CURRENT_BINARY_DIR}/imports.tbc)
add_test(NAME ternet_imports_bytecode_exec COMMAND tnc exec ${CMAKE_CURRENT_BINARY_DIR}/imports.tbc)
set_tests_properties(ternet_imports_bytecode_exec PROPERTIES DEPENDS ternet_imports_bytecode_build)
add_test(NAME ternet_structs_bytecode_build COMMAND tnc build ${CMAKE_SOURCE_DIR}/tests/structs.trn -o ${CMAKE_CURRENT_BINARY_DIR}/structs.tbc)
add_test(NAME ternet_structs_bytecode_exec COMMAND tnc exec ${CMAKE_CURRENT_BINARY_DIR}/structs.tbc)
set_tests_properties(ternet_structs_bytecode_exec PROPERTIES DEPENDS ternet_structs_bytecode_build)
add_test(NAME ternet_for_typed COMMAND tnc run ${CMAKE_SOURCE_DIR}/tests/for_typed.trn)
add_test(NAME ternet_for_typed_check COMMAND tnc check ${CMAKE_SOURCE_DIR}/tests/for_typed.trn)
add_test(NAME ternet_for_typed_invalid COMMAND tnc check ${CMAKE_SOURCE_DIR}/tests/for_typed_invalid.trn)
set_tests_properties(ternet_for_typed_invalid PROPERTIES WILL_FAIL TRUE)

add_test(NAME ternet_typecheck COMMAND tnc check ${CMAKE_SOURCE_DIR}/tests/bytecode.trn)
add_test(NAME ternet_typecheck_invalid COMMAND tnc check ${CMAKE_SOURCE_DIR}/tests/typecheck_invalid.trn)
set_tests_properties(ternet_typecheck_invalid PROPERTIES WILL_FAIL TRUE)