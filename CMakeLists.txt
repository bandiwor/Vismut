cmake_minimum_required(VERSION 3.16)
project(Vismut C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Отключаем LTO для MinGW, так как он часто вызывает проблемы
set(RELEASE_OPTIMIZATION_FLAGS
        -O2
        -static
        -s
        -march=native
        -ffast-math
        -funroll-loops
        -finline-functions
        -fomit-frame-pointer
)

# Debug флаги
set(DEBUG_FLAGS
        -O0
        -g3
        -Wall
        -Wextra
        -Wpedantic
)

# Проверяем поддержку LTO, но не используем его по умолчанию для MinGW
include(CheckCCompilerFlag)
check_c_compiler_flag(-flto SUPPORTS_LTO)

# Если очень нужно LTO, можно использовать только для не-MinGW компиляторов
if(SUPPORTS_LTO AND NOT CMAKE_C_COMPILER_ID MATCHES "GNU")
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELEASE TRUE)
    message(STATUS "LTO enabled for non-MinGW compiler")
else()
    message(STATUS "LTO disabled - not supported or MinGW compiler")
endif()

add_executable(Vismut main.c
        Vismut/core/types.h
        Vismut/io/reader/reader.h
        Vismut/io/reader/reader.c
        Vismut/core/Vismut.h
        Vismut/core/errors/errors.h
        Vismut/core/errors/errors.c
        Vismut/core/tokenizer/tokenizer.h
        Vismut/core/tokenizer/tokenizer.c
        Vismut/core/tokenizer/token.h
        Vismut/core/types_maps.h
        Vismut/core/tokenizer/token.c
        Vismut/core/types.c
        Vismut/core/convert.h
        Vismut/core/convert.c
        Vismut/core/ast/ast.h
        Vismut/core/ast/value.h
        Vismut/core/ast/ast.c
        Vismut/core/ast/ast_parse.c
        Vismut/core/ast/ast_parse.h
        Vismut/core/memory/arena.h
        Vismut/core/memory/arena.c
        Vismut/core/errors/callstack.h
        Vismut/core/errors/callstack.c
        Vismut/core/debug.h
        Vismut/core/ansi_colors.h
        Vismut/core/ansi_colors.c
        Vismut/core/ast/ast_analyze.h
        Vismut/core/ast/ast_analyze.c
        Vismut/core/ast/scope.h
        Vismut/core/ast/scope.c
        Vismut/core/hash/murmur3.h
        Vismut/core/hash/murmur3.c
        Vismut/core/ast/ast_typing.h
        Vismut/core/ast/ast_typing.c
        Vismut/core/ast/ast_optimize.c
        Vismut/core/ast/ast_optimize.h
        Vismut/core/codegen/codegen.c
        Vismut/core/codegen/codegen.h
        Vismut/core/codegen/run.h
        Vismut/core/codegen/run.c
        Vismut/utils/find_position.h
        Vismut/utils/find_position.c
        Vismut/utils/module_name.h
        Vismut/utils/module_name.c)

# Разделение флагов по конфигурациям
target_compile_options(Vismut PRIVATE
        -Wno-unused-parameter
        $<$<CONFIG:Release>:${RELEASE_OPTIMIZATION_FLAGS} -DNDEBUG>
        $<$<CONFIG:Debug>:${DEBUG_FLAGS}>
)

# Для линковки используем те же флаги, но без проблемных
target_link_options(Vismut PRIVATE
        $<$<CONFIG:Release>:-static -march=native -ffast-math -funroll-loops>
)

# Цель для генерации препроцессированного файла
add_custom_target(preprocess_types
        COMMAND ${CMAKE_C_COMPILER} -E -P -nostdinc
        -I ${CMAKE_CURRENT_SOURCE_DIR}
        -I ${CMAKE_CURRENT_SOURCE_DIR}/Vismut/core
        -I ${CMAKE_CURRENT_SOURCE_DIR}/Vismut/io/reader
        -I ${CMAKE_CURRENT_SOURCE_DIR}/Vismut/core/errors
        -I ${CMAKE_CURRENT_SOURCE_DIR}/Vismut/utils
        -I ${CMAKE_CURRENT_SOURCE_DIR}/Vismut/core/tokenizer
        ${CMAKE_CURRENT_SOURCE_DIR}/Vismut/core/types.c
        > ${CMAKE_CURRENT_BINARY_DIR}/types_preprocessed.c
        COMMENT "Preprocessing types.c"
        VERBATIM
)

# Или для конкретного файла с исходным кодом функции
add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/token_types_preprocessed.c
        COMMAND ${CMAKE_C_COMPILER} -E -P
        -I ${CMAKE_CURRENT_SOURCE_DIR}
        -I ${CMAKE_CURRENT_SOURCE_DIR}/Vismut/core
        ${CMAKE_CURRENT_SOURCE_DIR}/Vismut/core/types.c
        > ${CMAKE_CURRENT_BINARY_DIR}/token_types_preprocessed.c
        DEPENDS Vismut/core/types.c
        COMMENT "Preprocessing token types"
)
