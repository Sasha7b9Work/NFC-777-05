cmake_minimum_required(VERSION 3.14)

project (NFC-777-05)

set(CMAKE_UNITY_BUILD ON)

if(${CMAKE_BUILD_TYPE} STREQUAL Debug)
    add_definitions(-DDEBUG)
endif()

add_subdirectory(Reader/vs_keil799)
add_subdirectory(Loader-799)
