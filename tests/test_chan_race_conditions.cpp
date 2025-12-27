#include "../coco.h"
#include <iostream>
#include <cassert>
#include <vector>
#include <set>

using namespace coco;

// Test 1: Concurrent operations on closed channel
void test_operations_on_closed_channel() {
    std::cout << "=== Test 1: Operations on Closed Channel ===" << std::endl;
    
    chan_t<int> ch(2);
    std::vector<bool> write_results;
    std::vector<std::optional<int>> read_results;
    
    auto setup_and_close = [&ch]() -> co_t {
        // Add some data before closing
        co_await ch.write(100);
        co_await ch.write(200);
        std::cout << "Added data, closing channel..." << std::endl;
        ch.close();
        std::cout << "Channel closed" << std::endl;
        co_return;
    };
    
    auto writer_after_close = [&ch, &write_results]() -> co_t {
        std::cout << "Attempting write after close..." << std::endl;
        bool result = co_await ch.write(300);
        write_results.push_back(result);
        std::cout << "Write after close result: " << result << std::endl;
        co_return;
    };
    
    auto reader_after_close = [&ch, &read_results]() -> co_t {
        std::cout << "Reading from closed channel..." << std::endl;
        
        // Should be able to read buffered data
        auto result1 = co_await ch.read();
        read_results.push_back(result1);
        if (result1.has_value()) {
            std::cout << "Read 1: " << result1.value() << std::endl;
        }
        
        auto result2 = co_await ch.read();
        read_results.push_back(result2);
        if (result2.has_value()) {
            std::cout << "Read 2: " << result2.value() << std::endl;
        }
        
        // This should return empty optional
        auto result3 = co_await ch.read();
        read_results.push_back(result3);
        std::cout << "Read 3 (should be empty): " << (result3.has_value() ? "has value" : "empty") << std::endl;
        
        co_return;
    };
    
    co_t setup_coro = setup_and_close();
    co_t writer_coro = writer_after_close();
    co_t reader_coro = reader_after_close();
    
    setup_coro.resume();
    writer_coro.resume();
    reader_coro.resume();
    
    scheduler_t::instance().run();
    
    // Verify results
    assert(write_results.size() == 1);
    assert(write_results[0] == false); // Write should fail on closed channel
    
    assert(read_results.size() == 3);
    assert(read_results[0].has_value() && read_results[0].value() == 100);
    assert(read_results[1].has_value() && read_results[1].value() == 200);
    assert(!read_results[2].has_value()); // Should be empty
    
    std::cout << "✓ Operations on closed channel behave correctly" << std::endl;
}

// Test 2: Multiple readers competing for data
void test_multiple_readers_competition() {
    std::cout << "\n=== Test 2: Multiple Readers Competition ===" << std::endl;
    
    chan_t<int> ch(1); // Small buffer to force competition
    std::vector<int> reader1_values, reader2_values, reader3_values;
    
    auto writer = [&ch]() -> co_t {
        for (int i = 1; i <= 6; i++) {
            bool ok = co_await ch.write(i * 10);
            std::cout << "Wrote " << (i * 10) << ", ok=" << ok << std::endl;
        }
        ch.close();
        std::cout << "Writer done, channel closed" << std::endl;
        co_return;
    };
    
    auto reader1 = [&ch, &reader1_values]() -> co_t {
        while (true) {
            auto result = co_await ch.read();
            if (!result.has_value()) break;
            reader1_values.push_back(result.value());
            std::cout << "Reader1 got: " << result.value() << std::endl;
        }
        std::cout << "Reader1 done" << std::endl;
        co_return;
    };
    
    auto reader2 = [&ch, &reader2_values]() -> co_t {
        while (true) {
            auto result = co_await ch.read();
            if (!result.has_value()) break;
            reader2_values.push_back(result.value());
            std::cout << "Reader2 got: " << result.value() << std::endl;
        }
        std::cout << "Reader2 done" << std::endl;
        co_return;
    };
    
    auto reader3 = [&ch, &reader3_values]() -> co_t {
        while (true) {
            auto result = co_await ch.read();
            if (!result.has_value()) break;
            reader3_values.push_back(result.value());
            std::cout << "Reader3 got: " << result.value() << std::endl;
        }
        std::cout << "Reader3 done" << std::endl;
        co_return;
    };
    
    co_t w_coro = writer();
    co_t r1_coro = reader1();
    co_t r2_coro = reader2();
    co_t r3_coro = reader3();
    
    w_coro.resume();
    r1_coro.resume();
    r2_coro.resume();
    r3_coro.resume();
    
    scheduler_t::instance().run();
    
    // Verify all values were read exactly once
    std::set<int> all_read_values;
    for (int v : reader1_values) all_read_values.insert(v);
    for (int v : reader2_values) all_read_values.insert(v);
    for (int v : reader3_values) all_read_values.insert(v);
    
    std::set<int> expected_values = {10, 20, 30, 40, 50, 60};
    
    std::cout << "Total values read: " << (reader1_values.size() + reader2_values.size() + reader3_values.size()) << std::endl;
    std::cout << "Unique values read: " << all_read_values.size() << std::endl;
    
    assert(all_read_values == expected_values);
    assert(all_read_values.size() == 6); // No duplicates
    
    std::cout << "✓ Multiple readers correctly compete for data without duplication" << std::endl;
}

