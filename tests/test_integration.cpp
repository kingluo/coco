#include "../coco.h"
#include <iostream>
#include <cassert>
#include <vector>
#include <string>

using namespace coco;

// Test 1: Producer-Consumer pattern with channels
void test_producer_consumer() {
    std::cout << "Testing producer-consumer pattern..." << std::endl;
    
    chan_t<int> ch(2); // Buffered channel
    std::vector<int> consumed_values;
    bool producer_done = false;
    bool consumer_done = false;
    
    auto producer = [&ch, &producer_done]() -> co_t {
        for (int i = 1; i <= 3; i++) {
            bool ok = co_await ch.write(i);
            if (!ok) break;
        }
        ch.close();
        producer_done = true;
        co_return;
    };
    
    auto consumer = [&ch, &consumed_values, &consumer_done]() -> co_t {
        while (true) {
            auto result = co_await ch.read();
            if (!result.has_value()) break;
            consumed_values.push_back(result.value());
        }
        consumer_done = true;
        co_return;
    };
    
    // Run producer
    co_t prod_coro = producer();
    prod_coro.resume();
    scheduler_t::instance().run();
    
    // Run consumer
    co_t cons_coro = consumer();
    cons_coro.resume();
    scheduler_t::instance().run();
    
    // Verify results
    assert(producer_done);
    assert(consumer_done);
    assert(consumed_values.size() == 3);
    assert(consumed_values[0] == 1);
    assert(consumed_values[1] == 2);
    assert(consumed_values[2] == 3);
    
    std::cout << "✓ Producer-consumer test passed" << std::endl;
}

// Test 2: Worker pool with wait groups
void test_worker_pool() {
    std::cout << "Testing worker pool with wait groups..." << std::endl;
    
    wg_t wg;
    chan_t<std::string> results(10);
    std::vector<std::string> collected_results;
    bool coordinator_done = false;
    bool collector_done = false;
    
    const int num_workers = 3;
    wg.add(num_workers);
    
    auto worker = [&wg, &results](int id) -> co_t {
        // Simulate work
        co_await std::suspend_always{};
        
        // Send result
        std::string result = "Worker " + std::to_string(id) + " done";
        co_await results.write(result);
        
        wg.done();
        co_return;
    };
    
    auto coordinator = [&wg, &results, &coordinator_done]() -> co_t {
        co_await wg.wait();
        results.close();
        coordinator_done = true;
        co_return;
    };
    
    auto collector = [&results, &collected_results, &collector_done]() -> co_t {
        while (true) {
            auto result = co_await results.read();
            if (!result.has_value()) break;
            collected_results.push_back(result.value());
        }
        collector_done = true;
        co_return;
    };
    
    // Start coordinator
    co_t coord_coro = coordinator();
    coord_coro.resume();
    scheduler_t::instance().run();
    
    // Start workers
    co_t worker1 = worker(1);
    co_t worker2 = worker(2);
    co_t worker3 = worker(3);
    
    // Execute workers (each needs two resumes)
    worker1.resume();
    scheduler_t::instance().run();
    worker1.resume();
    scheduler_t::instance().run();
    
    worker2.resume();
    scheduler_t::instance().run();
    worker2.resume();
    scheduler_t::instance().run();
    
    worker3.resume();
    scheduler_t::instance().run();
    worker3.resume();
    scheduler_t::instance().run();
    
    // Start collector
    co_t coll_coro = collector();
    coll_coro.resume();
    scheduler_t::instance().run();
    
    // Verify results
    assert(coordinator_done);
    assert(collector_done);
    assert(collected_results.size() == 3);
    
    std::cout << "✓ Worker pool test passed" << std::endl;
}

// Test 3: Pipeline pattern
void test_pipeline() {
    std::cout << "Testing pipeline pattern..." << std::endl;
    
    chan_t<int> input_ch(2);
    chan_t<int> output_ch(2);
    std::vector<int> final_results;
    bool stage1_done = false;
    bool stage2_done = false;
    bool sink_done = false;
    
    // Stage 1: Generate numbers
    auto stage1 = [&input_ch, &stage1_done]() -> co_t {
        for (int i = 1; i <= 3; i++) {
            co_await input_ch.write(i);
        }
        input_ch.close();
        stage1_done = true;
        co_return;
    };
    
    // Stage 2: Double the numbers
    auto stage2 = [&input_ch, &output_ch, &stage2_done]() -> co_t {
        while (true) {
            auto result = co_await input_ch.read();
            if (!result.has_value()) break;
            co_await output_ch.write(result.value() * 2);
        }
        output_ch.close();
        stage2_done = true;
        co_return;
    };
    
    // Sink: Collect results
    auto sink = [&output_ch, &final_results, &sink_done]() -> co_t {
        while (true) {
            auto result = co_await output_ch.read();
            if (!result.has_value()) break;
            final_results.push_back(result.value());
        }
        sink_done = true;
        co_return;
    };
    
    // Run pipeline stages
    co_t s1 = stage1();
    co_t s2 = stage2();
    co_t sk = sink();
    
    s1.resume();
    scheduler_t::instance().run();
    
    s2.resume();
    scheduler_t::instance().run();
    
    sk.resume();
    scheduler_t::instance().run();
    
    // Verify results
    assert(stage1_done);
    assert(stage2_done);
    assert(sink_done);
    assert(final_results.size() == 3);
    assert(final_results[0] == 2);
    assert(final_results[1] == 4);
    assert(final_results[2] == 6);
    
    std::cout << "✓ Pipeline test passed" << std::endl;
}

int main() {
    std::cout << "Running integration tests..." << std::endl;
    std::cout << "=============================" << std::endl;
    
    test_producer_consumer();
    test_worker_pool();
    test_pipeline();
    
    std::cout << "=============================" << std::endl;
    std::cout << "All integration tests passed! ✓" << std::endl;
    
    return 0;
}
