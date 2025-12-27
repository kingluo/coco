# coco

A simple, stackless, single-threaded, header-only C++20 coroutine library with Go-like concurrency primitives.

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)

## Overview

**coco** is a lightweight C++20 coroutine library that brings Go-style concurrency to C++.

- 🚀 **Native C++20 coroutines** - Leverages standard C++20 coroutine support
- 📦 **Header-only** - Just include `coco.h`, no linking required
- 🔄 **Go-like primitives** - Channels (`chan_t`) and wait groups (`wg_t`)
- ⚡ **Zero dependencies** - Only requires C++20 standard library
- 🎯 **Single-threaded** - No locks, no data races, cooperative multitasking
- 🔧 **Simple scheduler** - FIFO queue-based coroutine scheduling
- 🎨 **Clean API** - Intuitive async/await syntax

## Key Features

- **Coroutines (`co_t`)** - Stackless coroutines with join support and exception propagation
- **Channels (`chan_t<T>`)** - Type-safe communication channels (buffered and unbuffered)
- **Wait Groups (`wg_t`)** - Synchronization primitive for coordinating multiple coroutines
- **Custom Awaiters** - Easy integration with external async systems (io_uring, timers, etc.)
- **RAII-friendly** - Proper resource management across suspension points
- **Extensively tested** - Comprehensive test suite with 20+ test files

## Requirements

- **C++20 compiler** with coroutine support:
  - GCC 10+ (with `-std=c++20 -fcoroutines`)
  - Clang 14+ (with `-std=c++20`)
  - MSVC 2019+ (with `/std:c++20`)
- Standard library with `<coroutine>` header support

## Synopsis

### Basic Coroutine Usage

```cpp
#include "coco.h"
using namespace coco;

// 1. Define a coroutine - must return co_t and use co_await/co_yield/co_return
co_t simple_task(int id) {
    std::cout << "Task " << id << " started" << std::endl;
    co_yield resched;  // Yield control, will be automatically rescheduled
    std::cout << "Task " << id << " completed" << std::endl;
    co_return;
}

// 2. Start and run coroutines
int main() {
    auto task = simple_task(1);  // Create the coroutine
    task.resume();               // Schedule it for execution

    // 3. Must use scheduler to run all coroutines
    scheduler_t::instance().run();

    return 0;
}
```

### Using Join to Compose Multiple Coroutines

C++20 coroutines have a fundamental limitation: **you cannot directly call one coroutine from another and await its result**. This is because coroutine keywords (`co_await`, `co_yield`, `co_return`) must appear directly in the coroutine function body.

**What DOESN'T Work:**
```cpp
co_t authenticate_user(const std::string& username) {
    std::cout << "Authenticating " << username << "..." << std::endl;
    co_yield resched;
    std::cout << "Authentication successful" << std::endl;
    co_return;
}

// ❌ This FAILS - mixing return with coroutine keywords
co_t handle_request_WRONG(const std::string& username) {
    if (username.empty()) {
        return authenticate_user(username);  // Compilation error!
        // Error: cannot use 'return' in a coroutine (must use co_return)
    }
    co_yield resched;  // This makes the function a coroutine
    co_return;
}

// ❌ This also FAILS - cannot directly await a function call
co_t handle_request_ALSO_WRONG(const std::string& username) {
    co_await authenticate_user(username);  // Doesn't work as expected!
    // The coroutine is created but never scheduled/resumed
    co_return;
}
```

**What DOES Work - Using `join()`:**

The `join()` method allows you to properly compose coroutines by starting them and waiting for completion:

```cpp
// Step 1: Authenticate user
co_t authenticate_user(const std::string& username) {
    std::cout << "Authenticating " << username << "..." << std::endl;
    co_yield resched;  // Simulate async auth
    std::cout << "Authentication successful" << std::endl;
    co_return;
}

// Step 2: Load user data
co_t load_user_data(int user_id) {
    std::cout << "Loading data for user " << user_id << "..." << std::endl;
    co_yield resched;  // Simulate async database query
    std::cout << "User data loaded" << std::endl;
    co_return;
}

// Step 3: Process request
co_t process_request(const std::string& request) {
    std::cout << "Processing request: " << request << std::endl;
    co_yield resched;  // Simulate async processing
    std::cout << "Request processed" << std::endl;
    co_return;
}

// ✅ Correct: Use join() to compose coroutines
co_t handle_user_request(const std::string& username, int user_id, const std::string& request) {
    // Shortcut: use go().join() to create, schedule, and join in one expression
    co_await go([&](){ return authenticate_user(username); }).join();
    co_await go([&](){ return load_user_data(user_id); }).join();
    co_await go([&](){ return process_request(request); }).join();

    std::cout << "All steps completed!" << std::endl;
    co_return;
}

int main() {
    auto request_handler = handle_user_request("alice", 123, "GET /data");
    request_handler.resume();
    scheduler_t::instance().run();
    return 0;
}
```

