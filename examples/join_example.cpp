#include "../coco.h"
#include <iostream>
#include <string>

using namespace coco;

// Example worker task
co_t worker_task(int id, int work_duration) {
    std::cout << "Worker " << id << " starting..." << std::endl;

    // Simulate work with multiple yield points
    for (int i = 0; i < work_duration; i++) {
        std::cout << "Worker " << id << " working... (" << (i + 1) << "/" << work_duration << ")"
                  << std::endl;
        co_yield resched; // Yield control to allow other coroutines to run
    }

    std::cout << "Worker " << id << " completed!" << std::endl;
    co_return;
}

// Example task that might throw an exception
co_t risky_task(int id, bool should_fail) {
    std::cout << "Risky task " << id << " starting..." << std::endl;
    co_yield resched;

    if (should_fail) {
        throw std::runtime_error("Task " + std::to_string(id) + " failed!");
    }

    std::cout << "Risky task " << id << " completed successfully!" << std::endl;
    co_return;
}

// Coordinator that uses join to wait for tasks
co_t coordinator() {
    std::cout << "\n=== Join Example: Basic Task Coordination ===" << std::endl;

    // Start multiple worker tasks
    auto task1 = go([]() { return worker_task(1, 3); });
    auto task2 = go([]() { return worker_task(2, 2); });
    auto task3 = go([]() { return worker_task(3, 4); });

    std::cout << "All tasks started, waiting for completion..." << std::endl;

    // Wait for all tasks to complete
    co_await task1.join();
    std::cout << "Task 1 joined!" << std::endl;

    co_await task2.join();
    std::cout << "Task 2 joined!" << std::endl;

    co_await task3.join();
    std::cout << "Task 3 joined!" << std::endl;

    std::cout << "All tasks completed!" << std::endl;
    co_return;
}

// Example with exception handling
co_t exception_example() {
    std::cout << "\n=== Join Example: Exception Handling ===" << std::endl;

    auto safe_task = go([]() { return risky_task(1, false); });
    auto failing_task = go([]() { return risky_task(2, true); });

    // Handle successful task
    try {
        co_await safe_task.join();
        std::cout << "Safe task completed successfully!" << std::endl;
    } catch (const std::exception &e) {
        std::cout << "Safe task failed: " << e.what() << std::endl;
    }

    // Handle failing task
    try {
        co_await failing_task.join();
        std::cout << "Failing task completed successfully!" << std::endl;
    } catch (const std::exception &e) {
        std::cout << "Caught expected exception: " << e.what() << std::endl;
    }

    co_return;
}

// Simple demonstration of join functionality
co_t simple_join_demo() {
    std::cout << "\n=== Join Example: Simple Join Demo ===" << std::endl;

    auto task = go([]() { return worker_task(99, 2); });

    std::cout << "Waiting for task to complete..." << std::endl;
    co_await task.join();
    std::cout << "Task completed successfully!" << std::endl;

    co_return;
}

int main() {
    std::cout << "Coco Join Functionality Examples" << std::endl;
    std::cout << "=================================" << std::endl;

    // Run basic coordination example
    auto coord = coordinator();
    coord.resume();
    scheduler_t::instance().run();

    // Run exception handling example
    auto except = exception_example();
    except.resume();
    scheduler_t::instance().run();

    // Run simple join demo
    auto demo = simple_join_demo();
    demo.resume();
    scheduler_t::instance().run();

    std::cout << "\n=== All Examples Completed ===" << std::endl;
    return 0;
}
