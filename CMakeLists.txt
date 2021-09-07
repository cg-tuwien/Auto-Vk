cmake_minimum_required(VERSION 3.13)
project(avk)

set(CMAKE_CXX_STANDARD 20)

set(avk_AllowedLibraryTypes INTERFACE SHARED STATIC)
set(avk_LibraryType INTERFACE CACHE STRING
        "The type of library avk should be built as. Must be one of ${avk_AllowedLibraryTypes}. Default: INTERFACE")
set_property(CACHE avk_LibraryType PROPERTY STRINGS ${avk_AllowedLibraryTypes})

if(NOT avk_LibraryType IN_LIST avk_AllowedLibraryTypes)
    message(FATAL_ERROR "avk_LibraryType must be one of ${avk_AllowedLibraryTypes}")
endif()

option(avk_UseVMA "Use Vulkan Memory Allocator (VMA) for custom memory allocation." OFF)

set(avk_IncludeDirs
        include)
set(avk_Sources
        src/avk.cpp
        src/sync.cpp)

if(avk_UseVMA)
    list(APPEND avk_Sources src/vk_mem_alloc.cpp)
endif()

add_library(${PROJECT_NAME} ${avk_LibraryType})

if(NOT avk_LibraryType STREQUAL "INTERFACE")
    if (MSVC)
        target_compile_options(${PROJECT_NAME}
            PUBLIC "/std:c++latest")
    endif (MSVC)

    target_include_directories(${PROJECT_NAME} PUBLIC ${avk_IncludeDirs})
    target_sources(${PROJECT_NAME} PUBLIC ${avk_Sources})
    find_package(Vulkan)
    target_include_directories(${PROJECT_NAME} PUBLIC
        ${Vulkan_INCLUDE_DIR})
else()
    if (MSVC)
        target_compile_options(${PROJECT_NAME}
            INTERFACE "/std:c++latest")
    endif (MSVC)

    target_include_directories(${PROJECT_NAME} INTERFACE ${avk_IncludeDirs})
    target_sources(${PROJECT_NAME} INTERFACE ${avk_Sources})
endif()