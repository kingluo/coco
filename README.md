# coco

coco is a simple stackless, single-threaded, and header-only C++20 coroutine library.

## Design

1. Uses C++20 coroutines for native async/await support.
2. Header-only library with no external dependencies.
3. Stackless coroutines with async/await implementation similar to Rust.
4. Channel and waitgroup primitives like Go.
5. Single-threaded, no locks required.
6. Minimal performance overhead.
7. Simple scheduler for managing multiple coroutines.

## Synopsis

channels:

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

    // Simple round-robin scheduler
    for (int i = 0; i < 100; i++) { // Limit iterations to prevent infinite loop
        bool any_active = false;

        if (prod.handle && !prod.handle.done()) {
            prod.resume();
            any_active = true;
        }
        if (cons1.handle && !cons1.handle.done()) {
            cons1.resume();
            any_active = true;
        }
        if (cons2.handle && !cons2.handle.done()) {
            cons2.resume();
            any_active = true;
        }

        // Check if all coroutines are done
        if (prod.handle.done() && cons1.handle.done() && cons2.handle.done()) {
            break;
        }

        if (!any_active) break;
    }

    std::cout << "---> ALL DONE! check errors if any." << std::endl;
    return 0;
}
```

webserver with io_uring:

```cpp
struct iouring_awaiter {
    int res = -1;
    std::coroutine_handle<> handle;

    bool await_ready() const noexcept { return false; }
    void await_suspend(std::coroutine_handle<> h) noexcept { handle = h; }
    int await_resume() noexcept { return res; }
};

struct conn_state_t {
    int client_socket = -1;
    static constexpr int MAX_IOVLEN = 16;
    int iovec_count = 0;
    std::array<iovec, MAX_IOVLEN> iov{};
    iouring_awaiter read_awaiter;
    iouring_awaiter write_awaiter;

    conn_state_t(int sk) : client_socket(sk) {}
    // ... additional methods for resource management
};

co_t handle_connection(int client_socket) {
    auto req = std::make_unique<conn_state_t>(client_socket);

    try {
        // Read request
        int bytes_read = co_await read_request(req.get());
        if (bytes_read <= 0) {
            // Client disconnected or read error - this is normal
            co_return;
        }

        // Handle request
        handle_request(req.get());

        // Send response
        int bytes_sent = co_await send_response(req.get());
        if (bytes_sent <= 0) {
            // Failed to send response - client may have disconnected
        }
    } catch (...) {
        // Handle any exceptions to prevent coroutine from being left in bad state
        printf("Exception in handle_connection for socket %d\n", client_socket);
    }
}

co_t server_coroutine(int server_socket) {
    while (true) {
        int client_socket = co_await do_accept(server_socket, nullptr, nullptr);
        if (client_socket < 0) {
            // Accept failed - this can happen during server shutdown
            continue;
        }

        // Start a new coroutine to handle this connection and store it
        active_connections.emplace_back(handle_connection(client_socket));

        // Clean up completed connections
        active_connections.remove_if([](const co_t& conn) {
            return conn.handle.done();
        });
    }
}
```

## API Reference

### Core Types

#### `co_t` - Coroutine Type

The fundamental coroutine type that wraps C++20 coroutine handles.

```cpp
struct co_t {
    void resume();                    // Resume coroutine execution
    join_awaiter join();             // Await coroutine completion
    bool is_done() const;            // Check if coroutine is completed

    // Move-only semantics
    co_t(co_t&& other) noexcept;
    co_t& operator=(co_t&& other) noexcept;

    // Non-copyable
    co_t(const co_t&) = delete;
    co_t& operator=(const co_t&) = delete;
};
```

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

**Note:** `co_yield resched` automatically schedules the coroutine for resumption, making it a true cooperative yielding mechanism. The coroutine will suspend and allow other coroutines to run, then automatically resume when the scheduler processes it. Use `co_yield no_sched` if you need to yield without automatic rescheduling (requires manual resume).

#### `chan_t<T>` - Channel Type

Thread-safe communication channels between coroutines, similar to Go channels.

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
    size_t size();             // Current number of items
    size_t cap();              // Channel capacity
    bool ready();              // Has data available for reading
    bool closed();             // Is channel closed

    // Channel management
    void close(std::function<void(T)> cleanup = nullptr);
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

#### `scheduler_t` - Simple Scheduler

A round-robin scheduler for managing multiple coroutines.

```cpp
#include "scheduler.h"

class scheduler_t {
public:
    // Add coroutines
    template<typename CoroFunc>
    void spawn(CoroFunc&& func);           // Add coroutine from factory function
    void add_coroutine(co_t&& coro);       // Add existing coroutine

    // Execution control
    void run(size_t max_iterations = 10000);  // Run until completion
    bool step();                               // Execute one scheduling round
    template<typename Condition>
    void run_while(Condition&& condition);     // Run with custom condition

    // Status
    size_t active_count();                     // Number of active coroutines
    size_t total_count();                      // Total coroutines
    bool has_active_coroutines();              // Any active coroutines?
    void cleanup_completed();                  // Remove completed coroutines
};
```

**Usage Pattern:**
```cpp
scheduler_t scheduler;