**Why Join is Needed:**
- You **cannot** return a coroutine from another coroutine (compilation error)
- You **cannot** directly `co_await` a coroutine function call without scheduling it first
- You **must** create the coroutine, schedule it with `resume()`, then `co_await` its `join()`
- **Shortcut pattern**: Use `co_await go(...).join()` to create, schedule, and join in one expression
- This pattern allows sequential composition of independent coroutines
- Each coroutine runs independently and can be joined when its result is needed

### Producer-Consumer with Channels

```cpp
co_t producer(chan_t<int>& ch) {
    for (int i = 0; i < 3; i++) {
        std::cout << "Sending: " << i << std::endl;
        bool ok = co_await ch.write(i);
        if (!ok) {
            std::cout << "Channel closed, stopping producer" << std::endl;
            break;
        }
    }
    ch.close();
    std::cout << "Producer finished" << std::endl;
}

co_t consumer(chan_t<int>& ch, const std::string& name) {
    while (true) {
        auto result = co_await ch.read();
        if (result.has_value()) {
            std::cout << name << " received: " << result.value() << std::endl;
        } else {
            std::cout << name << " channel closed" << std::endl;
            break;
        }
    }
}

int main() {
    chan_t<int> ch(1); // Buffered channel with capacity 1

    auto prod = producer(ch);
    auto cons1 = consumer(ch, "Consumer1");
    auto cons2 = consumer(ch, "Consumer2");

    // Resume coroutines and run scheduler
    prod.resume();
    cons1.resume();
    cons2.resume();
    scheduler_t::instance().run();

    std::cout << "---> ALL DONE!" << std::endl;
    return 0;
}
```

### Running Multiple Coroutines with WaitGroup

```cpp
co_t worker(int id, wg_t& wg) {
    std::cout << "Worker " << id << " starting" << std::endl;
    co_yield resched;  // Simulate async work
    std::cout << "Worker " << id << " finished" << std::endl;
    wg.done();  // Signal completion
    co_return;
}

co_t run_workers() {
    wg_t wg;
    wg.add(3);  // Expecting 3 workers

    // Start all workers simultaneously
    auto w1 = go([&wg](){ return worker(1, wg); });
    auto w2 = go([&wg](){ return worker(2, wg); });
    auto w3 = go([&wg](){ return worker(3, wg); });

    // Wait for all workers to complete
    co_await wg.wait();
    std::cout << "All workers completed!" << std::endl;
    co_return;
}

int main() {
    auto main_task = run_workers();
    main_task.resume();
    scheduler_t::instance().run();
    return 0;
}
```

## Installation

**coco** is a header-only library. Simply copy `coco.h` to your project:

```bash
# Clone the repository
git clone https://github.com/kingluo/coco.git

# Copy the header to your project
cp coco/coco.h /path/to/your/project/
```

Or include it directly:
```cpp
#include "path/to/coco.h"
```

## Examples

The `examples/` directory contains practical demonstrations:

### 1. Channel and Wait Group (`channel_and_waitgroup.cpp`)
Producer-consumer pattern with channels and scheduler usage.

### 2. Coroutine Join (`join_example.cpp`)
Demonstrates coroutine composition, join functionality, and exception handling.

### 3. Web Server (`webserver.cpp`)
High-performance HTTP server using io_uring for async I/O with custom awaiters.

### Building Examples

```bash
# Build all examples
make examples

# Or build individually
cd examples/
make

# Run examples
../build/channel_and_waitgroup
../build/join_example

# Run webserver (requires liburing)
make run-webserver

# Test webserver (in another terminal)
curl -i http://localhost:8000
```

## Testing

The library includes a comprehensive test suite with 20+ test files covering:

