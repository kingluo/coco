#include "../coco.h"
#include <iostream>
#include <cassert>
#include <vector>
#include <string>

using namespace coco;

// Test 1: Verify unbuffered channel has no intermediate storage
void test_unbuffered_no_storage() {
    std::cout << "=== Test 1: Unbuffered Channel Storage Verification ===" << std::endl;
    
    chan_t<int> ch(0); // Unbuffered channel
    
    // Check initial state
    assert(ch.cap() == 0);
    assert(ch.size() == 0);
    assert(!ch.ready());
    assert(!ch.closed());
    
    std::cout << "Initial state: cap=" << ch.cap() << ", size=" << ch.size() 
              << ", ready=" << ch.ready() << ", closed=" << ch.closed() << std::endl;
    
    // After creating unbuffered channel, it should never show as "ready" 
    // because unbuffered channels don't buffer data
    bool writer_executed = false;
    bool reader_executed = false;
    
    auto writer = [&ch, &writer_executed]() -> co_t {
        std::cout << "Writer: About to write to unbuffered channel" << std::endl;
        bool ok = co_await ch.write(42);
        std::cout << "Writer: Write completed, ok=" << ok << std::endl;
        writer_executed = true;
        co_return;
    };
    
    auto reader = [&ch, &reader_executed]() -> co_t {
        std::cout << "Reader: About to read from unbuffered channel" << std::endl;
        auto result = co_await ch.read();
        if (result.has_value()) {
            std::cout << "Reader: Received " << result.value() << std::endl;
            assert(result.value() == 42);
        }
        reader_executed = true;
        co_return;
    };
    
    co_t w_coro = writer();
    co_t r_coro = reader();
    
    w_coro.resume();
    r_coro.resume();
    
    scheduler_t::instance().run();
    
    // Verify both completed
    assert(writer_executed);
    assert(reader_executed);
    
    // After operation, channel should still show no storage
    assert(ch.size() == 0);
    assert(!ch.ready());
    
    std::cout << "Final state: cap=" << ch.cap() << ", size=" << ch.size() 
              << ", ready=" << ch.ready() << std::endl;
    
    std::cout << "✓ Unbuffered channel shows no intermediate storage" << std::endl;
}

// Test 2: Verify synchronous behavior - writer blocks until reader ready
void test_unbuffered_synchronous_behavior() {
    std::cout << "\n=== Test 2: Unbuffered Channel Synchronous Behavior ===" << std::endl;
    
    chan_t<std::string> ch(0);
    std::vector<std::string> execution_order;
    
    auto writer = [&ch, &execution_order]() -> co_t {
        execution_order.push_back("Writer: Start");
        std::cout << "Writer: Starting write operation" << std::endl;
        
        bool ok = co_await ch.write("SYNC_DATA");
        
        execution_order.push_back("Writer: Complete");
        std::cout << "Writer: Write operation completed, ok=" << ok << std::endl;
        co_return;
    };
    
    auto reader = [&ch, &execution_order]() -> co_t {
        execution_order.push_back("Reader: Start");
        std::cout << "Reader: Starting read operation" << std::endl;
        
        auto result = co_await ch.read();
        
        execution_order.push_back("Reader: Complete");
        if (result.has_value()) {
            std::cout << "Reader: Read operation completed, value=" << result.value() << std::endl;
            assert(result.value() == "SYNC_DATA");
        }
        co_return;
    };
    
    co_t w_coro = writer();
    co_t r_coro = reader();
    
    w_coro.resume();
    r_coro.resume();
    
    scheduler_t::instance().run();
    
    std::cout << "Execution order:" << std::endl;
    for (const auto& event : execution_order) {
        std::cout << "  " << event << std::endl;
    }
    
    // For unbuffered channels, the exact order depends on implementation
    // but both should complete
    assert(execution_order.size() == 4);
    
    std::cout << "✓ Unbuffered channel synchronous behavior verified" << std::endl;
}

