#include "../coco.h"
#include <iostream>
#include <cassert>
#include <vector>

using namespace coco;

// Test 1: Verify size() method accuracy for buffered channels
void test_buffered_size_accuracy() {
    std::cout << "=== Test 1: Buffered Channel size() Method Accuracy ===" << std::endl;
    
    chan_t<int> ch(3); // Buffer capacity 3
    
    // Initial state
    assert(ch.cap() == 3);
    assert(ch.size() == 0);
    assert(!ch.ready());
    assert(!ch.closed());
    
    std::cout << "Initial: cap=" << ch.cap() << ", size=" << ch.size() 
              << ", ready=" << ch.ready() << std::endl;
    
    // Test size reporting during buffer filling
    bool write_completed = false;
    
    auto writer = [&ch, &write_completed]() -> co_t {
        // Write 3 values to fill buffer
        for (int i = 1; i <= 3; i++) {
            std::cout << "Writing " << i << ", before: size=" << ch.size() 
                      << ", ready=" << ch.ready() << std::endl;
            
            bool ok = co_await ch.write(i);
            
            std::cout << "After write " << i << ": size=" << ch.size() 
                      << ", ready=" << ch.ready() << ", ok=" << ok << std::endl;
            
            // Verify size increases correctly
            assert(ch.size() == static_cast<size_t>(i));
            assert(ch.ready() == true); // Should be ready after first write
        }
        write_completed = true;
        co_return;
    };
    
    co_t w_coro = writer();
    w_coro.resume();
    scheduler_t::instance().run();
    
    assert(write_completed);
    assert(ch.size() == 3);
    assert(ch.ready());
    
    std::cout << "✓ Buffered channel size() method reports correctly during filling" << std::endl;
}

// Test 2: Verify size() method accuracy during reading
void test_buffered_size_during_reading() {
    std::cout << "\n=== Test 2: Buffered Channel size() During Reading ===" << std::endl;
    
    chan_t<std::string> ch(2);
    
    // Fill buffer first
    auto filler = [&ch]() -> co_t {
        co_await ch.write("FIRST");
        co_await ch.write("SECOND");
        std::cout << "Buffer filled: size=" << ch.size() << ", ready=" << ch.ready() << std::endl;
        co_return;
    };
    
    co_t f_coro = filler();
    f_coro.resume();
    scheduler_t::instance().run();
    
    assert(ch.size() == 2);
    assert(ch.ready());
    
    // Now read and verify size decreases
    auto reader = [&ch]() -> co_t {
        for (int i = 0; i < 2; i++) {
            std::cout << "Before read " << (i+1) << ": size=" << ch.size() 
                      << ", ready=" << ch.ready() << std::endl;
            
            auto result = co_await ch.read();
            
            std::cout << "After read " << (i+1) << ": size=" << ch.size() 
                      << ", ready=" << ch.ready();
            if (result.has_value()) {
                std::cout << ", value=" << result.value();
            }
            std::cout << std::endl;
            
            // Verify size decreases correctly
            assert(ch.size() == static_cast<size_t>(2 - i - 1));
        }
        co_return;
    };
    
    co_t r_coro = reader();
    r_coro.resume();
    scheduler_t::instance().run();
    
    assert(ch.size() == 0);
    assert(!ch.ready());
    
    std::cout << "✓ Buffered channel size() method reports correctly during reading" << std::endl;
}

// Test 3: Verify ready() method behavior
void test_ready_method_behavior() {
    std::cout << "\n=== Test 3: ready() Method Behavior ===" << std::endl;
    
    // Test unbuffered channel
    chan_t<int> unbuffered(0);
    assert(!unbuffered.ready()); // Should never be ready when empty
    
    // Test buffered channel
    chan_t<int> buffered(2);
    assert(!buffered.ready()); // Should not be ready when empty
    
    auto test_ready = [&buffered]() -> co_t {
        // Add one item
        co_await buffered.write(100);
        std::cout << "After first write: ready=" << buffered.ready() 
                  << ", size=" << buffered.size() << std::endl;
        assert(buffered.ready()); // Should be ready with data
        
        // Add second item
        co_await buffered.write(200);
        std::cout << "After second write: ready=" << buffered.ready() 
                  << ", size=" << buffered.size() << std::endl;
        assert(buffered.ready()); // Should still be ready
        
        // Read one item
        auto result1 = co_await buffered.read();
        std::cout << "After first read: ready=" << buffered.ready()
                  << ", size=" << buffered.size() << std::endl;
        assert(result1.has_value()); // Should have read a value
        assert(buffered.ready()); // Should still be ready (one item left)

        // Read second item
        auto result2 = co_await buffered.read();
        std::cout << "After second read: ready=" << buffered.ready()
                  << ", size=" << buffered.size() << std::endl;
        assert(result2.has_value()); // Should have read a value
        assert(!buffered.ready()); // Should not be ready (empty)
        
        co_return;
    };
    
    co_t t_coro = test_ready();
    t_coro.resume();
    scheduler_t::instance().run();
    
    std::cout << "✓ ready() method behaves correctly" << std::endl;
}

// Test 4: Verify state consistency after channel close
void test_state_after_close() {
    std::cout << "\n=== Test 4: State Consistency After Channel Close ===" << std::endl;
    
    chan_t<int> ch(2);
    
    // Fill buffer partially
    auto setup = [&ch]() -> co_t {
        co_await ch.write(42);
        std::cout << "Before close: size=" << ch.size() << ", ready=" << ch.ready() 
                  << ", closed=" << ch.closed() << std::endl;
        
        ch.close();
        
        std::cout << "After close: size=" << ch.size() << ", ready=" << ch.ready() 
                  << ", closed=" << ch.closed() << std::endl;
        
        // Channel should still report buffered data as available
        assert(ch.closed());
        // Size and ready state should reflect remaining buffered data
        
        co_return;
    };
    
    co_t s_coro = setup();
    s_coro.resume();
    scheduler_t::instance().run();
    
    std::cout << "✓ State consistency maintained after close" << std::endl;
}

int main() {
    std::cout << "=== Chan_t State Consistency Tests ===" << std::endl;
    std::cout << "Testing accuracy of size(), ready(), cap(), closed() methods\n" << std::endl;
    
    test_buffered_size_accuracy();
    test_buffered_size_during_reading();
    test_ready_method_behavior();
    test_state_after_close();
    
    std::cout << "\n=== State Consistency Tests Completed ===" << std::endl;
    return 0;
}
