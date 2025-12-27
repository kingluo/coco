#include "../coco.h"
#include <iostream>
#include <cassert>
#include <vector>

using namespace coco;

// Test 1: Basic functionality
bool test_basic_functionality() {
    std::cout << "=== Test 1: Basic Functionality ===" << std::endl;
    
    chan_t<int> ch(2);
    
    // Test initial state
    if (ch.cap() != 2 || ch.size() != 0 || ch.ready() || ch.closed()) {
        std::cout << "❌ Basic state test failed" << std::endl;
        return false;
    }
    
    std::cout << "✓ Basic functionality test passed" << std::endl;
    return true;
}

// Test 2: Unbuffered channel
bool test_unbuffered_channel() {
    std::cout << "=== Test 2: Unbuffered Channel ===" << std::endl;

    chan_t<int> ch(0);
    bool writer_done = false;
    bool reader_done = false;
    int received = 0;

    // Create coroutines at the same scope level to avoid nested coroutine issues
    auto writer = [&ch, &writer_done]() -> co_t {
        bool ok = co_await ch.write(42);
        writer_done = (ok == true);
        co_return;
    };

    auto reader = [&ch, &reader_done, &received]() -> co_t {
        auto result = co_await ch.read();
        if (result.has_value()) {
            received = result.value();
            reader_done = true;
        }
        co_return;
    };

    co_t w = writer();
    co_t r = reader();

    // Start reader first for unbuffered channel
    r.resume();
    scheduler_t::instance().run();

    // Then start writer
    w.resume();
    scheduler_t::instance().run();

    if (writer_done && reader_done && received == 42) {
        std::cout << "✓ Unbuffered channel test passed" << std::endl;
        return true;
    } else {
        std::cout << "❌ Unbuffered channel test failed (writer_done=" << writer_done
                  << ", reader_done=" << reader_done << ", received=" << received << ")" << std::endl;
        return false;
    }
}

// Test 3: Buffered channel normal operation
bool test_buffered_normal() {
    std::cout << "=== Test 3: Buffered Channel Normal ===" << std::endl;
    
    chan_t<int> ch(3);
    std::vector<int> values;
    bool success = false;
    
    auto test_coro = [&ch, &values, &success]() -> co_t {
        // Write values
        for (int i = 1; i <= 3; i++) {
            bool ok = co_await ch.write(i);
            if (!ok) co_return;
        }
        
        // Read values
        for (int i = 0; i < 3; i++) {
            auto result = co_await ch.read();
            if (!result.has_value()) co_return;
            values.push_back(result.value());
        }
        
        success = true;
        co_return;
    };
    
    co_t coro = test_coro();
    coro.resume();
    scheduler_t::instance().run();
    
    if (success && values.size() == 3 && values[0] == 1 && values[1] == 2 && values[2] == 3) {
        std::cout << "✓ Buffered normal operation test passed" << std::endl;
        return true;
    } else {
        std::cout << "❌ Buffered normal operation test failed" << std::endl;
        return false;
    }
}

// Test 4: Channel closure
bool test_channel_closure() {
    std::cout << "=== Test 4: Channel Closure ===" << std::endl;
    
    chan_t<int> ch(2);
    std::vector<int> values;
    bool write_after_close_failed = false;
    bool success = false;
    
    auto test_coro = [&ch, &values, &write_after_close_failed, &success]() -> co_t {
        // Write some values
        bool ok1 = co_await ch.write(1);
        bool ok2 = co_await ch.write(2);
        if (!ok1 || !ok2) co_return;
        
        // Close channel
        ch.close();
        
        // Try to write after close
        bool ok3 = co_await ch.write(3);
        write_after_close_failed = !ok3;
        
        // Read remaining values
        while (true) {
            auto result = co_await ch.read();
            if (!result.has_value()) break;
            values.push_back(result.value());
        }
        
        success = true;
        co_return;
    };
    
    co_t coro = test_coro();
    coro.resume();
    scheduler_t::instance().run();
    
    std::cout << "Values read after close: ";
    for (int val : values) {
        std::cout << val << " ";
    }
    std::cout << "(count: " << values.size() << ")" << std::endl;
    
    if (success && write_after_close_failed) {
        std::cout << "✓ Channel closure test passed" << std::endl;
        std::cout << "  Note: " << values.size() << " values were readable after close" << std::endl;
        return true;
    } else {
        std::cout << "❌ Channel closure test failed" << std::endl;
        return false;
    }
}

int main() {
    std::cout << "Running comprehensive chan_t tests..." << std::endl;
    std::cout << "=====================================" << std::endl;
    
    int passed = 0;
    int total = 4;
    
    if (test_basic_functionality()) passed++;
    if (test_unbuffered_channel()) passed++;
    if (test_buffered_normal()) passed++;
    if (test_channel_closure()) passed++;
    
    std::cout << "=====================================" << std::endl;
    std::cout << "Test Results: " << passed << "/" << total << " passed" << std::endl;
    
    if (passed == total) {
        std::cout << "✅ All tests passed!" << std::endl;
        return 0;
    } else {
        std::cout << "❌ Some tests failed!" << std::endl;
        return 1;
    }
}
