#include "../coco.h"
#include <iostream>
#include <cassert>
#include <vector>
#include <string>

using namespace coco;

// Test basic join functionality
void test_basic_join() {
    std::cout << "Testing basic join functionality..." << std::endl;
    
    bool task_completed = false;
    bool join_completed = false;
    
    auto task = go([&task_completed]() -> co_t {
        std::cout << "Task: Starting work" << std::endl;
        co_yield resched; // Simulate async work
        task_completed = true;
        std::cout << "Task: Work completed" << std::endl;
        co_return;
    });
    
    auto joiner = [&task, &join_completed]() -> co_t {
        std::cout << "Joiner: Waiting for task to complete" << std::endl;
        co_await task.join();
        join_completed = true;
        std::cout << "Joiner: Task completed!" << std::endl;
        co_return;
    };
    
    co_t joiner_coro = joiner();
    joiner_coro.resume();
    
    // Run scheduler to process all coroutines
    scheduler_t::instance().run();
    
    assert(task_completed == true);
    assert(join_completed == true);
    
    std::cout << "✓ Basic join test passed" << std::endl;
}

// Test multiple joiners on same coroutine
void test_multiple_joiners() {
    std::cout << "Testing multiple joiners..." << std::endl;

    bool task_completed = false;
    int joiners_completed = 0;
    std::vector<co_t> joiner_coros;

    auto task = go([&task_completed]() -> co_t {
        std::cout << "Task: Starting work" << std::endl;
        co_yield resched; // Simulate async work
        co_yield resched; // Give more time for joiners to set up
        task_completed = true;
        std::cout << "Task: Work completed" << std::endl;
        co_return;
    });

    // Create multiple joiners
    for (int i = 0; i < 3; i++) {
        auto joiner = [&task, &joiners_completed, i]() -> co_t {
            std::cout << "Joiner " << i << ": Waiting for task" << std::endl;
            co_await task.join();
            joiners_completed++;
            std::cout << "Joiner " << i << ": Task completed!" << std::endl;
            co_return;
        };

        joiner_coros.emplace_back(joiner());
        joiner_coros.back().resume();
    }

    // Run scheduler to process all coroutines
    scheduler_t::instance().run();

    assert(task_completed == true);
    assert(joiners_completed == 3);

    std::cout << "✓ Multiple joiners test passed" << std::endl;
}

// Test join with exception handling
void test_join_with_exception() {
    std::cout << "Testing join with exception handling..." << std::endl;
    
    bool exception_caught = false;
    
    auto task = go([]() -> co_t {
        std::cout << "Task: Starting work" << std::endl;
        co_yield resched; // Simulate async work
        std::cout << "Task: About to throw exception" << std::endl;
        throw std::runtime_error("Test exception");
        co_return;
    });
    
    auto joiner = [&task, &exception_caught]() -> co_t {
        try {
            std::cout << "Joiner: Waiting for task" << std::endl;
            co_await task.join();
            std::cout << "Joiner: This should not be reached" << std::endl;
        } catch (const std::exception& e) {
            exception_caught = true;
            std::cout << "Joiner: Caught exception: " << e.what() << std::endl;
        }
        co_return;
    };
    
    co_t joiner_coro = joiner();
    joiner_coro.resume();
    
    // Run scheduler to process all coroutines
    scheduler_t::instance().run();
    
    assert(exception_caught == true);
    
    std::cout << "✓ Join with exception test passed" << std::endl;
}

// Test immediate join (coroutine already completed)
void test_immediate_join() {
    std::cout << "Testing immediate join..." << std::endl;
    
    bool join_completed = false;
    
    auto task = go([]() -> co_t {
        std::cout << "Task: Completing immediately" << std::endl;
        co_return;
    });
    
    // Let the task complete first
    scheduler_t::instance().run();
    
    auto joiner = [&task, &join_completed]() -> co_t {
        std::cout << "Joiner: Joining already completed task" << std::endl;
        co_await task.join();
        join_completed = true;
        std::cout << "Joiner: Join completed immediately" << std::endl;
        co_return;
    };
    
    co_t joiner_coro = joiner();
    joiner_coro.resume();
    
    // Run scheduler to process joiner
    scheduler_t::instance().run();
    
    assert(join_completed == true);
    
    std::cout << "✓ Immediate join test passed" << std::endl;
}

// Test simple sequential join
void test_sequential_join() {
    std::cout << "Testing sequential join..." << std::endl;

    bool task1_done = false;
    bool task2_done = false;
    bool coordinator_done = false;

    auto task1 = go([&task1_done]() -> co_t {
        std::cout << "Task1: Working" << std::endl;
        co_yield resched;
        task1_done = true;
        std::cout << "Task1: Done, flag set to " << task1_done << std::endl;
        co_return;
    });

    auto task2 = go([&task2_done]() -> co_t {
        std::cout << "Task2: Working" << std::endl;
        co_yield resched;
        task2_done = true;
        std::cout << "Task2: Done, flag set to " << task2_done << std::endl;
        co_return;
    });

    auto coordinator = [&task1, &task2, &coordinator_done, &task1_done, &task2_done]() -> co_t {
        std::cout << "Coordinator: Waiting for task1" << std::endl;
        co_await task1.join();
        std::cout << "Coordinator: Task1 joined, flag is " << task1_done << std::endl;
        std::cout << "Coordinator: Waiting for task2" << std::endl;
        co_await task2.join();
        std::cout << "Coordinator: Task2 joined, flag is " << task2_done << std::endl;
        coordinator_done = true;
        std::cout << "Coordinator: Both completed!" << std::endl;
        co_return;
    };

    co_t coordinator_coro = coordinator();
    coordinator_coro.resume();

    // Run scheduler to process all coroutines
    scheduler_t::instance().run();

    std::cout << "Final check: task1_done=" << task1_done << ", task2_done=" << task2_done << ", coordinator_done=" << coordinator_done << std::endl;

    assert(task1_done == true);
    assert(task2_done == true);
    assert(coordinator_done == true);

    std::cout << "✓ Sequential join test passed" << std::endl;
}

// Test is_done() method
void test_is_done_method() {
    std::cout << "Testing is_done() method..." << std::endl;

    auto task = go([]() -> co_t {
        co_yield resched;
        co_return;
    });

    // Task should not be done initially
    assert(task.is_done() == false);

    // Run scheduler to complete the task
    scheduler_t::instance().run();

    // Task should be done now
    assert(task.is_done() == true);

    std::cout << "✓ is_done() method test passed" << std::endl;
}

int main() {
    std::cout << "Running co_t join tests..." << std::endl;
    std::cout << "=========================" << std::endl;

    test_basic_join();
    test_multiple_joiners();
    test_join_with_exception();
    test_immediate_join();
    test_is_done_method();

    std::cout << "\n✓ All co_t join tests passed!" << std::endl;
    return 0;
}
