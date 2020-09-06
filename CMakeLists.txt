cmake_minimum_required(VERSION 3.13)
project(avk)

set(CMAKE_CXX_STANDARD 20)

add_library(${PROJECT_NAME} STATIC)
target_include_directories(${PROJECT_NAME} PUBLIC
        include)
target_sources(${PROJECT_NAME} PUBLIC
        src/avk.cpp
        src/sync.cpp)