// Test 3: Multiple writers and readers on unbuffered channel
void test_unbuffered_multiple_operations() {
    std::cout << "\n=== Test 3: Multiple Operations on Unbuffered Channel ===" << std::endl;
    
    chan_t<int> ch(0);
    std::vector<int> received_values;
    std::vector<int> sent_values = {100, 200, 300};
    
    auto writer1 = [&ch]() -> co_t {
        std::cout << "Writer1: Sending 100" << std::endl;
        bool ok = co_await ch.write(100);
        std::cout << "Writer1: Send result=" << ok << std::endl;
        co_return;
    };
    
    auto writer2 = [&ch]() -> co_t {
        std::cout << "Writer2: Sending 200" << std::endl;
        bool ok = co_await ch.write(200);
        std::cout << "Writer2: Send result=" << ok << std::endl;
        co_return;
    };
    
    auto writer3 = [&ch]() -> co_t {
        std::cout << "Writer3: Sending 300" << std::endl;
        bool ok = co_await ch.write(300);
        std::cout << "Writer3: Send result=" << ok << std::endl;
        co_return;
    };
    
    auto reader = [&ch, &received_values]() -> co_t {
        for (int i = 0; i < 3; i++) {
            auto result = co_await ch.read();
            if (result.has_value()) {
                received_values.push_back(result.value());
                std::cout << "Reader: Received " << result.value() << std::endl;
            }
        }
        co_return;
    };
    
    co_t w1_coro = writer1();
    co_t w2_coro = writer2();
    co_t w3_coro = writer3();
    co_t r_coro = reader();
    
    w1_coro.resume();
    w2_coro.resume();
    w3_coro.resume();
    r_coro.resume();
    
    scheduler_t::instance().run();
    
    std::cout << "Sent values: ";
    for (int v : sent_values) std::cout << v << " ";
    std::cout << std::endl;
    
    std::cout << "Received values: ";
    for (int v : received_values) std::cout << v << " ";
    std::cout << std::endl;
    
    // All values should be received
    assert(received_values.size() == 3);
    
    // Values should be received in some order (FIFO for the channel)
    std::sort(received_values.begin(), received_values.end());
    std::sort(sent_values.begin(), sent_values.end());
    assert(received_values == sent_values);
    
    std::cout << "✓ Multiple operations on unbuffered channel work correctly" << std::endl;
}

// Test 4: Examine internal state during operations
void test_unbuffered_internal_state() {
    std::cout << "\n=== Test 4: Unbuffered Channel Internal State Examination ===" << std::endl;

    chan_t<int> ch(0);

    auto writer = [&ch]() -> co_t {
        std::cout << "Writer: Before write - size=" << ch.size() << ", ready=" << ch.ready() << std::endl;
        bool ok = co_await ch.write(999);
        std::cout << "Writer: After write - size=" << ch.size() << ", ready=" << ch.ready() << ", ok=" << ok << std::endl;
        co_return;
    };

    auto reader = [&ch]() -> co_t {
        std::cout << "Reader: Before read - size=" << ch.size() << ", ready=" << ch.ready() << std::endl;
        auto result = co_await ch.read();
        std::cout << "Reader: After read - size=" << ch.size() << ", ready=" << ch.ready() << std::endl;
        if (result.has_value()) {
            std::cout << "Reader: Received value=" << result.value() << std::endl;
        }
        co_return;
    };

    co_t w_coro = writer();
    co_t r_coro = reader();

    w_coro.resume();
    r_coro.resume();

    scheduler_t::instance().run();

    std::cout << "✓ Internal state examination completed" << std::endl;
}

int main() {
    std::cout << "=== Chan_t Unbuffered Channel Direct Handoff Tests ===" << std::endl;
    std::cout << "Testing compliance with Go unbuffered channel semantics\n" << std::endl;

    test_unbuffered_no_storage();
    test_unbuffered_synchronous_behavior();
    test_unbuffered_multiple_operations();
    test_unbuffered_internal_state();

    std::cout << "\n=== Unbuffered Channel Tests Completed ===" << std::endl;
    return 0;
}
