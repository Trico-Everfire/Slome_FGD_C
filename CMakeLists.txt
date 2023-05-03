cmake_minimum_required(VERSION 3.25)
project(Slome_FGD_C C)

set(CMAKE_C_STANDARD 17)

option(SLOME_UNIFIED_FGD "Unified FDD are FGD Alterations made by TeamSpen's HammerAddons.
[https://github.com/TeamSpen210/HammerAddons/wiki/Unified-FGD]
and is fully backwards compatible with Valve's FGD standard." OFF)

message(STATUS "SLOME_UNIFIED_FGD: ${SLOME_UNIFIED_FGD}")

set(SRC
        src/libslome.c
        src/include/libslome.h
        src/tokenizer.h
        src/tokenizer.c
        src/include/enums.h)

if (STOME_TEST_APPLICATION)
    add_executable(${PROJECT_NAME} src/main.c ${SRC})
else ()
    add_library(${PROJECT_NAME} STATIC ${SRC})
    target_include_directories(${PROJECT_NAME} PUBLIC src/include/)
endif ()
if(SLOME_UNIFIED_FGD)
    add_definitions(-DSLOME_UNIFIED_FGD)
endif ()