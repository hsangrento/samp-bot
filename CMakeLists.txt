cmake_minimum_required(VERSION 3.15)
project(sampbot CXX)

if(NOT MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -m32 -fPIC")
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -m32")
endif()

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(sampbot SHARED
    src/main.cpp
    sdk/amxplugin.cpp
)

target_include_directories(sampbot PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/sdk
    ${CMAKE_CURRENT_SOURCE_DIR}/lib/urmem
)

if(NOT MSVC)
    target_compile_definitions(sampbot PRIVATE LINUX)
endif()

set_target_properties(sampbot PROPERTIES
    PREFIX ""
    OUTPUT_NAME "sampbot"
)

if(MSVC)
    target_compile_definitions(sampbot PRIVATE _CRT_SECURE_NO_WARNINGS WIN32_LEAN_AND_MEAN)
    set_property(TARGET sampbot PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded")
    set(SAMPBOT_DEF "${CMAKE_CURRENT_BINARY_DIR}/sampbot.def")
    file(WRITE "${SAMPBOT_DEF}" "EXPORTS\nSupports\nLoad\nUnload\nAmxLoad\nAmxUnload\n")
    target_sources(sampbot PRIVATE "${SAMPBOT_DEF}")
endif()
