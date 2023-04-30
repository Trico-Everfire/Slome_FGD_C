cmake_minimum_required(VERSION 3.25)
project(Slome_FGD_C C)

set(CMAKE_C_STANDARD 17)

set(SRC
        src/libslome.c
        src/include/libslome.h
        src/tokenizer.h
        src/tokenizer.c)

if (STOME_TEST_APPLICATION)
    add_executable(${PROJECT_NAME} src/main.c ${SRC})
else ()
    add_library(${PROJECT_NAME} STATIC ${SRC})
    target_include_directories(${PROJECT_NAME} PUBLIC src/include/)
endif ()

