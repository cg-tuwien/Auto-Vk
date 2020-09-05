cmake_minimum_required(VERSION 3.13)
project(avk)

set(CMAKE_CXX_STANDARD 20)

add_library(${PROJECT_NAME} INTERFACE)
target_include_directories(${PROJECT_NAME} INTERFACE
        include)
target_sources(${PROJECT_NAME} INTERFACE
        src/avk.cpp
        src/sync.cpp)


# this is only temporary for testing purposes
# find & link Vulkan
add_executable(demo examples/main.cpp)
find_package(Vulkan)
target_link_libraries(demo ${PROJECT_NAME} Vulkan::Vulkan)

