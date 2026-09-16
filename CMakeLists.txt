cmake_minimum_required(VERSION 3.14)
project(cachingProxyServer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

if(WIN32)
    message(STATUS "Downloading and building OpenSSL via FetchContent for Windows...")
    FetchContent_Declare(
        openssl
        GIT_REPOSITORY https://github.com/janbar/openssl-cmake.git
        GIT_TAG        master
    )
    FetchContent_MakeAvailable(openssl)
else()
    find_package(OpenSSL REQUIRED)
endif()

FetchContent_Declare(
    asio
    GIT_REPOSITORY https://github.com/chriskohlhoff/asio.git
    GIT_TAG        master
)
FetchContent_MakeAvailable(asio)

find_path(ASIO_INCLUDE_DIR asio.hpp
    HINTS
        "${asio_SOURCE_DIR}/asio/include"
        "${asio_SOURCE_DIR}/include"
        "${asio_SOURCE_DIR}"
)

message(STATUS "===> REAL ASIO INCLUDE DIR: ${ASIO_INCLUDE_DIR} <===")

add_executable(caching-proxy
    cachingProxy/src/main.cpp
    cachingProxy/src/cachingProxyServer.cpp
    cachingProxy/src/LRUCache.cpp
    cachingProxy/src/session.cpp
    cachingProxy/include/cachingProxyServer.h
    cachingProxy/include/net_common.h
    cachingProxy/include/session.h
    cachingProxy/include/LRUCache.h 
)

target_compile_definitions(caching-proxy PRIVATE 
    ASIO_STANDALONE
    ASIO_HAS_OPENSSL
)

target_include_directories(caching-proxy PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/cachingProxy/include
    ${ASIO_INCLUDE_DIR}
)

find_package(Threads REQUIRED)

if(WIN32)
    target_link_libraries(caching-proxy PRIVATE 
        Threads::Threads
        ssl
        crypto
    )
else()
    target_link_libraries(caching-proxy PRIVATE 
        Threads::Threads
        OpenSSL::SSL
    )
endif()