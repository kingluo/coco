#include "../coco.h"
#include <iostream>
#include <cassert>
#include <vector>

using namespace coco;

int main() {
    std::cout << "Running wg_t tests..." << std::endl;
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
    
    // Test 2: Wait group with single task
    {
        std::cout << "Testing wait group with single task..." << std::endl;
        
        wg_t wg;
        bool task_completed = false;
        bool wait_completed = false;
        
        // Add one task to wait group
        wg.add(1);
        
        auto task = [&wg, &task_completed]() -> co_t {
            co_await std::suspend_always{}; // Simulate some work
            task_completed = true;
            wg.done(); // Mark task as done
            co_return;
        };
        
        auto waiter = [&wg, &wait_completed]() -> co_t {
            co_await wg.wait();
            wait_completed = true;
            co_return;
        };
        
        co_t task_coro = task();
        co_t waiter_coro = waiter();
        
        // Start waiter first
        waiter_coro.resume();
        scheduler_t::instance().run();

        // Wait should not complete yet
        assert(wait_completed == false);
        assert(task_completed == false);
        
        // Complete the task (first resume - executes until suspend)
        task_coro.resume();
        scheduler_t::instance().run();
        assert(task_completed == false); // Still suspended

        // Second resume - completes the task
        task_coro.resume();
        scheduler_t::instance().run();
        assert(task_completed == true);

        // Waiter should complete automatically when wg.done() is called
        assert(wait_completed == true);
        
        std::cout << "✓ Single task wait group test passed" << std::endl;
    }
    
    // Test 3: Wait group with multiple tasks
    {
        std::cout << "Testing wait group with multiple tasks..." << std::endl;
        
        wg_t wg;
        std::vector<bool> tasks_completed(3, false);
        bool wait_completed = false;
        
        // Add 3 tasks to wait group
        wg.add(3);
        
        auto create_task = [&wg, &tasks_completed](int task_id) -> co_t {
            co_await std::suspend_always{}; // Simulate some work
            tasks_completed[task_id] = true;
            wg.done(); // Mark task as done
            co_return;
        };
        
        auto waiter = [&wg, &wait_completed]() -> co_t {
            co_await wg.wait();
            wait_completed = true;
            co_return;
        };
        
        // Create task coroutines
        co_t task1 = create_task(0);
        co_t task2 = create_task(1);
        co_t task3 = create_task(2);
        co_t waiter_coro = waiter();
        
        // Start waiter
        waiter_coro.resume();
        scheduler_t::instance().run();

        // Wait should not complete yet
        assert(wait_completed == false);
        
        // Complete tasks one by one (each needs two resumes)
        task1.resume();
        scheduler_t::instance().run();
        task1.resume();
        scheduler_t::instance().run();
        assert(tasks_completed[0] == true);
        assert(wait_completed == false); // Still waiting for other tasks

        task2.resume();
        scheduler_t::instance().run();
        task2.resume();
        scheduler_t::instance().run();
        assert(tasks_completed[1] == true);
        assert(wait_completed == false); // Still waiting for last task

        task3.resume();
        scheduler_t::instance().run();
        task3.resume();
        scheduler_t::instance().run();
        assert(tasks_completed[2] == true);

        // Waiter should complete automatically when last task calls wg.done()
        assert(wait_completed == true);
        
        std::cout << "✓ Multiple tasks wait group test passed" << std::endl;
    }
    
    std::cout << "========================" << std::endl;
    std::cout << "All wg_t tests passed! ✓" << std::endl;
    
    return 0;
}
