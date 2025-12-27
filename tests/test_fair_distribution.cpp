#include <iostream>
#include <vector>
#include <map>
#include <cassert>
#include "coco.h"

using namespace coco;

// Demonstration of how to achieve more fair distribution using separate channels
void test_fair_distribution_with_separate_channels() {
    std::cout << "=== Fair Distribution Using Separate Channels ===" << std::endl;
    
    chan_t<int> ch1(5);
    chan_t<int> ch2(5);
    std::map<std::string, std::vector<int>> consumer_values;
    
    // Producer that distributes to multiple channels
    auto producer = [&ch1, &ch2]() -> co_t {
        for (int i = 0; i < 6; i++) {
            std::cout << "Sending: " << i << std::endl;
            if (i % 2 == 0) {
                bool ok = co_await ch1.write(i);
                assert(ok);
                std::cout << "  -> sent to channel 1" << std::endl;
            } else {
                bool ok = co_await ch2.write(i);
                assert(ok);
                std::cout << "  -> sent to channel 2" << std::endl;
            }
        }
        ch1.close();
        ch2.close();
        std::cout << "Producer finished" << std::endl;
    };
    
    // Consumer for channel 1
    auto consumer1 = [&ch1, &consumer_values]() -> co_t {
        while (true) {
            auto result = co_await ch1.read();
            if (result.has_value()) {
                int value = result.value();
                std::cout << "Consumer1 received: " << value << std::endl;
                consumer_values["Consumer1"].push_back(value);
            } else {
                std::cout << "Consumer1 channel closed" << std::endl;
                break;
            }
        }
    };
    
    // Consumer for channel 2
    auto consumer2 = [&ch2, &consumer_values]() -> co_t {
        while (true) {
            auto result = co_await ch2.read();
            if (result.has_value()) {
                int value = result.value();
                std::cout << "Consumer2 received: " << value << std::endl;
                consumer_values["Consumer2"].push_back(value);
            } else {
                std::cout << "Consumer2 channel closed" << std::endl;
                break;
            }
        }
    };
    
    // Run all coroutines
    co_t producer_coro = producer();
    co_t consumer1_coro = consumer1();
    co_t consumer2_coro = consumer2();

    // Actually run the coroutines
    producer_coro.resume();
    consumer1_coro.resume();
    consumer2_coro.resume();

    // Execute all coroutines
    scheduler_t::instance().run();
    
    // Print results
    std::cout << "\n=== Fair Distribution Results ===" << std::endl;
    int total_received = 0;
    for (const auto& [name, values] : consumer_values) {
        std::cout << name << " received " << values.size() << " values: ";
        for (int val : values) {
            std::cout << val << " ";
        }
        std::cout << std::endl;
        total_received += values.size();
    }
    
    std::cout << "Total values received: " << total_received << std::endl;
    
    if (consumer_values["Consumer1"].size() == consumer_values["Consumer2"].size()) {
        std::cout << "✅ FAIR DISTRIBUTION ACHIEVED!" << std::endl;
    } else {
        std::cout << "Distribution is still uneven" << std::endl;
    }
}

// Demonstration using a work queue pattern
void test_work_queue_pattern() {
    std::cout << "\n=== Work Queue Pattern ===" << std::endl;
    std::cout << "This shows the intended behavior of the blog.md example" << std::endl;
    std::cout << "where multiple workers compete for tasks from a shared queue." << std::endl;
    
    chan_t<int> work_queue(10);
    std::map<std::string, std::vector<int>> worker_results;
    
    // Task producer
    auto task_producer = [&work_queue]() -> co_t {
        for (int task_id = 1; task_id <= 10; task_id++) {
            std::cout << "Adding task " << task_id << " to queue" << std::endl;
            bool ok = co_await work_queue.write(task_id);
            assert(ok);
        }
        work_queue.close();
        std::cout << "All tasks added to queue" << std::endl;
    };
    
    // Worker
    auto worker = [&work_queue, &worker_results](const std::string& name) -> co_t {
        while (true) {
            auto task = co_await work_queue.read();
            if (task.has_value()) {
                int task_id = task.value();
                std::cout << name << " processing task " << task_id << std::endl;
                worker_results[name].push_back(task_id);
                // Simulate work
                co_yield resched;
            } else {
                std::cout << name << " no more tasks" << std::endl;
                break;
            }
        }
    };
    
    // Start producer and workers
    co_t producer_coro = task_producer();
    co_t worker1_coro = worker("Worker1");
    co_t worker2_coro = worker("Worker2");
    co_t worker3_coro = worker("Worker3");

    // Actually run the coroutines
    producer_coro.resume();
    worker1_coro.resume();
    worker2_coro.resume();
    worker3_coro.resume();

    // Execute all coroutines
    scheduler_t::instance().run();
    
    // Print results
    std::cout << "\n=== Work Queue Results ===" << std::endl;
    int total_tasks = 0;
    for (const auto& [name, tasks] : worker_results) {
        std::cout << name << " processed " << tasks.size() << " tasks: ";
        for (int task : tasks) {
            std::cout << task << " ";
        }
        std::cout << std::endl;
        total_tasks += tasks.size();
    }
    
    std::cout << "Total tasks processed: " << total_tasks << std::endl;
    
    // In this case, uneven distribution is actually the expected behavior
    // for a work queue - workers compete for tasks
    std::cout << "Note: In a work queue pattern, uneven distribution is normal and expected." << std::endl;
    std::cout << "The first available worker gets the next task." << std::endl;
}

int main() {
    std::cout << "=== Channel Distribution Solutions ===" << std::endl;
    std::cout << "This demonstrates different approaches to handle multiple consumers.\n" << std::endl;
    
    try {
        test_fair_distribution_with_separate_channels();
        test_work_queue_pattern();
        
        std::cout << "\n=== SUMMARY ===" << std::endl;
        std::cout << "1. The blog.md example shows typical work queue behavior where one consumer" << std::endl;
        std::cout << "   gets all items due to single-threaded scheduling." << std::endl;
        std::cout << "2. For fair distribution, use separate channels for each consumer." << std::endl;
        std::cout << "3. For work queues, the uneven distribution is actually the intended behavior." << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "Test failed with exception: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
