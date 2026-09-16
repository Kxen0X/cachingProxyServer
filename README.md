# Caching Proxy Server

High-performance asynchronous HTTP/HTTPS caching proxy server implemented in C++20 using **Boost.Asio**, **OpenSSL**, and **CMake**.

This project is an implementation of the [roadmap.sh Caching Proxy Server Project](https://roadmap.sh/projects/caching-server).

---

## 📑 Table of Contents

- [Overview](#-overview)
- [Key Features](#-key-features)
- [Tech Stack & Prerequisites](#-tech-stack--prerequisites)
- [Installation & Building](#-installation--building)
- [Usage & CLI Interface](#-usage--cli-interface)
- [Testing the Proxy](#-testing-the-proxy)
- [Cache Verification & Headers](#-cache-verification--headers)

---

## 🚀 Overview

`cachingProxyServer` acts as an intermediary CLI application between incoming HTTP/HTTPS client requests and an upstream origin server. 

When a client requests a resource:
1. **Cache Hit:** If the resource exists in the local cache, the proxy returns the cached response directly with an `X-Cache: HIT` header without contacting the origin server.
2. **Cache Miss:** If the resource is not cached or invalidated, the proxy forwards the request to the origin server, stores the response in memory (with LRU eviction management), and returns it to the client with an `X-Cache: MISS` header.

---

## ✨ Key Features

* **Asynchronous I/O Execution:** Built with C++20 coroutines (`boost::asio::awaitable` / `co_await`) for lightweight, non-blocking request handling.
* **Thread-Safe LRU Cache:** Implements a thread-safe Least Recently Used (LRU) in-memory cache to maintain frequent requests while controlling memory usage.
* **HTTPS / SSL Support:** Full SSL/TLS handshake support via **OpenSSL** (`boost::asio::ssl::stream`) for securing upstream forwarding and tunneling.
* **Command Line Management:** CLI flags for setting custom ports, defining origin target URLs, and executing cache clearing operations.
* **Header Injection:** Appends custom `X-Cache: HIT` and `X-Cache: MISS` headers to HTTP responses for instant status inspection.

---

## 🛠 Tech Stack & Prerequisites

* **Language:** C++20
* **Networking:** Boost.Asio
* **Security:** OpenSSL
* **Build System:** CMake (v3.20+)
* **Package Management:** `vcpkg` / `FetchContent` / System Package Manager

---

## ⚙️ Installation & Building

### 1. Clone the Repository

```bash
git clone [https://github.com/Kxen0X/cachingProxyServer.git](https://github.com/Kxen0X/cachingProxyServer.git)
cd cachingProxyServer
