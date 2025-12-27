# Coco Library Tests

This directory contains comprehensive tests for the coco library's core components: `co_t`, `chan_t`, and `wg_t`.

## Overview

The coco library provides C++20 coroutine-based primitives for concurrent programming:

- **`co_t`** - Coroutine type for creating and managing coroutines
- **`chan_t<T>`** - Channel type for communication between coroutines (similar to Go channels)
- **`wg_t`** - Wait group type for synchronizing multiple coroutines (similar to Go wait groups)

## Test Categories

### Core Component Tests

1. **`test_co_t.cpp`** - Tests for the `co_t` coroutine type
   - Basic coroutine creation and execution
   - Multiple suspension points
   - State management across suspensions
   - Parameter passing via lambda captures
   - Cooperative scheduling
   - Exception handling

2. **`test_chan_t.cpp`** - Tests for the `chan_t<T>` channel type
   - Basic channel creation and properties
   - Read/write operations with coroutines
   - Channel closure behavior
   - Writing to closed channels
   - Different data types (int, string, custom structs)
   - Buffered vs unbuffered channels

3. **`test_wg_t.cpp`** - Tests for the `wg_t` wait group type
   - Basic wait group operations
   - Single and multiple task synchronization
   - Incremental task addition
   - Delta parameter usage
   - Multiple waiters (documents current behavior)
   - Wait group reuse

4. **`test_integration.cpp`** - Integration tests combining all components
   - Producer-consumer pattern
   - Multiple producers, single consumer
   - Pipeline pattern with multiple stages
   - Fan-out/fan-in pattern

### C++20 Coroutine Caveat Tests

5. **`test_cpp20_caveats.cpp`** - Core caveat tests
   - Top-level function restriction
   - RAII preservation across suspension points
   - Reference/pointer danger demonstration
   - Variable lifetime management

6. **`test_advanced_caveats.cpp`** - Advanced caveat scenarios
   - Complex RAII scenarios
   - Smart pointer behavior
   - Lambda captures
   - Container safety

7. **`test_stack_relocation_demo.cpp`** - Stack relocation demonstration
   - Large stack data tests
   - Multiple suspension tests
   - Address change tracking



### Channel Behavior Tests

9. **`simple_channel_test.cpp`** - Simple channel demonstration from blog
10. **`test_chan_comprehensive.cpp`** - Comprehensive channel functionality
11. **`test_chan_edge_cases.cpp`** - Channel edge cases and error conditions
12. **`test_chan_fair_distribution.cpp`** - Fair distribution among consumers
13. **`test_chan_go_compatibility.cpp`** - Go channel compatibility tests
14. **`test_chan_stress.cpp`** - Channel stress testing
15. **`test_go_channel_compliance.cpp`** - Go channel compliance verification

### Wait Group Tests

16. **`test_wg_simple.cpp`** - Simple wait group scenarios
17. **`test_wg_multiple_waiters.cpp`** - Multiple waiters scenarios
18. **`test_wg_no_bounds.cpp`** - Wait group boundary testing

### Additional Tests

19. **`test_bug_fixes.cpp`** - Bug fix verification tests
20. **`test_fair_distribution.cpp`** - Fair distribution analysis
21. **`test_weak_ptr_improvements.cpp`** - Weak pointer improvements
22. **`test_scheduler_clear.cpp`** - Scheduler clearing functionality
23. **`test_mutex_yield.cpp`** - Mutex yield behavior (may be problematic)

## Building and Running Tests

### Prerequisites

- C++20 compatible compiler (GCC 10+, Clang 10+, MSVC 2019+)
- Make (optional, for using the Makefile)

### Using Make

```bash
# Check compiler support
make check-compiler

# Build all tests
make all

# Build and run core tests
make test

# Run test categories
make run-core-tests        # Core library tests
make run-channel-tests     # Channel behavior tests
make run-wg-tests          # Wait group tests

# Run individual core tests
make run-co-t
make run-chan-t
make run-wg-t
make run-integration
make run-cpp20-caveats
make run-advanced-caveats
make run-stack-relocation-demo

# Clean build artifacts
make clean
```

### Manual Compilation