- Core components (`co_t`, `chan_t`, `wg_t`)
- Integration tests (producer-consumer, pipelines, fan-out/fan-in)
- Channel behavior (Go compatibility, fair distribution, stress tests)
- C++20 coroutine caveats (RAII, stack relocation, lifetime management)
- Bug fixes and edge cases

### Running Tests

```bash
# Run all tests
make test

# Or run specific test categories
cd tests/
make run-core-tests      # Core library tests
make run-channel-tests   # Channel behavior tests
make run-wg-tests        # Wait group tests

# Run individual tests
make run-co-t
make run-chan-t
make run-integration
```

See [`tests/README.md`](tests/README.md) for detailed test documentation.

## API Reference

### Core Types

#### `co_t` - Coroutine Type

The fundamental coroutine type that wraps C++20 coroutine handles.

```cpp
struct co_t {
    std::coroutine_handle<promise_type> handle;  // Underlying coroutine handle

    void resume();                         // Schedule coroutine for execution
    join_awaiter join();                   // Await coroutine completion
    bool is_done() const;                  // Check if coroutine is completed
    std::exception_ptr get_exception() const;  // Get captured exception (if any)

    // Move-only semantics
    co_t(co_t&& other) noexcept;
    co_t& operator=(co_t&& other) noexcept;

    // Non-copyable
    co_t(const co_t&) = delete;
    co_t& operator=(const co_t&) = delete;
};
```

**Key Methods:**
- **`resume()`**: Schedules the coroutine for execution via `scheduler_t::instance().schedule(handle)`. Does not execute immediately; the coroutine runs when the scheduler processes it.
- **`join()`**: Returns a `join_awaiter` that can be used with `co_await` to wait for the coroutine to complete.
- **`is_done()`**: Returns `true` if the coroutine has finished execution.
- **`get_exception()`**: Returns the `std::exception_ptr` captured by `unhandled_exception()` if the coroutine threw an exception, or `nullptr` if no exception occurred.

**Usage:**
- Functions using `co_await`, `co_yield`, or `co_return` must return `co_t`
- Use `co_yield resched` to yield control back to the scheduler (automatically resumes later)
- Use `co_yield no_sched` to yield without automatic rescheduling (manual resume required)
- Use `co_await coroutine.join()` to wait for a coroutine to complete
- Use `coroutine.is_done()` to check if a coroutine has finished
- Coroutines are automatically destroyed when `co_t` goes out of scope

**Join Functionality:**
The `join()` method allows you to await the completion of any coroutine:
- **Exception Propagation**: If the joined coroutine throws an exception, it will be rethrown in the joining coroutine
- **Multiple Joiners**: Multiple coroutines can join the same coroutine
- **Immediate Return**: If the coroutine is already completed, `join()` returns immediately

**Exception Handling:**
Coroutines support standard C++ exception handling:
- Exceptions thrown in a coroutine are captured by `unhandled_exception()` in the promise
- Use `co_await coroutine.join()` to automatically propagate exceptions to the joining coroutine
- Use `coroutine.get_exception()` to manually check for exceptions without rethrowing
- Example:
  ```cpp
  co_t worker() {
      throw std::runtime_error("Worker failed!");
      co_return;
  }

  co_t supervisor() {
      auto w = worker();
      w.resume();
      scheduler_t::instance().run();

      // Check for exception without rethrowing
      if (auto ex = w.get_exception()) {
          std::cout << "Worker threw an exception" << std::endl;
          // Handle error...
      }
      co_return;
  }
  ```

**Note:** `co_yield resched` automatically schedules the coroutine for resumption, making it a true cooperative yielding mechanism. The coroutine will suspend and allow other coroutines to run, then automatically resume when the scheduler processes it. Use `co_yield no_sched` if you need to yield without automatic rescheduling (requires manual resume).

#### `go()` - Coroutine Launcher

```cpp
co_t go(std::function<co_t()> fn);
```

Convenience function that creates and immediately resumes a coroutine. Equivalent to:
```cpp
auto co = fn();
co.resume();
return co;
```

**Example:**
```cpp
co_t simple_task(int id) {
    std::cout << "Task " << id << std::endl;
    co_return;
}

// Use go() to create and immediately schedule a coroutine
auto task = go([](){ return simple_task(1); });
// task is now scheduled and will run when scheduler processes it
scheduler_t::instance().run();
```

**Note:** The lambda passed to `go()` is a regular function (not a coroutine) that returns a coroutine. This is valid because the lambda itself doesn't use coroutine keywords - it just calls `simple_task()` which creates and returns a `co_t`.

