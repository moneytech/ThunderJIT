cmake_minimum_required(VERSION 3.2)
project(ThunderJIT)

include_directories(include)

add_executable(ThunderJIT main.c include/assembler.h src/assembler.c include/assembler_x64.h include/constants_x64.h src/assembler_x64.c)