```bash
# Compile individual tests (generic rule works for most tests)
g++ -std=c++20 -fcoroutines -I.. -Wall -Wextra -Werror -O2 -g -o test_co_t test_co_t.cpp
g++ -std=c++20 -fcoroutines -I.. -Wall -Wextra -Werror -O2 -g -o test_chan_t test_chan_t.cpp
g++ -std=c++20 -fcoroutines -I.. -Wall -Wextra -Werror -O2 -g -o test_wg_t test_wg_t.cpp
g++ -std=c++20 -fcoroutines -I.. -Wall -Wextra -Werror -O2 -g -o test_integration test_integration.cpp

# Special case: mutex test needs pthread
g++ -std=c++20 -fcoroutines -I.. -Wall -Wextra -Werror -O2 -g -o test_mutex_yield test_mutex_yield.cpp -pthread

# Run tests
./test_co_t
./test_chan_t
./test_wg_t
./test_integration
```

## Test Coverage

### Core Component Tests
- ✅ **`co_t`** - Basic coroutine creation, execution, state management, exception handling
- ✅ **`chan_t<T>`** - Channel operations, buffering, closure, type safety, error handling
- ✅ **`wg_t`** - Wait group synchronization, multiple tasks, dynamic addition, reuse
- ✅ **Integration** - Producer-consumer, multi-producer, pipeline, fan-out/fan-in patterns

### C++20 Coroutine Caveats
- ✅ **Top-level function restriction** - Compilation failures for non-coroutine functions
- ✅ **RAII preservation** - Resource management across suspension points
- ✅ **Reference/pointer danger** - Stack relocation and invalidation risks
- ✅ **Variable lifetime** - Automatic coroutine frame management
- ✅ **Advanced scenarios** - Smart pointers, lambda captures, containers

### Channel Behavior
- ✅ **Basic functionality** - Creation, read/write, closure
- ✅ **Edge cases** - Error conditions, boundary testing
- ✅ **Fair distribution** - Multiple consumer scenarios
- ✅ **Go compatibility** - Go channel semantics compliance
- ✅ **Stress testing** - High-load scenarios

### Wait Group Behavior
- ✅ **Simple scenarios** - Basic add/done/wait patterns
- ✅ **Multiple waiters** - Complex synchronization scenarios
- ✅ **Boundary testing** - Edge cases and limits

### Additional Features
- ✅ **Bug fixes** - Verification of resolved issues
- ✅ **Performance** - Fair distribution analysis
- ✅ **Memory management** - Weak pointer improvements
- ✅ **Scheduler** - Clearing and management functionality

## Key Features Tested

### Coroutine Behavior
- Proper suspension and resumption
- State preservation across suspension points
- Cooperative scheduling
- Memory management
- Exception handling

### Channel Communication
- Synchronous and asynchronous communication
- Buffering behavior (unbuffered and buffered channels)
- Channel closure semantics
- Type safety with different data types
- Go channel compatibility
- Fair distribution among multiple consumers

### Synchronization
- Wait group coordination
- Multiple waiter scenarios
- Dynamic task management
- Proper cleanup and resource management

### C++20 Coroutine Caveats
- Top-level function restrictions
- RAII preservation across suspensions
- Reference and pointer invalidation risks
- Variable lifetime management
- Stack relocation demonstration

## Expected Output

When all tests pass, you should see output similar to:

```
Running co_t tests...
========================
Testing basic coroutine creation and execution...
✓ Basic coroutine test passed
...
All co_t tests passed! ✓

Running chan_t tests...
========================
Testing basic channel creation and properties...
✓ Basic channel test passed
...
All chan_t tests passed! ✓

Running wg_t tests...
========================
Testing basic wait group creation and operations...
✓ Basic wait group test passed
...
All wg_t tests passed! ✓

Running integration tests...
=============================
Testing producer-consumer pattern...
✓ Producer-consumer test passed
...
All integration tests passed! ✓
```

## Notes

- Tests use assertions to verify correctness
- All tests are designed to be deterministic
- Integration tests demonstrate real-world usage patterns
- Tests document the current behavior of the library

## Contributing

When adding new tests:
1. Follow the existing naming conventions
2. Include comprehensive assertions
3. Add descriptive output messages
4. Update this README if adding new test categories
5. Ensure tests are deterministic and don't rely on timing