// Add coroutines
scheduler.spawn([]() -> co_t {
    // Your coroutine code
    co_yield resched; // Yield to allow other coroutines to run
    co_return;
});

// Run all coroutines
scheduler.run();
```

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
7. **Scheduler**: Use `scheduler_t` to manage multiple coroutines with fair round-robin scheduling.
8. **Memory Management**: Channels, waitgroups, and scheduler are RAII objects - no manual deletion needed.
9. **Exception Safety**: Exceptions can be used normally within coroutines and are properly propagated through join operations.

### Advanced Usage Patterns

#### Producer-Consumer with Multiple Workers

```cpp
co_t producer(chan_t<int>& ch, int count) {
    for (int i = 0; i < count; i++) {
        bool ok = co_await ch.write(i);
        if (!ok) break;  // Channel closed
    }
    ch.close();
    co_return;
}

co_t consumer(chan_t<int>& ch, const std::string& name) {
    while (true) {
        auto result = co_await ch.read();
        if (!result.has_value()) break;  // Channel closed

        std::cout << name << " processed: " << result.value() << std::endl;
        co_yield resched;  // Yield to other coroutines
    }
    co_return;
}

int main() {
    chan_t<int> ch(10);  // Buffered channel

    auto prod = producer(ch, 100);
    auto cons1 = consumer(ch, "Worker-1");
    auto cons2 = consumer(ch, "Worker-2");

    // Simple scheduler loop
    while (!prod.handle.done() || !cons1.handle.done() || !cons2.handle.done()) {
        if (!prod.handle.done()) prod.resume();
        if (!cons1.handle.done()) cons1.resume();
        if (!cons2.handle.done()) cons2.resume();
    }
    return 0;
}
```

#### Coordinated Work with Wait Groups

```cpp
co_t worker(int id, wg_t& wg, chan_t<std::string>& results) {
    // Simulate work
    for (int i = 0; i < 3; i++) {
        co_yield resched;  // Simulate async work
    }

    // Report result
    std::string result = "Worker " + std::to_string(id) + " completed";
    co_await results.write(result);

    wg.done();  // Signal completion
    co_return;
}

co_t coordinator() {
    wg_t wg;
    chan_t<std::string> results(10);

    // Start workers
    wg.add(3);
    auto w1 = worker(1, wg, results);
    auto w2 = worker(2, wg, results);
    auto w3 = worker(3, wg, results);

    // Wait for all workers
    co_await wg.wait();
    results.close();

    // Collect results
    while (true) {
        auto result = co_await results.read();
        if (!result.has_value()) break;
        std::cout << result.value() << std::endl;
    }

    co_return;
}
```

#### Coroutine Join Operations

The `join()` method provides a clean way to wait for coroutine completion without manual coordination:

```cpp
co_t worker_task(int id) {
    std::cout << "Worker " << id << " starting" << std::endl;
    co_yield resched;  // Simulate async work
    std::cout << "Worker " << id << " completed" << std::endl;
    co_return;
}

co_t coordinator_with_join() {
    // Start multiple tasks
    auto task1 = go([]() -> co_t { return worker_task(1); });
    auto task2 = go([]() -> co_t { return worker_task(2); });
    auto task3 = go([]() -> co_t { return worker_task(3); });

    // Wait for all tasks to complete
    co_await task1.join();
    co_await task2.join();
    co_await task3.join();

    std::cout << "All workers completed!" << std::endl;
    co_return;
}
```

**Exception Handling with Join:**
```cpp
co_t risky_task() {
    co_yield resched;
    throw std::runtime_error("Something went wrong!");
    co_return;
}

co_t error_handler() {
    auto task = go(risky_task);

    try {
        co_await task.join();
        std::cout << "Task completed successfully" << std::endl;
    } catch (const std::exception& e) {
        std::cout << "Task failed: " << e.what() << std::endl;
    }
    co_return;
}
```

**Multiple Joiners:**
```cpp
co_t shared_task() {
    std::cout << "Shared task working..." << std::endl;
    co_yield resched;
    std::cout << "Shared task done!" << std::endl;
    co_return;
}

co_t multiple_joiners_example() {
    auto task = go(shared_task);

    // Multiple coroutines can join the same task
    auto joiner1 = go([&task]() -> co_t {
        co_await task.join();
        std::cout << "Joiner 1 notified" << std::endl;
        co_return;
    });

    auto joiner2 = go([&task]() -> co_t {
        co_await task.join();
        std::cout << "Joiner 2 notified" << std::endl;
        co_return;
    });

    // Wait for all joiners to complete
    co_await joiner1.join();
    co_await joiner2.join();
    co_return;
}
```

#### Custom Awaitable Integration

```cpp
struct timer_awaitable {
    int milliseconds;

    bool await_ready() const noexcept { return milliseconds <= 0; }
    void await_suspend(std::coroutine_handle<> handle) noexcept {
        // In real implementation, would use actual timer
        // For demo, just resume immediately
        handle.resume();
    }
    void await_resume() noexcept {}
};

