#pragma once


#include <memory>
#include <thread>
#include <mutex>
#include <optional>
#include <iostream>
#include <algorithm>
#include <cstdint>
#include <unordered_map>
#include <list>
#include <string_view>
#include <filesystem>
#include <fstream>

#ifdef _WIN32
#define _WIN32_WINNT 0x0A00
#endif

#include <asio.hpp>
#include <asio/ts/buffer.hpp>
#include <asio/ts/internet.hpp>
