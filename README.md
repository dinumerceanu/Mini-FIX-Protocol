# High-Performance Trading Engine

⚡ A simplified trading engine prototype that simulates client-server order execution, inspired by real-world electronic trading systems.  
Built with **C++17**, it showcases advanced concepts in networking, concurrency, and low-latency order handling.

---

## 🚀 Key Features
- **Custom Client/Server Architecture**
  - Non-blocking I/O with `fcntl` and epoll-style loops
  - Error-safe wrappers (`check`, `make_nonblocking`) for robust system calls
- **Order Execution Engine**
  - Tracks order lifecycle with `OrderExecutionInfo`
  - Generates FIX-like `ExecutionReport` messages
  - Handles partial fills, complete fills, and unfilled orders
- **Concurrency**
  - Custom **thread pool** implementation using templates (`thread_pool.tpp`)
  - Work distribution for efficient request handling
- **Session Layer**
  - Message parsing & routing abstraction
  - Client/session management decoupled from business logic
- **Parser Module**
  - Converts raw server messages into structured events
  - Supports incremental order book updates
- **Utility Layer**
  - Generic helpers for string manipulation, ID generation, and error handling

---

## 📂 Project Structure
```text
.
├── src/                # Entry points: server.cpp, client.cpp
├── include/
│   ├── order_execution # Order lifecycle & FIX-like reports
│   ├── parser          # Raw → structured message parser
│   ├── session_layer   # Connection/session management
│   ├── thread_pool     # Custom concurrency utilities
│   └── utils           # Shared helpers

🛠️ Build & Run

make
./server    # Run server
./client    # Run client

🔑 Core Concepts Demonstrated

    Low-level UNIX socket programming with non-blocking file descriptors

    Custom protocol parsing and execution reporting (FIX-inspired)

    Thread pool pattern for scalable request processing

    Modern C++17 memory management with RAII and STL containers

    Clean modular design (parser, session_layer, thread_pool, order_execution)

📖 Why It’s Interesting

This project combines networking, concurrency, and financial protocols into a compact prototype.
It’s not just about sending messages—it mimics how actual trading systems handle order lifecycle events (fills, partial fills, rejects) while keeping latency low.
🔮 Future Improvements

    Implement risk checks (credit limits, duplicate order detection)

    Add persistent storage for order history (SQLite / flat files)

    Optimize with lock-free data structures for ultra-low latency

    Introduce latency benchmarks and profiling

    Extend FIX message coverage (cancel/replace, reject handling)

    Add unit tests for parser, execution engine, and session layer