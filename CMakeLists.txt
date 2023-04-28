cmake_minimum_required(VERSION 3.25)
project(Slome_FGD_C C)

set(CMAKE_C_STANDARD 17)

set(SRC
        src/library.c src/library.h src/tokenizer.h src/tokenizer.c)

if (STOME_TEST_APPLICATION)
    add_executable(${PROJECT_NAME} src/main.c ${SRC})
else ()
    add_library(${PROJECT_NAME} ${SRC})
endif ()