co_t timed_worker(chan_t<int>& ch) {
    for (int i = 0; i < 5; i++) {
        co_await timer_awaitable{100};  // Wait 100ms
        bool ok = co_await ch.write(i);
        if (!ok) break;
    }
    ch.close();
    co_return;
}
```

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

    // Resources are automatically cleaned up when coroutine ends
    // even if suspended and resumed multiple times

    co_await local_channel.write(*resource);
    auto result = co_await local_channel.read();

    // resource and local_channel automatically destroyed
    co_return;
}
```

#### Performance Tips

1. **Channel Sizing**: Choose appropriate buffer sizes
   - Unbuffered (0): Use for synchronization points
   - Small buffer (1-10): Use for loose coupling
   - Large buffer (100+): Use for high-throughput scenarios

2. **Yielding Strategy**: Use `co_yield resched` to prevent coroutine starvation
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

3. **Memory Efficiency**: Prefer move semantics for large objects
   ```cpp
   co_t efficient_data_transfer(chan_t<std::vector<int>>& ch) {
       std::vector<int> large_data(10000);
       // Move instead of copy
       bool ok = co_await ch.write(std::move(large_data));
       co_return;
   }
   ```

## Requirements

- C++20 compiler with coroutine support (GCC 10+, Clang 14+, MSVC 2019+)
- Standard library with `<coroutine>` header support
- For webserver example: liburing development library

## Building and Running Examples

The `examples/` directory contains practical demonstrations of coco's features:

### Quick Start

```bash
# Build all examples
cd examples/
make all

# Run the channel and waitgroup example
make run-channel

# Run the webserver example (requires liburing)
make run-webserver
```

### Available Examples

1. **channel_and_waitgroup.cpp** - Demonstrates channel communication between producer/consumer coroutines
2. **webserver.cpp** - HTTP server using io_uring for async I/O operations

### Example Commands

```bash
# Check dependencies
make check-deps

# Install dependencies on Ubuntu/Debian
make install-deps

# Build and test all examples
make test-build

# Get help with all available targets
make help
```

### Testing the Webserver

```bash
# Terminal 1: Start the server
make run-webserver

# Terminal 2: Test with curl
curl -i http://localhost:8000
curl -i http://localhost:8000/tux.png
```

## Building and Running Tests

The `.tests/` directory contains comprehensive unit tests for all coco components:

### Quick Start

```bash
# Build and run all tests
cd .tests/
make test

# Build all test executables
make all

# Run individual test suites
make run-co-t        # Test co_t coroutine type
make run-chan-t      # Test chan_t channel type
make run-wg-t        # Test wg_t waitgroup type
make run-integration # Test integration scenarios
```

### Available Test Suites

1. **test_co_t.cpp** - Tests for the coroutine type and basic coroutine operations
2. **test_chan_t.cpp** - Tests for channel operations, buffering, and closing
3. **test_wg_t.cpp** - Tests for waitgroup synchronization primitives
4. **test_integration.cpp** - Integration tests combining multiple components
5. **test_cpp20_caveats.cpp** - Tests demonstrating C++20 coroutine limitations and best practices
6. **test_advanced_caveats.cpp** - Advanced tests for coroutine behavior and edge cases

### Test Commands

```bash
# Check compiler support
make check-compiler

# Clean build artifacts
make clean

# Get help with all available targets
make help
```

## Quick Reference

### Essential Includes
```cpp
#include "coco.h"
using namespace coco;
```

### Basic Coroutine
```cpp
co_t my_coroutine() {
    // Coroutine body
    co_yield resched;   // Yield control (with automatic rescheduling)
    co_return;          // End coroutine
}
```

### Channel Operations
```cpp
chan_t<int> ch(capacity);           // Create channel
auto result = co_await ch.read();   // Read (returns std::optional<T>)
bool ok = co_await ch.write(value); // Write (returns bool)
ch.close();                         // Close channel
```

### Wait Group Operations
```cpp
wg_t wg;
wg.add(n);              // Add n work items
wg.done();              // Mark one item complete
co_await wg.wait();     // Wait for all items
```

### Channel States
```cpp
ch.size()     // Current items in channel
ch.cap()      // Channel capacity
ch.ready()    // Has data available
ch.closed()   // Is channel closed
```

### Common Patterns
```cpp
// Producer-Consumer
co_t producer(chan_t<T>& ch) {
    for (auto item : items) {
        if (!co_await ch.write(item)) break;
    }
    ch.close();
    co_return;
}

// Worker with WaitGroup
co_t worker(wg_t& wg) {
    // Do work...
    wg.done();
    co_return;
}

// Worker with Join (simpler alternative to WaitGroup)
co_t worker_task() {
    // Do work...
    co_return;
}

co_t coordinator() {
    auto task = go(worker_task);
    co_await task.join();  // Wait for completion
    co_return;
}

// Simple Scheduler (using the provided scheduler_t)
#include "scheduler.h"
scheduler_t scheduler;
scheduler.spawn([]() -> co_t { /* your coroutine */ co_return; });
scheduler.run();
```
