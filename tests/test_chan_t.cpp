#include "../coco.h"
#include <iostream>
#include <cassert>
#include <vector>

using namespace coco;

int main() {
    std::cout << "Running chan_t tests..." << std::endl;
    std::cout << "========================" << std::endl;
    
    // Test 1: Basic channel creation and properties
    {
        std::cout << "Testing basic channel creation and properties..." << std::endl;
        
        // Test unbuffered channel
        chan_t<int> unbuffered_ch(0);
        assert(unbuffered_ch.cap() == 0);
        assert(unbuffered_ch.size() == 0);
        assert(!unbuffered_ch.ready());
        assert(!unbuffered_ch.closed());
        
        // Test buffered channel
        chan_t<int> buffered_ch(5);
        assert(buffered_ch.cap() == 5);
        assert(buffered_ch.size() == 0);
        assert(!buffered_ch.ready());
        assert(!buffered_ch.closed());
        
        std::cout << "✓ Basic channel test passed" << std::endl;
    }
    
    // Test 2: Simple channel read/write operations
    {
        std::cout << "Testing channel read/write operations..." << std::endl;
        
        chan_t<int> ch(2); // Buffered channel with capacity 2
        bool write_completed = false;
        
        // Writer coroutine
        auto writer = [&ch, &write_completed]() -> co_t {
            // Write first value (should not block due to buffer)
            auto write_result1 = co_await ch.write(42);
            assert(write_result1 == true);
            
            // Write second value (should not block due to buffer)
            auto write_result2 = co_await ch.write(84);
            assert(write_result2 == true);
            
            write_completed = true;
            co_return;
        };
        
        // Execute writer
        co_t writer_coro = writer();
        writer_coro.resume();
        scheduler_t::instance().run();

        // Both writes should complete due to buffering (capacity = 2)
        assert(write_completed == true);
        assert(ch.size() == 2);
        assert(ch.ready() == true);
        
        std::cout << "✓ Channel read/write test passed" << std::endl;
    }
    
    // Test 3: Channel closure
    {
        std::cout << "Testing channel closure..." << std::endl;

        chan_t<int> ch(1);
        bool test_completed = false;

        // Write a value then close
        auto test_coro = [&ch, &test_completed]() -> co_t {
            auto write_result = co_await ch.write(100);
            assert(write_result == true);

            ch.close();
            assert(ch.closed() == true);

            // Try to write to closed channel - should return false
            auto write_result2 = co_await ch.write(200);
            assert(write_result2 == false);

            test_completed = true;
            co_return;
        };

        co_t coro = test_coro();
        coro.resume();
        scheduler_t::instance().run();

        assert(test_completed == true);

        std::cout << "✓ Channel closure test passed" << std::endl;
    }

    // Test 4: Channel with different data types
    {
        std::cout << "Testing channel with different data types..." << std::endl;

        // Test with string
        chan_t<std::string> string_ch(1);
        bool string_test_completed = false;

        auto string_test = [&string_ch, &string_test_completed]() -> co_t {
            auto write_result = co_await string_ch.write(std::string("Hello World"));
            assert(write_result == true);

            auto read_result = co_await string_ch.read();
            assert(read_result.has_value());
            assert(read_result.value() == "Hello World");

            string_test_completed = true;
            co_return;
        };

        co_t string_coro = string_test();
        string_coro.resume();
        scheduler_t::instance().run();

        assert(string_test_completed == true);

        std::cout << "✓ Channel different types test passed" << std::endl;
    }
    
    std::cout << "========================" << std::endl;
    std::cout << "All chan_t tests passed! ✓" << std::endl;
    
    return 0;
}
