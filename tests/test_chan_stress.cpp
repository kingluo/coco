#include "../coco.h"
#include <iostream>
#include <cassert>
#include <vector>
#include <chrono>
#include <atomic>

using namespace coco;

// Test 1: High volume data transfer
void test_high_volume() {
    std::cout << "=== Test 1: High Volume Data Transfer ===" << std::endl;
    
    const int NUM_VALUES = 1000;
    chan_t<int> ch(100);  // Large buffer
    std::vector<int> received;
    bool producer_done = false;
    bool consumer_done = false;
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    auto producer = [&ch, &producer_done]() -> co_t {
        for (int i = 0; i < NUM_VALUES; i++) {
            co_await ch.write(i);
        }
        ch.close();
        producer_done = true;
        co_return;
    };
    
    auto consumer = [&ch, &received, &consumer_done]() -> co_t {
        while (true) {
            auto result = co_await ch.read();
            if (!result.has_value()) break;
            received.push_back(result.value());
        }
        consumer_done = true;
        co_return;
    };
    
    co_t prod_coro = producer();
    co_t cons_coro = consumer();
    
    prod_coro.resume();
    cons_coro.resume();
    scheduler_t::instance().run();
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    std::cout << "Transferred " << received.size() << " values in " << duration.count() << "ms" << std::endl;
    std::cout << "Producer done: " << producer_done << ", Consumer done: " << consumer_done << std::endl;
    
    assert(producer_done && consumer_done);
    assert(received.size() == NUM_VALUES);
    
    // Check ordering
    bool ordered = true;
    for (int i = 0; i < NUM_VALUES; i++) {
        if (received[i] != i) {
            ordered = false;
            break;
        }
    }
    assert(ordered);
    
    std::cout << "✓ High volume test passed" << std::endl;
}

// Test 2: Multiple producers, single consumer
void test_multiple_producers() {
    std::cout << "\n=== Test 2: Multiple Producers, Single Consumer ===" << std::endl;

    // Use buffer size of 10 to test the direct handoff mechanism
    chan_t<int> ch(10);
    std::vector<int> received;
    bool prod1_done = false, prod2_done = false, prod3_done = false;
    bool consumer_done = false;

    auto producer1 = [&ch, &prod1_done]() -> co_t {
        for (int i = 100; i < 110; i++) {
            co_await ch.write(i);
        }
        prod1_done = true;
        co_return;
    };

    auto producer2 = [&ch, &prod2_done]() -> co_t {
        for (int i = 200; i < 210; i++) {
            co_await ch.write(i);
        }
        prod2_done = true;
        co_return;
    };

    auto producer3 = [&ch, &prod3_done]() -> co_t {
        for (int i = 300; i < 310; i++) {
            co_await ch.write(i);
        }
        prod3_done = true;
        co_return;
    };

    auto consumer = [&ch, &received, &consumer_done]() -> co_t {
        // Read exactly 30 values (10 from each producer)
        for (int i = 0; i < 30; i++) {
            auto result = co_await ch.read();
            if (result.has_value()) {
                received.push_back(result.value());
            }
        }
        consumer_done = true;
        co_return;
    };

    co_t prod1_coro = producer1();
    co_t prod2_coro = producer2();
    co_t prod3_coro = producer3();
    co_t cons_coro = consumer();

    prod1_coro.resume();
    prod2_coro.resume();
    prod3_coro.resume();
    cons_coro.resume();
    scheduler_t::instance().run();

    std::cout << "Producers done: " << prod1_done << ", " << prod2_done << ", " << prod3_done << std::endl;
    std::cout << "Consumer done: " << consumer_done << ", received: " << received.size() << " values" << std::endl;

    assert(prod1_done && prod2_done && prod3_done && consumer_done);
    assert(received.size() == 30);

    std::cout << "✓ Multiple producers test passed" << std::endl;
}

// Test 3: Channel with complex data types
void test_complex_data_types() {
    std::cout << "\n=== Test 3: Complex Data Types ===" << std::endl;
    
    struct ComplexData {
        int id;
        std::string name;
        std::vector<int> values;
        
        bool operator==(const ComplexData& other) const {
            return id == other.id && name == other.name && values == other.values;
        }
    };
    
    chan_t<ComplexData> ch(5);
    std::vector<ComplexData> received;
    bool producer_done = false;
    bool consumer_done = false;
    
    auto producer = [&ch, &producer_done]() -> co_t {
        ComplexData data1{1, "first", {1, 2, 3}};
        ComplexData data2{2, "second", {4, 5, 6}};
        ComplexData data3{3, "third", {7, 8, 9}};
        
        co_await ch.write(data1);
        co_await ch.write(data2);
        co_await ch.write(data3);
        ch.close();
        producer_done = true;
        co_return;
    };
    
    auto consumer = [&ch, &received, &consumer_done]() -> co_t {
        while (true) {
            auto result = co_await ch.read();
            if (!result.has_value()) break;
            received.push_back(result.value());
        }
        consumer_done = true;
        co_return;
    };
    
    co_t prod_coro = producer();
    co_t cons_coro = consumer();
    
    prod_coro.resume();
    cons_coro.resume();
    scheduler_t::instance().run();
    
    std::cout << "Producer done: " << producer_done << ", Consumer done: " << consumer_done << std::endl;
    std::cout << "Received " << received.size() << " complex objects" << std::endl;
    
    assert(producer_done && consumer_done);
    assert(received.size() == 3);
    assert(received[0].id == 1 && received[0].name == "first");
    assert(received[1].id == 2 && received[1].name == "second");
    assert(received[2].id == 3 && received[2].name == "third");
    
    std::cout << "✓ Complex data types test passed" << std::endl;
}

int main() {
    std::cout << "=== chan_t Stress Test Suite ===" << std::endl;
    std::cout << "=================================" << std::endl;

    try {
        test_high_volume();
        test_multiple_producers();
        test_complex_data_types();

        std::cout << "\n=================================" << std::endl;
        std::cout << "🎉 All stress tests passed!" << std::endl;
        return 0;

    } catch (const std::exception& e) {
        std::cout << "❌ Stress test failed: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cout << "❌ Stress test failed with unknown exception" << std::endl;
        return 1;
    }
}
