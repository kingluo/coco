#include "../coco.h"
#include <iostream>
#include <cassert>
#include <vector>
#include <string>
#include <chrono>
#include <thread>

using namespace coco;

// Test 1: FIFO ordering for blocked readers on unbuffered channel
void test_unbuffered_blocked_readers_fifo() {
    std::cout << "=== Test 1: FIFO Ordering for Blocked Readers (Unbuffered) ===" << std::endl;
    
    chan_t<int> ch(0); // Unbuffered channel
    std::vector<std::string> reader_order;
    std::vector<int> received_values;
    bool all_readers_done = false;
    
    // Create 3 readers that will block in order
    auto reader1 = [&ch, &reader_order, &received_values]() -> co_t {
        std::cout << "Reader1: Starting and blocking..." << std::endl;
        auto result = co_await ch.read();
        if (result.has_value()) {
            reader_order.push_back("R1");
            received_values.push_back(result.value());
            std::cout << "Reader1: Received " << result.value() << std::endl;
        }
        co_return;
    };
    
    auto reader2 = [&ch, &reader_order, &received_values]() -> co_t {
        std::cout << "Reader2: Starting and blocking..." << std::endl;
        auto result = co_await ch.read();
        if (result.has_value()) {
            reader_order.push_back("R2");
            received_values.push_back(result.value());
            std::cout << "Reader2: Received " << result.value() << std::endl;
        }
        co_return;
    };
    
    auto reader3 = [&ch, &reader_order, &received_values]() -> co_t {
        std::cout << "Reader3: Starting and blocking..." << std::endl;
        auto result = co_await ch.read();
        if (result.has_value()) {
            reader_order.push_back("R3");
            received_values.push_back(result.value());
            std::cout << "Reader3: Received " << result.value() << std::endl;
        }
        co_return;
    };
    
    auto writer = [&ch, &all_readers_done]() -> co_t {
        std::cout << "Writer: Starting..." << std::endl;

        // Send 3 values to wake up readers in order
        std::cout << "Writer: About to send 100..." << std::endl;
        bool ok1 = co_await ch.write(100);
        std::cout << "Writer: Sent 100, ok=" << ok1 << std::endl;

        std::cout << "Writer: About to send 200..." << std::endl;
        bool ok2 = co_await ch.write(200);
        std::cout << "Writer: Sent 200, ok=" << ok2 << std::endl;

        std::cout << "Writer: About to send 300..." << std::endl;
        bool ok3 = co_await ch.write(300);
        std::cout << "Writer: Sent 300, ok=" << ok3 << std::endl;

        all_readers_done = true;
        std::cout << "Writer: Done" << std::endl;
        co_return;
    };
    
    // Start all coroutines
    co_t r1_coro = reader1();
    co_t r2_coro = reader2();
    co_t r3_coro = reader3();
    co_t w_coro = writer();

    r1_coro.resume();
    r2_coro.resume();
    r3_coro.resume();
    w_coro.resume();
    
    scheduler_t::instance().run();
    
    std::cout << "Reader execution order: ";
    for (const auto& r : reader_order) {
        std::cout << r << " ";
    }
    std::cout << std::endl;
    
    std::cout << "Received values: ";
    for (int v : received_values) {
        std::cout << v << " ";
    }
    std::cout << std::endl;
    
    // Check FIFO ordering - readers should be served in the order they blocked
    if (reader_order.size() == 3 && reader_order[0] == "R1" && reader_order[1] == "R2" && reader_order[2] == "R3") {
        std::cout << "✓ FIFO ordering maintained for blocked readers" << std::endl;
    } else {
        std::cout << "❌ FIFO ordering VIOLATED for blocked readers" << std::endl;
        std::cout << "   Expected: R1 R2 R3, Got: ";
        for (const auto& r : reader_order) std::cout << r << " ";
        std::cout << std::endl;
    }
    
    assert(all_readers_done);
    assert(received_values.size() == 3);
}