#### `chan_t<T>` - Channel Type

Communication channels between coroutines, similar to Go channels. Channels are single-threaded and use the scheduler for coroutine coordination.

```cpp
template<typename T>
class chan_t {
public:
    // Constructor
    chan_t(int capacity = 0);  // 0 = unbuffered, >0 = buffered

    // Channel operations (awaitable)
    read_wait_t read();        // Returns std::optional<T>
    write_wait_t write(T data); // Returns bool

    // Channel state
    size_t size();             // Current number of items in buffer
    size_t cap();              // Channel capacity
    bool ready();              // Has data available for reading
    bool closed();             // Is channel closed

    // Channel management
    void close();              // Close channel and wake all waiting coroutines
};
```

**Channel Operations:**
```cpp
// Reading from channel
auto result = co_await channel.read();
if (result.has_value()) {
    T value = result.value();
    // Process value
} else {
    // Channel is closed
}

// Writing to channel
bool success = co_await channel.write(value);
if (!success) {
    // Channel is closed
}
```

**Channel Types:**
- **Unbuffered** (`capacity = 0`): Synchronous communication, writer blocks until reader is ready
- **Buffered** (`capacity > 0`): Asynchronous communication up to capacity limit

**Important:** When a channel operation suspends (blocks), the coroutine handle is automatically scheduled via `scheduler_t::instance().schedule()` when the operation can proceed. This means you must run the scheduler to process channel operations.

#### `wg_t` - Wait Group Type

Synchronization primitive for waiting on multiple coroutines, similar to Go wait groups.

```cpp
class wg_t {
public:
    void add(uint64_t delta = 1);   // Add work items to wait for
    void done();                    // Mark one work item as complete
    async_wait_t wait();            // Wait for all work items to complete (awaitable)
};
```

**Usage Pattern:**
```cpp
co_t example() {
    wg_t wg;
    wg.add(3);  // Expecting 3 workers

    // Start worker coroutines
    auto worker = [&wg](int id) -> co_t {
        // Do work...
        wg.done();  // Signal completion
        co_return;
    };

    // Wait for all workers to complete
    co_await wg.wait();
    co_return;
}
```

**Important:** When `done()` is called and the counter reaches zero, all waiting coroutines are scheduled via `scheduler_t::instance().schedule()`. The scheduler must be running to process these resumed coroutines.

#### `wg_guard_t` - RAII Guard for Wait Groups

RAII wrapper that automatically calls `wg.done()` when the guard goes out of scope, ensuring proper cleanup even in the presence of exceptions.

```cpp
class wg_guard_t {
public:
    wg_guard_t(wg_t* wg);  // Constructor takes waitgroup pointer
    ~wg_guard_t();          // Destructor calls wg->done()
};
```

**Usage Pattern:**
```cpp
co_t worker(int id, wg_t& wg) {
    wg_guard_t guard(&wg);  // Automatically calls wg.done() on scope exit

    std::cout << "Worker " << id << " starting" << std::endl;
    co_yield resched;  // Simulate async work

    // If exception occurs here, guard still calls wg.done()
    if (id == 2) {
        throw std::runtime_error("Worker 2 failed!");
    }

    std::cout << "Worker " << id << " finished" << std::endl;
    // guard destructor calls wg.done() automatically
    co_return;
}

co_t run_workers_with_guard() {
    wg_t wg;
    wg.add(3);

    auto w1 = go([&wg](){ return worker(1, wg); });
    auto w2 = go([&wg](){ return worker(2, wg); });  // Will throw exception
    auto w3 = go([&wg](){ return worker(3, wg); });

    // Wait for all workers - even if some throw exceptions, wg.done() is called
    co_await wg.wait();
    std::cout << "All workers completed (with or without errors)" << std::endl;
    co_return;
}
```

**Benefits:**
- **Exception Safety**: Guarantees `wg.done()` is called even if the coroutine throws an exception
- **No Manual Cleanup**: Eliminates the need to manually call `wg.done()` before every `co_return`
- **RAII Pattern**: Follows C++ best practices for resource management

**When to Use:**
- Use `wg_guard_t` when your worker coroutines might throw exceptions
- Use `wg_guard_t` when you have multiple exit points in your coroutine
- Use manual `wg.done()` for simple cases without exception handling

