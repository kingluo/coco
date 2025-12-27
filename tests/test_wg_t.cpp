#include "../coco.h"
#include <iostream>
#include <cassert>

using namespace coco;

int main() {
    std::cout << "Running minimal wg_t tests..." << std::endl;
    std::cout << "========================" << std::endl;
    
    // Test 1: Basic wait group creation and operations
    {
        std::cout << "Testing basic wait group creation and operations..." << std::endl;
        
        wg_t wg;
        bool wait_completed = false;
        
        auto waiter = [&wg, &wait_completed]() -> co_t {
            co_await wg.wait();
            wait_completed = true;
            co_return;
        };
        
        co_t waiter_coro = waiter();
        waiter_coro.resume();
        scheduler_t::instance().run();

        // Since count is 0, wait should complete immediately
        assert(wait_completed == true);
        
        std::cout << "✓ Basic wait group test passed" << std::endl;
    }
    
    // Test 2: Wait group with add/done
    {
        std::cout << "Testing wait group with add/done..." << std::endl;
        
        wg_t wg;
        bool wait_completed = false;
        
        // Add one task to wait group
        wg.add(1);
        
        auto waiter = [&wg, &wait_completed]() -> co_t {
            co_await wg.wait();
            wait_completed = true;
            co_return;
        };
        
        co_t waiter_coro = waiter();
        waiter_coro.resume();
        scheduler_t::instance().run();

        // Wait should not complete yet (count is 1)
        assert(wait_completed == false);
        
        // Mark task as done
        wg.done();

        // Run scheduler to process the resumed waiter
        scheduler_t::instance().run();

        // Waiter should complete now
        assert(wait_completed == true);
        
        std::cout << "✓ Wait group add/done test passed" << std::endl;
    }
    
    // Test 3: Wait group with multiple add/done
    {
        std::cout << "Testing wait group with multiple add/done..." << std::endl;
        
        wg_t wg;
        bool wait_completed = false;
        
        // Add 3 tasks to wait group
        wg.add(3);
        
        auto waiter = [&wg, &wait_completed]() -> co_t {
            co_await wg.wait();
            wait_completed = true;
            co_return;
        };
        
        co_t waiter_coro = waiter();
        waiter_coro.resume();
        scheduler_t::instance().run();

        // Wait should not complete yet
        assert(wait_completed == false);
        
        // Complete tasks one by one
        wg.done();
        assert(wait_completed == false); // Still waiting for other tasks
        
        wg.done();
        assert(wait_completed == false); // Still waiting for last task
        
        wg.done();

        // Run scheduler to process the resumed waiter
        scheduler_t::instance().run();

        // Waiter should complete now
        assert(wait_completed == true);
        
        std::cout << "✓ Multiple add/done wait group test passed" << std::endl;
    }
    
    std::cout << "========================" << std::endl;
    std::cout << "All wg_t tests passed! ✓" << std::endl;
    
    return 0;
}