// Test 2: FIFO ordering for blocked writers on buffered channel
void test_buffered_blocked_writers_fifo() {
    std::cout << "\n=== Test 2: FIFO Ordering for Blocked Writers (Buffered) ===" << std::endl;
    
    chan_t<std::string> ch(1); // Buffer size 1
    std::vector<std::string> writer_order;
    std::vector<std::string> written_values;
    bool all_writers_done = false;
    
    // Fill the buffer first
    auto buffer_filler = [&ch]() -> co_t {
        bool ok = co_await ch.write("BUFFER");
        std::cout << "Buffer filled with 'BUFFER', ok=" << ok << std::endl;
        co_return;
    };
    
    // Create 3 writers that will block in order
    auto writer1 = [&ch, &writer_order, &written_values]() -> co_t {
        std::cout << "Writer1: Attempting to write (will block)..." << std::endl;
        bool ok = co_await ch.write("W1_DATA");
        if (ok) {
            writer_order.push_back("W1");
            written_values.push_back("W1_DATA");
            std::cout << "Writer1: Write completed" << std::endl;
        }
        co_return;
    };
    
    auto writer2 = [&ch, &writer_order, &written_values]() -> co_t {
        std::cout << "Writer2: Attempting to write (will block)..." << std::endl;
        bool ok = co_await ch.write("W2_DATA");
        if (ok) {
            writer_order.push_back("W2");
            written_values.push_back("W2_DATA");
            std::cout << "Writer2: Write completed" << std::endl;
        }
        co_return;
    };
    
    auto writer3 = [&ch, &writer_order, &written_values]() -> co_t {
        std::cout << "Writer3: Attempting to write (will block)..." << std::endl;
        bool ok = co_await ch.write("W3_DATA");
        if (ok) {
            writer_order.push_back("W3");
            written_values.push_back("W3_DATA");
            std::cout << "Writer3: Write completed" << std::endl;
        }
        co_return;
    };
    
    auto reader = [&ch, &all_writers_done]() -> co_t {
        std::cout << "Reader: Starting to drain channel..." << std::endl;

        // Read 4 values to unblock writers in order
        for (int i = 0; i < 4; i++) {
            auto result = co_await ch.read();
            if (result.has_value()) {
                std::cout << "Reader: Read '" << result.value() << "'" << std::endl;
            }
        }

        all_writers_done = true;
        std::cout << "Reader: Done" << std::endl;
        co_return;
    };
    
    // Start all coroutines
    co_t bf_coro = buffer_filler();
    co_t w1_coro = writer1();
    co_t w2_coro = writer2();
    co_t w3_coro = writer3();
    co_t r_coro = reader();
    
    bf_coro.resume();
    w1_coro.resume();
    w2_coro.resume();
    w3_coro.resume();
    r_coro.resume();
    
    scheduler_t::instance().run();
    
    std::cout << "Writer completion order: ";
    for (const auto& w : writer_order) {
        std::cout << w << " ";
    }
    std::cout << std::endl;
    
    // Check FIFO ordering - writers should complete in the order they blocked
    if (writer_order.size() == 3 && writer_order[0] == "W1" && writer_order[1] == "W2" && writer_order[2] == "W3") {
        std::cout << "✓ FIFO ordering maintained for blocked writers" << std::endl;
    } else {
        std::cout << "❌ FIFO ordering VIOLATED for blocked writers" << std::endl;
        std::cout << "   Expected: W1 W2 W3, Got: ";
        for (const auto& w : writer_order) std::cout << w << " ";
        std::cout << std::endl;
    }
    
    assert(all_writers_done);
}

int main() {
    std::cout << "=== Chan_t FIFO Ordering Validation Tests ===" << std::endl;
    std::cout << "Testing compliance with Go channel FIFO guarantees for blocked operations\n" << std::endl;
    
    test_unbuffered_blocked_readers_fifo();
    test_buffered_blocked_writers_fifo();
    
    std::cout << "\n=== FIFO Ordering Tests Completed ===" << std::endl;
    return 0;
}