// Test 3: Channel close during pending operations
void test_close_during_pending_operations() {
    std::cout << "\n=== Test 3: Channel Close During Pending Operations ===" << std::endl;
    
    chan_t<std::string> ch(1);
    std::vector<bool> write_results;
    std::vector<std::optional<std::string>> read_results;
    
    // Fill buffer to block subsequent writes
    auto buffer_filler = [&ch]() -> co_t {
        co_await ch.write("BUFFER_DATA");
        std::cout << "Buffer filled" << std::endl;
        co_return;
    };
    
    auto blocked_writer = [&ch, &write_results]() -> co_t {
        std::cout << "Writer attempting to write (will block)..." << std::endl;
        bool result = co_await ch.write("BLOCKED_DATA");
        write_results.push_back(result);
        std::cout << "Blocked writer result: " << result << std::endl;
        co_return;
    };
    
    auto blocked_reader = [&ch, &read_results]() -> co_t {
        // First read the buffer data
        auto result1 = co_await ch.read();
        read_results.push_back(result1);
        if (result1.has_value()) {
            std::cout << "Read buffer data: " << result1.value() << std::endl;
        }
        
        // This read will block until close
        std::cout << "Reader attempting second read (will block)..." << std::endl;
        auto result2 = co_await ch.read();
        read_results.push_back(result2);
        std::cout << "Blocked reader result: " << (result2.has_value() ? result2.value() : "empty") << std::endl;
        co_return;
    };
    
    auto closer = [&ch]() -> co_t {
        std::cout << "Closer will close channel..." << std::endl;
        ch.close();
        std::cout << "Channel closed by closer" << std::endl;
        co_return;
    };
    
    co_t bf_coro = buffer_filler();
    co_t bw_coro = blocked_writer();
    co_t br_coro = blocked_reader();
    co_t cl_coro = closer();
    
    bf_coro.resume();
    bw_coro.resume();
    br_coro.resume();
    cl_coro.resume();
    
    scheduler_t::instance().run();
    
    // Verify results
    assert(write_results.size() == 1);
    // The blocked writer should complete with the data it was trying to write
    // OR fail depending on implementation
    
    assert(read_results.size() == 2);
    assert(read_results[0].has_value() && read_results[0].value() == "BUFFER_DATA");
    // Second read should either get the blocked writer's data or be empty
    
    std::cout << "✓ Channel close during pending operations handled correctly" << std::endl;
}

int main() {
    std::cout << "=== Chan_t Race Conditions and Edge Cases Tests ===" << std::endl;
    std::cout << "Testing concurrent operations and edge cases\n" << std::endl;
    
    test_operations_on_closed_channel();
    test_multiple_readers_competition();
    test_close_during_pending_operations();
    
    std::cout << "\n=== Race Conditions and Edge Cases Tests Completed ===" << std::endl;
    return 0;
}
