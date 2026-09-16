# Caching Proxy Server

High-performance asynchronous HTTP/HTTPS caching proxy server implemented in C++20 using **Boost.Asio**, **OpenSSL**, and **CMake**.

This project is an implementation of the [roadmap.sh Caching Proxy Server Project](https://roadmap.sh/projects/caching-server).

---

## 📑 Table of Contents

- [Overview](#-overview)
- [Key Features](#-key-features)
- [Tech Stack & Prerequisites](#-tech-stack--prerequisites)
- [Building & Setup](#-building--setup)
- [Usage & CLI Interface](#-usage--cli-interface)
- [Testing the Proxy](#-testing-the-proxy)

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
* **Multi-Process CLI Management:** Supports starting the main proxy daemon process and running a secondary process with the `--clear-cache` flag to issue cache clearing commands.
* **Header Injection:** Appends custom `X-Cache: HIT` and `X-Cache: MISS` headers to HTTP responses for instant status inspection.

---

## 🛠 Tech Stack & Prerequisites

* **C++ Compiler:** GCC (11+), Clang (13+), or MSVC with full C++20 standard support
* **Build System:** CMake (v3.20+)
* **Libraries:**
  * Boost (specifically `Boost.Asio`)
  * OpenSSL

---

## ⚙️ Building & Setup

### 1. Clone the Repository

```bash
git clone https://github.com/Kxen0X/cachingProxyServer.git
cd cachingProxyServer
```

### 2. Configure Build Directory

Generate build files using CMake:

```bash
cmake -B build -S . -DCMAKE_BUILD_TYPE=Release
```

### 3. Compile the Executable

Build the binary target:

```bash
cmake --build build --config Release
```

After building, the compiled binary `caching-proxy` will be located inside the `./build` directory.

---

## 💻 Usage & CLI Interface

### Running the Proxy Server

Start the primary server process using standard command-line options:

```bash
./build/caching-proxy --port <PORT> --origin <ORIGIN_URL>
```

### Clearing the Cache (Separate Process)

To clear the active cache while the proxy server is running, execute a second process in another terminal window with the `--clear-cache` flag:

```bash
./build/caching-proxy --clear-cache
```

### Command Line Arguments

| Flag | Description | Example |
| :--- | :--- | :--- |
| `--port <number>` | Port on which the caching proxy server listens | `--port 3000` |
| `--origin <url>` | Target origin server URL to forward requests to | `--origin http://dummyjson.com` |
| `--clear-cache` | Launches a separate process to clear the proxy cache | `--clear-cache` |

---

## 🧪 Testing the Proxy

You can verify the proxy behavior using `curl` or any HTTP client.

### Step 1: Start Main Proxy Process
```bash
./build/caching-proxy --port 3000 --origin http://dummyjson.com
```

### Step 2: First Request (Cache Miss)
```bash
curl -i http://localhost:3000/products
```
**Expected Response Header:**
```http
HTTP/1.1 200 OK
X-Cache: MISS
Content-Type: application/json
...
```

### Step 3: Second Request (Cache Hit)
```bash
curl -i http://localhost:3000/products
```
**Expected Response Header:**
```http
HTTP/1.1 200 OK
X-Cache: HIT
Content-Type: application/json
...
```

### Step 4: Clear Memory Cache (Run in a Second Terminal Process)
```bash
./build/caching-proxy --clear-cache
```
