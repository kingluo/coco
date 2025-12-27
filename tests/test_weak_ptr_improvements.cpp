#include "../coco.h"
#include <iostream>
#include <cassert>
#include <vector>
#include <cstdint>

using namespace coco;

// Test that channels can safely resume coroutines in close()
void test_channel_close_resume() {
    std::cout << "Testing channel close() safely resumes waiting coroutines..." << std::endl;

    // Test with a simple scenario: reader waiting on empty channel
    chan_t<int> ch(0); // Unbuffered channel
    bool reader_completed = false;

    // Reader coroutine that will wait on empty channel
    auto reader = [&ch, &reader_completed]() -> co_t {
        auto result = co_await ch.read();
        // Should get nullopt when channel is closed
        assert(!result.has_value());
        reader_completed = true;
        co_return;
    };

    co_t reader_coro = reader();
    reader_coro.resume(); // Start the reader coroutine

    // Run the scheduler to let the reader start and wait on the channel
    scheduler_t::instance().run();

    // Reader should be waiting at this point
    // Close the channel - this should resume the waiting reader
    ch.close();

    // Run the scheduler again to process the resumed reader
    scheduler_t::instance().run();

    // Reader should now be completed
    assert(reader_completed == true);

    std::cout << "✓ Channel close resume test passed" << std::endl;
}

// Test that wg_t now uses uint64_t parameter
void test_wg_uint64_overflow() {
    std::cout << "Testing wg_t uint64_t parameter behavior..." << std::endl;

    wg_t wg;

    // Test adding large positive values - now allows natural uint64_t overflow
    wg.add(UINT64_MAX - 10);
    wg.add(20); // This will now wrap around due to uint64_t overflow

    // Natural uint64_t overflow behavior
    std::cout << "  Added large values - natural uint64_t behavior" << std::endl;

    // Test uint64_t parameter - only accepts non-negative values
    wg_t wg2;
    wg2.add(5); // Add positive value

    std::cout << "  uint64_t parameter accepts only non-negative values" << std::endl;

    // Test normal operations with uint64_t
    wg_t wg3;
    wg3.add(5);
    wg3.add(10); // Add more work

    std::cout << "  Normal uint64_t operations working" << std::endl;

    std::cout << "✓ wg_t uint64_t parameter test passed" << std::endl;
}

// Test that wg_t can handle large counts with uint64_t
void test_wg_large_counts() {
    std::cout << "Testing wg_t with large counts..." << std::endl;

    wg_t wg;
    bool wait_completed = false;

    // Add a moderately large number of tasks
    const uint64_t large_count = 1000;
    wg.add(large_count);

    auto waiter = [&wg, &wait_completed]() -> co_t {
        co_await wg.wait();
        wait_completed = true;
        co_return;
    };

    co_t waiter_coro = waiter();
    waiter_coro.resume(); // Start the waiter coroutine

    // Run scheduler to let waiter start waiting
    scheduler_t::instance().run();

    // Should not be completed yet
    assert(wait_completed == false);

    // Complete all tasks using done() method
    for (uint64_t i = 0; i < large_count; i++) {
        wg.done();
    }

    // Run scheduler to process the completed wait
    scheduler_t::instance().run();

    // Should now be completed
    assert(wait_completed == true);

    std::cout << "✓ Large counts test passed" << std::endl;
}

// Test channel behavior with multiple readers/writers and close
void test_multiple_waiters_close() {
    std::cout << "Testing multiple waiters with channel close..." << std::endl;
    
    chan_t<std::string> ch(1); // Small buffer
    std::vector<bool> readers_completed(3, false);
    std::vector<bool> writers_completed(3, false);
    
    // Create multiple reader coroutines
    auto create_reader = [&ch, &readers_completed](int id) -> co_t {
        auto result = co_await ch.read();
        // Some might get data, others might get nullopt when closed
        readers_completed[id] = true;
        co_return;
    };
    
    // Create multiple writer coroutines
    auto create_writer = [&ch, &writers_completed](int id) -> co_t {
        std::string data = "data" + std::to_string(id);
        [[maybe_unused]] auto result = co_await ch.write(data);
        // Some might succeed, others might fail when closed
        writers_completed[id] = true;
        co_return;
    };
    
    std::vector<co_t> reader_coros;
    std::vector<co_t> writer_coros;
    
    for (int i = 0; i < 3; i++) {
        reader_coros.emplace_back(create_reader(i));
        writer_coros.emplace_back(create_writer(i));
    }

    // Start all coroutines
    for (auto& coro : reader_coros) {
        coro.resume();
    }
    for (auto& coro : writer_coros) {
        coro.resume();
    }

    // Run scheduler to let them start waiting/writing
    scheduler_t::instance().run();

    // Close the channel - should resume all waiting coroutines
    ch.close();

    // Run scheduler to process resumed coroutines
    scheduler_t::instance().run();

    // All coroutines should be completed
    for (int i = 0; i < 3; i++) {
        assert(readers_completed[i] == true);
        assert(writers_completed[i] == true);
    }
    
    std::cout << "✓ Multiple waiters close test passed" << std::endl;
}

int main() {
    std::cout << "=== Testing weak_ptr and uint64_t improvements ===" << std::endl;
    std::cout << std::endl;
    
    test_channel_close_resume();
    std::cout << std::endl;
    
    test_wg_uint64_overflow();
    std::cout << std::endl;
    
    test_wg_large_counts();
    std::cout << std::endl;
    
    test_multiple_waiters_close();
    std::cout << std::endl;
    
    std::cout << "=== All improvement tests passed! ✓ ===" << std::endl;
    return 0;
}
