cmake_minimum_required(VERSION 3.16)

project("chess-engine")

set(CMAKE_C_FLAGS "-std=c11 ${CMAKE_C_FLAGS} -Wall -Wpedantic -O3")

add_executable(${CMAKE_PROJECT_NAME} src/main.c)