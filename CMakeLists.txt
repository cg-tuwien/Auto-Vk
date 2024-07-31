cmake_minimum_required(VERSION 3.16)
project(avk)

if (MSVC)
    # <ranges> support requires /std:c++latest on MSVC
    set(CMAKE_CXX_STANDARD 20)
else (MSVC)
    set(CMAKE_CXX_STANDARD 20)
endif (MSVC)

set(avk_AllowedLibraryTypes INTERFACE SHARED STATIC)
set(avk_LibraryType INTERFACE CACHE STRING
        "The type of library avk should be built as. Must be one of ${avk_AllowedLibraryTypes}. Default: INTERFACE")
set_property(CACHE avk_LibraryType PROPERTY STRINGS ${avk_AllowedLibraryTypes})

if(NOT avk_LibraryType IN_LIST avk_AllowedLibraryTypes)
    message(FATAL_ERROR "avk_LibraryType must be one of ${avk_AllowedLibraryTypes}")
endif()

option(avk_UseVMA "Use Vulkan Memory Allocator (VMA) for internal memory allocation." OFF)

set(avk_IncludeDirs
        include)
set(avk_Sources
        src/avk.cpp)

if(avk_UseVMA)
    add_compile_definitions(AVK_USE_VMA)
endif()

add_library(${PROJECT_NAME} ${avk_LibraryType})

if(NOT avk_LibraryType STREQUAL "INTERFACE")
    target_include_directories(${PROJECT_NAME} PUBLIC ${avk_IncludeDirs})
    target_sources(${PROJECT_NAME} PUBLIC ${avk_Sources})
    find_package(Vulkan)
    target_include_directories(${PROJECT_NAME} PUBLIC
        ${Vulkan_INCLUDE_DIR})
else()
    target_include_directories(${PROJECT_NAME} INTERFACE ${avk_IncludeDirs})
    target_sources(${PROJECT_NAME} INTERFACE ${avk_Sources})
endif()