#### `scheduler_t` - Global Scheduler

A simple FIFO queue-based scheduler for managing coroutine execution. The scheduler is a thread-local singleton accessed via `scheduler_t::instance()`.

```cpp
class scheduler_t {
public:
    std::queue<std::coroutine_handle<>> q;  // Queue of scheduled coroutines

    void schedule(std::coroutine_handle<> handle);  // Add handle to queue
    void run();                                      // Process all queued handles
    void clear();                                    // Clear the queue

    static scheduler_t& instance();                  // Get thread-local instance
};
```

**Key Methods:**
- **`schedule(handle)`**: Adds a coroutine handle to the queue if it's valid and not done
- **`run()`**: Processes all handles in the queue until empty. Each handle is resumed once.
- **`clear()`**: Empties the queue without resuming any coroutines
- **`instance()`**: Returns the thread-local singleton scheduler instance

**Usage Pattern:**
```cpp
// The scheduler is used automatically by coco primitives
auto task = go([](){ return simple_task(); });

// Run the scheduler to process all scheduled coroutines
scheduler_t::instance().run();
```

**Important Notes:**
- The scheduler is **automatically used** by all coco primitives (channels, waitgroups, join, resume)
- When a coroutine is resumed via `co_t::resume()`, it's scheduled, not executed immediately
- When channel/waitgroup operations unblock, they schedule waiting coroutines
- You **must call** `scheduler_t::instance().run()` to actually execute scheduled coroutines
- The scheduler processes coroutines in FIFO order

### Encapsulating Async Events with User-Defined Awaiters

You can integrate external async events (like I/O operations, timers, or custom event systems) by creating custom awaiter types. **Critical requirement:** You must use the scheduler to resume the coroutine handle.

An awaiter must implement three methods:
- `bool await_ready() const noexcept` - Check if result is immediately available
- `void await_suspend(std::coroutine_handle<> handle) noexcept` - Called when coroutine suspends, save the handle
- `ReturnType await_resume() noexcept` - Called when coroutine resumes, return the result

#### Example: I/O Uring Integration

```cpp
#include "coco.h"
#include <liburing.h>

// 1. Define the awaiter - saves the handle and stores the result
struct iouring_awaiter {
    int res = -1;  // Store I/O result
    std::coroutine_handle<> handle;  // Save coroutine handle for resumption

    bool await_ready() const noexcept { return false; }  // Always suspend

    void await_suspend(std::coroutine_handle<> h) noexcept {
        handle = h;  // Save the handle - will be resumed when I/O completes
    }

    int await_resume() noexcept { return res; }  // Return the I/O result
};

// 2. Submit I/O operation and associate the awaiter
iouring_awaiter& async_read(io_uring* ring, int fd, void* buf, size_t len, iouring_awaiter* awaiter) {
    io_uring_sqe* sqe = io_uring_get_sqe(ring);
    io_uring_prep_read(sqe, fd, buf, len, 0);
    io_uring_sqe_set_data(sqe, awaiter);  // Associate awaiter with this I/O operation
    io_uring_submit(ring);
    return *awaiter;
}

// 3. Event loop processes I/O completions
void event_loop(io_uring* ring) {
    while (true) {
        io_uring_cqe* cqe;
        io_uring_wait_cqe(ring, &cqe);

        // Get the awaiter from completion event
        iouring_awaiter* awaiter = (iouring_awaiter*)cqe->user_data;
        awaiter->res = cqe->res;  // Store the result

        // CRITICAL: Use scheduler to resume the coroutine handle
        if (awaiter->handle) {
            scheduler_t::instance().schedule(awaiter->handle);
        }

        io_uring_cqe_seen(ring, cqe);

        // Run scheduler to process resumed coroutines
        scheduler_t::instance().run();
    }
}

// 4. Use the awaiter in a coroutine
co_t handle_request(io_uring* ring, int client_fd) {
    char buffer[4096];
    iouring_awaiter read_awaiter;

    // co_await suspends, saves handle in awaiter, returns when I/O completes
    int bytes_read = co_await async_read(ring, client_fd, buffer, sizeof(buffer), &read_awaiter);

    if (bytes_read > 0) {
        std::cout << "Read " << bytes_read << " bytes" << std::endl;
        // Process data...
    }
    co_return;
}

int main() {
    io_uring ring;
    io_uring_queue_init(256, &ring, 0);

    // Start coroutine
    auto coro = handle_request(&ring, client_fd);
    coro.resume();

    // Run event loop - processes I/O and resumes coroutines
    event_loop(&ring);

    return 0;
}
```

