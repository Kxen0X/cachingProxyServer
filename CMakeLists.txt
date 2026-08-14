cmake_minimum_required(VERSION 3.14)
project(cachingProxyServer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
FetchContent_Declare(
    asio
    GIT_REPOSITORY https://github.com/chriskohlhoff/asio.git
    GIT_TAG        master
)

FetchContent_GetProperties(asio)
if(NOT asio_POPULATED)
    FetchContent_Populate(asio)
endif()

find_path(ASIO_INCLUDE_DIR asio.hpp
    HINTS
        "${asio_SOURCE_DIR}/asio/include"
        "${asio_SOURCE_DIR}/include"
        "${asio_SOURCE_DIR}"
)

message(STATUS "===> REAL ASIO INCLUDE DIR: ${ASIO_INCLUDE_DIR} <===")

add_executable(caching-proxy
    cachingProxy/src/cachingProxyServer.cpp
    cachingProxy/include/cachingProxyServer.h
    cachingProxy/include/net_common.h
    cachingProxy/include/session.h
)

target_compile_definitions(caching-proxy PRIVATE ASIO_STANDALONE)

target_include_directories(caching-proxy PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/cachingProxy/include
    ${ASIO_INCLUDE_DIR}
)

find_package(Threads REQUIRED)
target_link_libraries(caching-proxy PRIVATE Threads::Threads)