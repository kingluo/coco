#include <iostream>
#include <cassert>
#include <vector>
#include "../coco.h"

using namespace coco;

void test_multiple_waiters_basic() {
    std::cout << "=== Testing Multiple Waiters Basic ===" << std::endl;
    
    wg_t wg;
    std::vector<bool> waiters_completed(3, false);
    bool task_completed = false;
    
    wg.add(1);
    
    auto task = [&wg, &task_completed]() -> co_t {
        task_completed = true;
        wg.done();
        co_return;
    };
    
    auto waiter1 = [&wg, &waiters_completed]() -> co_t {
        co_await wg.wait();
        waiters_completed[0] = true;
        co_return;
    };
    
    auto waiter2 = [&wg, &waiters_completed]() -> co_t {
        co_await wg.wait();
        waiters_completed[1] = true;
        co_return;
    };
    
    auto waiter3 = [&wg, &waiters_completed]() -> co_t {
        co_await wg.wait();
        waiters_completed[2] = true;
        co_return;
    };
    
    // Create coroutines
    co_t task_coro = task();
    co_t waiter1_coro = waiter1();
    co_t waiter2_coro = waiter2();
    co_t waiter3_coro = waiter3();
    
    // Start waiters first
    waiter1_coro.resume();
    waiter2_coro.resume();
    waiter3_coro.resume();
    
    // Run scheduler to process any pending operations
    scheduler_t::instance().run();
    
    // Complete the task
    task_coro.resume();
    
    // Run scheduler again to process completions
    scheduler_t::instance().run();
    
    // Check results
    assert(task_completed == true);
    assert(waiters_completed[0] == true);
    assert(waiters_completed[1] == true);
    assert(waiters_completed[2] == true);
    
    std::cout << "✓ All three waiters were notified when task completed" << std::endl;
    
    (void)task_coro;
    (void)waiter1_coro;
    (void)waiter2_coro;
    (void)waiter3_coro;
}

void test_multiple_waiters_multiple_tasks() {
    std::cout << "\n=== Testing Multiple Waiters with Multiple Tasks ===" << std::endl;
    
    wg_t wg;
    std::vector<bool> waiters_completed(2, false);
    std::vector<bool> tasks_completed(3, false);
    
    wg.add(3); // Three tasks
    
    auto create_task = [&wg, &tasks_completed](int task_id) -> co_t {
        tasks_completed[task_id] = true;
        wg.done();
        co_return;
    };
    
    auto waiter1 = [&wg, &waiters_completed]() -> co_t {
        co_await wg.wait();
        waiters_completed[0] = true;
        co_return;
    };
    
    auto waiter2 = [&wg, &waiters_completed]() -> co_t {
        co_await wg.wait();
        waiters_completed[1] = true;
        co_return;
    };
    
    // Create coroutines
    co_t task1_coro = create_task(0);
    co_t task2_coro = create_task(1);
    co_t task3_coro = create_task(2);
    co_t waiter1_coro = waiter1();
    co_t waiter2_coro = waiter2();
    
    // Start waiters first
    waiter1_coro.resume();
    waiter2_coro.resume();
    scheduler_t::instance().run();
    
    // Complete tasks one by one
    task1_coro.resume();
    scheduler_t::instance().run();
    
    task2_coro.resume();
    scheduler_t::instance().run();
    
    task3_coro.resume(); // This should trigger waiter completion
    scheduler_t::instance().run();
    
    // Check results
    assert(tasks_completed[0] == true);
    assert(tasks_completed[1] == true);
    assert(tasks_completed[2] == true);
    assert(waiters_completed[0] == true);
    assert(waiters_completed[1] == true);
    
    std::cout << "✓ Both waiters were notified after all tasks completed" << std::endl;
    
    (void)task1_coro;
    (void)task2_coro;
    (void)task3_coro;
    (void)waiter1_coro;
    (void)waiter2_coro;
}

int main() {
    test_multiple_waiters_basic();
    test_multiple_waiters_multiple_tasks();
    
    std::cout << "\n=== All wg_t multiple waiters tests passed! ✓ ===" << std::endl;
    return 0;
}