**Key Points:**
1. **Save the handle**: In `await_suspend()`, save the `std::coroutine_handle<>` in the awaiter
2. **Associate awaiter with event**: Store the awaiter pointer in the async operation (e.g., `io_uring_sqe_set_data`)
3. **Resume via scheduler**: When the event completes, call `scheduler_t::instance().schedule(handle)` to resume the coroutine
4. **Run the scheduler**: Call `scheduler_t::instance().run()` to execute resumed coroutines
5. **Store results**: Use awaiter fields to pass results from the event handler to `await_resume()`

### Usage Guidelines

Since coco uses C++20 coroutines, there are some important usage patterns to follow:

1. **Coroutine Functions**: Functions that use `co_await`, `co_yield`, or `co_return` must return `co_t`.
2. **Awaitable Objects**: Use `co_await` with channel operations (`read()`, `write()`), waitgroup operations (`wait()`), coroutine join operations (`join()`), or custom awaitables.
3. **Yielding**: Use `co_yield resched` to yield control back to the caller/scheduler with automatic rescheduling, or `co_yield no_sched` to yield without automatic rescheduling.
4. **Channel Operations**:
   - `co_await channel.read()` returns `std::optional<T>` (nullopt when channel is closed)
   - `co_await channel.write(value)` returns `bool` (false when channel is closed)
5. **Waitgroups**: Use `wg.add(n)` to add work, `wg.done()` to mark completion, and `co_await wg.wait()` to wait for all work to finish.
6. **Coroutine Join**: Use `co_await coroutine.join()` to wait for a specific coroutine to complete. Exceptions are automatically propagated.
7. **Scheduler**: The scheduler is automatically used by all coco primitives. You must call `scheduler_t::instance().run()` to execute scheduled coroutines.
8. **Memory Management**: Channels, waitgroups, and scheduler are RAII objects - no manual deletion needed.
9. **Exception Safety**: Exceptions can be used normally within coroutines and are properly propagated through join operations.
10. **Custom Awaiters**: When creating custom awaiters, always use `scheduler_t::instance().schedule()` to resume coroutine handles.

### Error Handling and Best Practices

#### Channel Error Handling

```cpp
co_t safe_channel_operations(chan_t<int>& ch) {
    try {
        // Always check channel state before operations
        if (ch.closed()) {
            std::cout << "Channel already closed" << std::endl;
            co_return;
        }

        // Handle write failures
        bool write_ok = co_await ch.write(42);
        if (!write_ok) {
            std::cout << "Write failed - channel closed" << std::endl;
        }

        // Handle read from closed channel
        auto result = co_await ch.read();
        if (result.has_value()) {
            std::cout << "Read value: " << result.value() << std::endl;
        } else {
            std::cout << "Channel closed or empty" << std::endl;
        }

    } catch (const std::exception& e) {
        std::cout << "Exception in coroutine: " << e.what() << std::endl;
    }
    co_return;
}
```

#### Resource Management

```cpp
co_t resource_safe_coroutine() {
    // RAII objects work normally in coroutines
    std::unique_ptr<int> resource = std::make_unique<int>(42);
    chan_t<int> local_channel(5);

    // IMPORTANT: Resources remain alive during co_await/co_yield suspension
    // They are only destroyed when the coroutine function actually returns
    co_await local_channel.write(*resource);  // resource still valid here
    auto result = co_await local_channel.read();  // and here

    // Resources destroyed only when coroutine completes (co_return)
    co_return;
}
```

#### Performance Tips

1. **Yielding Strategy**: Use `co_yield resched` to prevent coroutine starvation
   ```cpp
   co_t cpu_intensive_task() {
       for (int i = 0; i < 1000000; i++) {
           // Do work...
           if (i % 1000 == 0) {
               co_yield resched;  // Yield periodically
           }
       }
       co_return;
   }
   ```

2. **Memory Efficiency**: Prefer move semantics for large objects
   ```cpp
   co_t efficient_data_transfer(chan_t<std::vector<int>>& ch) {
       std::vector<int> large_data(10000);
       // Move instead of copy
       bool ok = co_await ch.write(std::move(large_data));
       co_return;
   }
   ```
