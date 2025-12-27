#include "../coco.h"
#include <iostream>
#include <vector>
#include <cassert>

using namespace coco;

// Test the exact scenario described: unbuffered channel, writer first, then 3 readers
void test_even_consumption_scenario() {
    std::cout << "=== Testing Even Channel Consumption Scenario ===" << std::endl;
    std::cout << "Scenario: Create unbuffered channel, writer first, then readers r1, r2, r3" << std::endl;
    std::cout << "Writer writes 3 values, each reader should get one value" << std::endl;
    
    chan_t<int> ch(0); // Unbuffered channel
    std::vector<std::string> reader_order;
    std::vector<int> r1_values, r2_values, r3_values;
    
    // Writer coroutine - created and resumed first
    auto writer = [&ch]() -> co_t {
        std::cout << "Writer: Starting..." << std::endl;
        
        std::cout << "Writer: Sending value 1..." << std::endl;
        bool ok1 = co_await ch.write(1);
        std::cout << "Writer: Sent 1, ok=" << ok1 << std::endl;
        
        std::cout << "Writer: Sending value 2..." << std::endl;
        bool ok2 = co_await ch.write(2);
        std::cout << "Writer: Sent 2, ok=" << ok2 << std::endl;
        
        std::cout << "Writer: Sending value 3..." << std::endl;
        bool ok3 = co_await ch.write(3);
        std::cout << "Writer: Sent 3, ok=" << ok3 << std::endl;
        
        std::cout << "Writer: Done" << std::endl;
        co_return;
    };
    
    // Reader 1 - created and resumed second
    auto reader1 = [&ch, &reader_order, &r1_values]() -> co_t {
        std::cout << "Reader1: Starting and waiting..." << std::endl;
        auto result = co_await ch.read();
        if (result.has_value()) {
            reader_order.push_back("R1");
            r1_values.push_back(result.value());
            std::cout << "Reader1: Received " << result.value() << std::endl;
        }
        co_return;
    };
    
    // Reader 2 - created and resumed third
    auto reader2 = [&ch, &reader_order, &r2_values]() -> co_t {
        std::cout << "Reader2: Starting and waiting..." << std::endl;
        auto result = co_await ch.read();
        if (result.has_value()) {
            reader_order.push_back("R2");
            r2_values.push_back(result.value());
            std::cout << "Reader2: Received " << result.value() << std::endl;
        }
        co_return;
    };
    
    // Reader 3 - created and resumed fourth
    auto reader3 = [&ch, &reader_order, &r3_values]() -> co_t {
        std::cout << "Reader3: Starting and waiting..." << std::endl;
        auto result = co_await ch.read();
        if (result.has_value()) {
            reader_order.push_back("R3");
            r3_values.push_back(result.value());
            std::cout << "Reader3: Received " << result.value() << std::endl;
        }
        co_return;
    };
    
    // Create coroutines
    co_t w_coro = writer();
    co_t r1_coro = reader1();
    co_t r2_coro = reader2();
    co_t r3_coro = reader3();
    
    // Resume in the specified order: writer first, then readers r1, r2, r3
    std::cout << "\nResuming coroutines in order: writer, r1, r2, r3" << std::endl;
    w_coro.resume();   // Writer first
    r1_coro.resume();  // Reader1 second
    r2_coro.resume();  // Reader2 third
    r3_coro.resume();  // Reader3 fourth
    
    // Run scheduler
    scheduler_t::instance().run();
    
    // Analyze results
    std::cout << "\n=== Results Analysis ===" << std::endl;
    std::cout << "Reader execution order: ";
    for (const auto& r : reader_order) {
        std::cout << r << " ";
    }
    std::cout << std::endl;
    
    std::cout << "Reader1 received " << r1_values.size() << " values: ";
    for (int v : r1_values) std::cout << v << " ";
    std::cout << std::endl;
    
    std::cout << "Reader2 received " << r2_values.size() << " values: ";
    for (int v : r2_values) std::cout << v << " ";
    std::cout << std::endl;
    
    std::cout << "Reader3 received " << r3_values.size() << " values: ";
    for (int v : r3_values) std::cout << v << " ";
    std::cout << std::endl;
    
    // Check if distribution is even (each reader gets exactly 1 value)
    bool is_even = (r1_values.size() == 1) && (r2_values.size() == 1) && (r3_values.size() == 1);
    
    std::cout << "\n=== Conclusion ===" << std::endl;
    if (is_even) {
        std::cout << "✓ EVEN DISTRIBUTION: Each reader received exactly 1 value" << std::endl;
        std::cout << "✓ FIFO scheduler successfully achieved even channel consumption" << std::endl;
    } else {
        std::cout << "✗ UNEVEN DISTRIBUTION: Readers did not receive equal numbers of values" << std::endl;
        std::cout << "✗ FIFO scheduler did NOT achieve even channel consumption" << std::endl;
    }
    
    // Verify FIFO order
    bool fifo_order = (reader_order.size() == 3) && 
                      (reader_order[0] == "R1") && 
                      (reader_order[1] == "R2") && 
                      (reader_order[2] == "R3");
    
    if (fifo_order) {
        std::cout << "✓ FIFO ORDER: Readers were served in the order they were created/resumed" << std::endl;
    } else {
        std::cout << "✗ NON-FIFO ORDER: Readers were NOT served in creation/resume order" << std::endl;
    }
}

// Test variation: Different resume order
void test_different_resume_order() {
    std::cout << "\n\n=== Testing Different Resume Order ===" << std::endl;
    std::cout << "Scenario: Resume readers first, then writer" << std::endl;

    chan_t<int> ch(0);
    std::vector<std::string> reader_order;
    std::vector<int> r1_values, r2_values, r3_values;

    auto writer = [&ch]() -> co_t {
        std::cout << "Writer: Sending values..." << std::endl;
        co_await ch.write(10);
        co_await ch.write(20);
        co_await ch.write(30);
        std::cout << "Writer: Done" << std::endl;
        co_return;
    };

    auto reader1 = [&ch, &reader_order, &r1_values]() -> co_t {
        auto result = co_await ch.read();
        if (result.has_value()) {
            reader_order.push_back("R1");
            r1_values.push_back(result.value());
            std::cout << "Reader1: Received " << result.value() << std::endl;
        }
        co_return;
    };

    auto reader2 = [&ch, &reader_order, &r2_values]() -> co_t {
        auto result = co_await ch.read();
        if (result.has_value()) {
            reader_order.push_back("R2");
            r2_values.push_back(result.value());
            std::cout << "Reader2: Received " << result.value() << std::endl;
        }
        co_return;
    };

    auto reader3 = [&ch, &reader_order, &r3_values]() -> co_t {
        auto result = co_await ch.read();
        if (result.has_value()) {
            reader_order.push_back("R3");
            r3_values.push_back(result.value());
            std::cout << "Reader3: Received " << result.value() << std::endl;
        }
        co_return;
    };

    co_t w_coro = writer();
    co_t r1_coro = reader1();
    co_t r2_coro = reader2();
    co_t r3_coro = reader3();

    // Resume readers first, then writer
    std::cout << "Resuming in order: r1, r2, r3, writer" << std::endl;
    r1_coro.resume();
    r2_coro.resume();
    r3_coro.resume();
    w_coro.resume();

    scheduler_t::instance().run();

    std::cout << "Reader execution order: ";
    for (const auto& r : reader_order) std::cout << r << " ";
    std::cout << std::endl;

    bool is_even = (r1_values.size() == 1) && (r2_values.size() == 1) && (r3_values.size() == 1);
    std::cout << (is_even ? "✓ EVEN" : "✗ UNEVEN") << " distribution" << std::endl;
}

// Test with buffered channel
void test_buffered_channel_consumption() {
    std::cout << "\n\n=== Testing Buffered Channel Consumption ===" << std::endl;
    std::cout << "Scenario: Buffered channel (capacity 3), writer fills buffer, then readers consume" << std::endl;

    chan_t<int> ch(3); // Buffered channel
    std::vector<std::string> reader_order;
    std::vector<int> r1_values, r2_values, r3_values;

    auto writer = [&ch]() -> co_t {
        std::cout << "Writer: Filling buffer..." << std::endl;
        co_await ch.write(100);
        co_await ch.write(200);
        co_await ch.write(300);
        std::cout << "Writer: Buffer filled" << std::endl;
        co_return;
    };

    auto reader1 = [&ch, &reader_order, &r1_values]() -> co_t {
        auto result = co_await ch.read();
        if (result.has_value()) {
            reader_order.push_back("R1");
            r1_values.push_back(result.value());
            std::cout << "Reader1: Received " << result.value() << std::endl;
        }
        co_return;
    };

    auto reader2 = [&ch, &reader_order, &r2_values]() -> co_t {
        auto result = co_await ch.read();
        if (result.has_value()) {
            reader_order.push_back("R2");
            r2_values.push_back(result.value());
            std::cout << "Reader2: Received " << result.value() << std::endl;
        }
        co_return;
    };

    auto reader3 = [&ch, &reader_order, &r3_values]() -> co_t {
        auto result = co_await ch.read();
        if (result.has_value()) {
            reader_order.push_back("R3");
            r3_values.push_back(result.value());
            std::cout << "Reader3: Received " << result.value() << std::endl;
        }
        co_return;
    };

    co_t w_coro = writer();
    co_t r1_coro = reader1();
    co_t r2_coro = reader2();
    co_t r3_coro = reader3();

    // Writer first, then readers
    w_coro.resume();
    r1_coro.resume();
    r2_coro.resume();
    r3_coro.resume();

    scheduler_t::instance().run();

    std::cout << "Reader execution order: ";
    for (const auto& r : reader_order) std::cout << r << " ";
    std::cout << std::endl;

    bool is_even = (r1_values.size() == 1) && (r2_values.size() == 1) && (r3_values.size() == 1);
    std::cout << (is_even ? "✓ EVEN" : "✗ UNEVEN") << " distribution" << std::endl;
}

// Test with endless reading loops
void test_endless_reading_loops() {
    std::cout << "\n\n=== Testing Endless Reading Loops ===" << std::endl;
    std::cout << "Scenario: Each reader does endless loop, writer sends multiple values" << std::endl;

    chan_t<int> ch(0); // Unbuffered channel
    std::vector<int> r1_values, r2_values, r3_values;
    bool stop_reading = false;

    auto writer = [&ch, &stop_reading]() -> co_t {
        std::cout << "Writer: Starting to send 9 values..." << std::endl;
        for (int i = 1; i <= 9; i++) {
            std::cout << "Writer: Sending " << i << std::endl;
            bool ok = co_await ch.write(i);
            std::cout << "Writer: Sent " << i << ", ok=" << ok << std::endl;
        }
        std::cout << "Writer: Done sending, signaling stop" << std::endl;
        stop_reading = true;

        // Send stop signals to wake up any blocked readers
        co_await ch.write(-1);
        co_await ch.write(-1);
        co_await ch.write(-1);
        co_return;
    };

    auto reader1 = [&ch, &r1_values, &stop_reading]() -> co_t {
        std::cout << "Reader1: Starting endless loop..." << std::endl;
        while (!stop_reading) {
            auto result = co_await ch.read();
            if (result.has_value()) {
                int val = result.value();
                if (val == -1) break; // Stop signal
                r1_values.push_back(val);
                std::cout << "Reader1: Received " << val << " (total: " << r1_values.size() << ")" << std::endl;
            }
        }
        std::cout << "Reader1: Stopped" << std::endl;
        co_return;
    };

    auto reader2 = [&ch, &r2_values, &stop_reading]() -> co_t {
        std::cout << "Reader2: Starting endless loop..." << std::endl;
        while (!stop_reading) {
            auto result = co_await ch.read();
            if (result.has_value()) {
                int val = result.value();
                if (val == -1) break; // Stop signal
                r2_values.push_back(val);
                std::cout << "Reader2: Received " << val << " (total: " << r2_values.size() << ")" << std::endl;
            }
        }
        std::cout << "Reader2: Stopped" << std::endl;
        co_return;
    };

    auto reader3 = [&ch, &r3_values, &stop_reading]() -> co_t {
        std::cout << "Reader3: Starting endless loop..." << std::endl;
        while (!stop_reading) {
            auto result = co_await ch.read();
            if (result.has_value()) {
                int val = result.value();
                if (val == -1) break; // Stop signal
                r3_values.push_back(val);
                std::cout << "Reader3: Received " << val << " (total: " << r3_values.size() << ")" << std::endl;
            }
        }
        std::cout << "Reader3: Stopped" << std::endl;
        co_return;
    };

    co_t w_coro = writer();
    co_t r1_coro = reader1();
    co_t r2_coro = reader2();
    co_t r3_coro = reader3();

    // Resume in order: writer first, then readers
    std::cout << "Resuming coroutines..." << std::endl;
    w_coro.resume();
    r1_coro.resume();
    r2_coro.resume();
    r3_coro.resume();

    scheduler_t::instance().run();

    // Analyze distribution
    std::cout << "\n=== Distribution Analysis ===" << std::endl;
    std::cout << "Reader1 received " << r1_values.size() << " values: ";
    for (int v : r1_values) std::cout << v << " ";
    std::cout << std::endl;

    std::cout << "Reader2 received " << r2_values.size() << " values: ";
    for (int v : r2_values) std::cout << v << " ";
    std::cout << std::endl;

    std::cout << "Reader3 received " << r3_values.size() << " values: ";
    for (int v : r3_values) std::cout << v << " ";
    std::cout << std::endl;

    size_t total = r1_values.size() + r2_values.size() + r3_values.size();
    std::cout << "Total values distributed: " << total << " out of 9 sent" << std::endl;

    // Check fairness
    size_t max_diff = 0;
    size_t min_count = std::min({r1_values.size(), r2_values.size(), r3_values.size()});
    size_t max_count = std::max({r1_values.size(), r2_values.size(), r3_values.size()});
    max_diff = max_count - min_count;

    std::cout << "\n=== Fairness Analysis ===" << std::endl;
    std::cout << "Min values received by any reader: " << min_count << std::endl;
    std::cout << "Max values received by any reader: " << max_count << std::endl;
    std::cout << "Difference (unfairness): " << max_diff << std::endl;

    if (max_diff <= 1) {
        std::cout << "✓ FAIR DISTRIBUTION: Difference ≤ 1, very fair" << std::endl;
    } else if (max_diff <= 2) {
        std::cout << "~ MOSTLY FAIR: Difference ≤ 2, reasonably fair" << std::endl;
    } else {
        std::cout << "✗ UNFAIR DISTRIBUTION: Difference > 2, unfair" << std::endl;
    }

    // Check if perfect 3-3-3 distribution
    bool perfect_even = (r1_values.size() == 3) && (r2_values.size() == 3) && (r3_values.size() == 3);
    if (perfect_even) {
        std::cout << "✓ PERFECT EVEN DISTRIBUTION: Each reader got exactly 3 values" << std::endl;
    }
}

// Test with cooperative yielding
void test_endless_loops_with_yielding() {
    std::cout << "\n\n=== Testing Endless Loops WITH Cooperative Yielding ===" << std::endl;
    std::cout << "Scenario: Each reader yields after reading, allowing others to run" << std::endl;

    chan_t<int> ch(0);
    std::vector<int> r1_values, r2_values, r3_values;
    bool stop_reading = false;

    auto writer = [&ch, &stop_reading]() -> co_t {
        std::cout << "Writer: Starting to send 9 values..." << std::endl;
        for (int i = 1; i <= 9; i++) {
            std::cout << "Writer: Sending " << i << std::endl;
            bool ok = co_await ch.write(i);
            std::cout << "Writer: Sent " << i << ", ok=" << ok << std::endl;
            // Small yield to allow readers to process
            co_await std::suspend_always{};
        }
        std::cout << "Writer: Done sending" << std::endl;
        stop_reading = true;

        // Send stop signals
        co_await ch.write(-1);
        co_await ch.write(-1);
        co_await ch.write(-1);
        co_return;
    };

    auto reader1 = [&ch, &r1_values, &stop_reading]() -> co_t {
        std::cout << "Reader1: Starting endless loop with yielding..." << std::endl;
        while (!stop_reading) {
            auto result = co_await ch.read();
            if (result.has_value()) {
                int val = result.value();
                if (val == -1) break;
                r1_values.push_back(val);
                std::cout << "Reader1: Received " << val << " (total: " << r1_values.size() << ")" << std::endl;
                // Yield to allow other readers a chance
                co_await std::suspend_always{};
            }
        }
        std::cout << "Reader1: Stopped" << std::endl;
        co_return;
    };

    auto reader2 = [&ch, &r2_values, &stop_reading]() -> co_t {
        std::cout << "Reader2: Starting endless loop with yielding..." << std::endl;
        while (!stop_reading) {
            auto result = co_await ch.read();
            if (result.has_value()) {
                int val = result.value();
                if (val == -1) break;
                r2_values.push_back(val);
                std::cout << "Reader2: Received " << val << " (total: " << r2_values.size() << ")" << std::endl;
                // Yield to allow other readers a chance
                co_await std::suspend_always{};
            }
        }
        std::cout << "Reader2: Stopped" << std::endl;
        co_return;
    };

    auto reader3 = [&ch, &r3_values, &stop_reading]() -> co_t {
        std::cout << "Reader3: Starting endless loop with yielding..." << std::endl;
        while (!stop_reading) {
            auto result = co_await ch.read();
            if (result.has_value()) {
                int val = result.value();
                if (val == -1) break;
                r3_values.push_back(val);
                std::cout << "Reader3: Received " << val << " (total: " << r3_values.size() << ")" << std::endl;
                // Yield to allow other readers a chance
                co_await std::suspend_always{};
            }
        }
        std::cout << "Reader3: Stopped" << std::endl;
        co_return;
    };

    co_t w_coro = writer();
    co_t r1_coro = reader1();
    co_t r2_coro = reader2();
    co_t r3_coro = reader3();

    w_coro.resume();
    r1_coro.resume();
    r2_coro.resume();
    r3_coro.resume();

    scheduler_t::instance().run();

    // Analyze distribution
    std::cout << "\n=== Distribution Analysis (With Yielding) ===" << std::endl;
    std::cout << "Reader1 received " << r1_values.size() << " values: ";
    for (int v : r1_values) std::cout << v << " ";
    std::cout << std::endl;

    std::cout << "Reader2 received " << r2_values.size() << " values: ";
    for (int v : r2_values) std::cout << v << " ";
    std::cout << std::endl;

    std::cout << "Reader3 received " << r3_values.size() << " values: ";
    for (int v : r3_values) std::cout << v << " ";
    std::cout << std::endl;

    size_t max_diff = std::max({r1_values.size(), r2_values.size(), r3_values.size()}) -
                      std::min({r1_values.size(), r2_values.size(), r3_values.size()});

    std::cout << "Unfairness (max difference): " << max_diff << std::endl;

    if (max_diff <= 1) {
        std::cout << "✓ FAIR DISTRIBUTION with yielding" << std::endl;
    } else {
        std::cout << "✗ Still unfair even with yielding" << std::endl;
    }
}

// Test to understand why Reader1 dominates
void test_reader_dominance_analysis() {
    std::cout << "\n\n=== Analyzing Reader Dominance ===" << std::endl;
    std::cout << "Understanding why Reader1 gets all values in endless loops" << std::endl;

    chan_t<int> ch(0);
    std::vector<int> r1_values, r2_values, r3_values;
    int values_sent = 0;

    auto writer = [&ch, &values_sent]() -> co_t {
        std::cout << "Writer: Sending 6 values slowly..." << std::endl;
        for (int i = 1; i <= 6; i++) {
            std::cout << "Writer: About to send " << i << std::endl;
            bool ok = co_await ch.write(i);
            values_sent++;
            std::cout << "Writer: Sent " << i << ", ok=" << ok << " (total sent: " << values_sent << ")" << std::endl;
        }
        std::cout << "Writer: Finished" << std::endl;
        co_return;
    };

    auto reader1 = [&ch, &r1_values, &values_sent]() -> co_t {
        std::cout << "Reader1: Starting..." << std::endl;
        while (r1_values.size() < 6 && values_sent < 6) {
            std::cout << "Reader1: Attempting read..." << std::endl;
            auto result = co_await ch.read();
            if (result.has_value()) {
                int val = result.value();
                r1_values.push_back(val);
                std::cout << "Reader1: Got " << val << " (total: " << r1_values.size() << ")" << std::endl;

                // After reading, immediately try to read again (greedy behavior)
                std::cout << "Reader1: Immediately trying next read..." << std::endl;
            }
        }
        std::cout << "Reader1: Done" << std::endl;
        co_return;
    };

    auto reader2 = [&ch, &r2_values, &values_sent]() -> co_t {
        std::cout << "Reader2: Starting..." << std::endl;
        while (r2_values.size() < 6 && values_sent < 6) {
            std::cout << "Reader2: Attempting read..." << std::endl;
            auto result = co_await ch.read();
            if (result.has_value()) {
                int val = result.value();
                r2_values.push_back(val);
                std::cout << "Reader2: Got " << val << " (total: " << r2_values.size() << ")" << std::endl;
            }
        }
        std::cout << "Reader2: Done" << std::endl;
        co_return;
    };

    auto reader3 = [&ch, &r3_values, &values_sent]() -> co_t {
        std::cout << "Reader3: Starting..." << std::endl;
        while (r3_values.size() < 6 && values_sent < 6) {
            std::cout << "Reader3: Attempting read..." << std::endl;
            auto result = co_await ch.read();
            if (result.has_value()) {
                int val = result.value();
                r3_values.push_back(val);
                std::cout << "Reader3: Got " << val << " (total: " << r3_values.size() << ")" << std::endl;
            }
        }
        std::cout << "Reader3: Done" << std::endl;
        co_return;
    };

    co_t w_coro = writer();
    co_t r1_coro = reader1();
    co_t r2_coro = reader2();
    co_t r3_coro = reader3();

    // Start writer first, then readers
    w_coro.resume();
    r1_coro.resume();
    r2_coro.resume();
    r3_coro.resume();

    scheduler_t::instance().run();

    std::cout << "\n=== Final Analysis ===" << std::endl;
    std::cout << "Reader1: " << r1_values.size() << " values" << std::endl;
    std::cout << "Reader2: " << r2_values.size() << " values" << std::endl;
    std::cout << "Reader3: " << r3_values.size() << " values" << std::endl;

    std::cout << "\n=== Key Insight ===" << std::endl;
    std::cout << "The issue: Once Reader1 gets the first value, it immediately" << std::endl;
    std::cout << "goes back to the read() call and blocks again, putting itself" << std::endl;
    std::cout << "at the FRONT of the reader queue before other readers get a chance!" << std::endl;
}

// Test solution: Proper yielding for fairness
void test_fair_endless_loops() {
    std::cout << "\n\n=== Testing FAIR Endless Loops (Proper Solution) ===" << std::endl;
    std::cout << "Solution: Each reader yields BEFORE attempting next read" << std::endl;

    chan_t<int> ch(0);
    std::vector<int> r1_values, r2_values, r3_values;
    bool stop_reading = false;

    auto writer = [&ch, &stop_reading]() -> co_t {
        std::cout << "Writer: Sending 9 values..." << std::endl;
        for (int i = 1; i <= 9; i++) {
            bool ok = co_await ch.write(i);
            std::cout << "Writer: Sent " << i << ", ok=" << ok << std::endl;
        }
        stop_reading = true;

        // Send stop signals
        co_await ch.write(-1);
        co_await ch.write(-1);
        co_await ch.write(-1);
        co_return;
    };

    auto reader1 = [&ch, &r1_values, &stop_reading]() -> co_t {
        while (!stop_reading) {
            auto result = co_await ch.read();
            if (result.has_value()) {
                int val = result.value();
                if (val == -1) break;
                r1_values.push_back(val);
                std::cout << "Reader1: Got " << val << " (total: " << r1_values.size() << ")" << std::endl;
            }
            // KEY: Yield BEFORE attempting next read to give others a chance
            co_await std::suspend_always{};
        }
        co_return;
    };

    auto reader2 = [&ch, &r2_values, &stop_reading]() -> co_t {
        while (!stop_reading) {
            auto result = co_await ch.read();
            if (result.has_value()) {
                int val = result.value();
                if (val == -1) break;
                r2_values.push_back(val);
                std::cout << "Reader2: Got " << val << " (total: " << r2_values.size() << ")" << std::endl;
            }
            // KEY: Yield BEFORE attempting next read
            co_await std::suspend_always{};
        }
        co_return;
    };

    auto reader3 = [&ch, &r3_values, &stop_reading]() -> co_t {
        while (!stop_reading) {
            auto result = co_await ch.read();
            if (result.has_value()) {
                int val = result.value();
                if (val == -1) break;
                r3_values.push_back(val);
                std::cout << "Reader3: Got " << val << " (total: " << r3_values.size() << ")" << std::endl;
            }
            // KEY: Yield BEFORE attempting next read
            co_await std::suspend_always{};
        }
        co_return;
    };

    co_t w_coro = writer();
    co_t r1_coro = reader1();
    co_t r2_coro = reader2();
    co_t r3_coro = reader3();

    w_coro.resume();
    r1_coro.resume();
    r2_coro.resume();
    r3_coro.resume();

    scheduler_t::instance().run();

    std::cout << "\n=== Fair Distribution Results ===" << std::endl;
    std::cout << "Reader1: " << r1_values.size() << " values: ";
    for (int v : r1_values) std::cout << v << " ";
    std::cout << std::endl;

    std::cout << "Reader2: " << r2_values.size() << " values: ";
    for (int v : r2_values) std::cout << v << " ";
    std::cout << std::endl;

    std::cout << "Reader3: " << r3_values.size() << " values: ";
    for (int v : r3_values) std::cout << v << " ";
    std::cout << std::endl;

    size_t max_diff = std::max({r1_values.size(), r2_values.size(), r3_values.size()}) -
                      std::min({r1_values.size(), r2_values.size(), r3_values.size()});

    std::cout << "Unfairness: " << max_diff << std::endl;

    if (max_diff <= 1) {
        std::cout << "✓ ACHIEVED FAIR DISTRIBUTION!" << std::endl;
    } else {
        std::cout << "✗ Still unfair" << std::endl;
    }
}

// Test to trace scheduler queue behavior step by step
void test_scheduler_queue_tracing() {
    std::cout << "\n\n=== Tracing Scheduler Queue Behavior ===" << std::endl;
    std::cout << "Understanding the Reader1-Writer ping-pong effect" << std::endl;

    chan_t<int> ch(0);
    std::vector<int> r1_values, r2_values, r3_values;
    int step = 0;

    auto writer = [&ch, &step]() -> co_t {
        for (int i = 1; i <= 4; i++) {
            std::cout << "\n--- STEP " << (++step) << " ---" << std::endl;
            std::cout << "Writer: About to write " << i << std::endl;
            std::cout << "Writer: Calling co_await ch.write(" << i << ")" << std::endl;

            bool ok = co_await ch.write(i);

            std::cout << "Writer: Resumed after write, ok=" << ok << std::endl;
            std::cout << "Writer: Will loop back to write next value" << std::endl;
        }
        std::cout << "\nWriter: Finished all writes, exiting" << std::endl;
        co_return;
    };

    auto reader1 = [&ch, &r1_values, &step]() -> co_t {
        std::cout << "Reader1: Starting endless loop" << std::endl;
        for (int loop = 1; loop <= 4; loop++) {
            std::cout << "Reader1: Loop " << loop << " - calling co_await ch.read()" << std::endl;
            auto result = co_await ch.read();
            if (result.has_value()) {
                int val = result.value();
                r1_values.push_back(val);
                std::cout << "Reader1: Resumed after read, got " << val << std::endl;
                std::cout << "Reader1: Will immediately loop back to read again" << std::endl;
            }
        }
        std::cout << "Reader1: Exiting after 4 reads" << std::endl;
        co_return;
    };

    auto reader2 = [&ch, &r2_values]() -> co_t {
        std::cout << "Reader2: Starting, calling co_await ch.read()" << std::endl;
        auto result = co_await ch.read();
        if (result.has_value()) {
            r2_values.push_back(result.value());
            std::cout << "Reader2: Finally got " << result.value() << "!" << std::endl;
        }
        co_return;
    };

    auto reader3 = [&ch, &r3_values]() -> co_t {
        std::cout << "Reader3: Starting, calling co_await ch.read()" << std::endl;
        auto result = co_await ch.read();
        if (result.has_value()) {
            r3_values.push_back(result.value());
            std::cout << "Reader3: Finally got " << result.value() << "!" << std::endl;
        }
        co_return;
    };

    co_t w_coro = writer();
    co_t r1_coro = reader1();
    co_t r2_coro = reader2();
    co_t r3_coro = reader3();

    std::cout << "=== Initial Resume Order ===" << std::endl;
    std::cout << "1. Resuming Writer (goes to scheduler queue)" << std::endl;
    w_coro.resume();

    std::cout << "2. Resuming Reader1 (goes to scheduler queue)" << std::endl;
    r1_coro.resume();

    std::cout << "3. Resuming Reader2 (goes to scheduler queue)" << std::endl;
    r2_coro.resume();

    std::cout << "4. Resuming Reader3 (goes to scheduler queue)" << std::endl;
    r3_coro.resume();

    std::cout << "\n=== Running Scheduler ===" << std::endl;
    std::cout << "Scheduler will process queue in FIFO order..." << std::endl;

    scheduler_t::instance().run();

    std::cout << "\n=== Final Results ===" << std::endl;
    std::cout << "Reader1 got " << r1_values.size() << " values: ";
    for (int v : r1_values) std::cout << v << " ";
    std::cout << std::endl;

    std::cout << "Reader2 got " << r2_values.size() << " values: ";
    for (int v : r2_values) std::cout << v << " ";
    std::cout << std::endl;

    std::cout << "Reader3 got " << r3_values.size() << " values: ";
    for (int v : r3_values) std::cout << v << " ";
    std::cout << std::endl;

    std::cout << "\n=== Key Insight Confirmed ===" << std::endl;
    std::cout << "The Writer-Reader1 ping-pong keeps them at the front of the scheduler queue," << std::endl;
    std::cout << "while Reader2 and Reader3 remain blocked in the channel's reader queue (rq)" << std::endl;
    std::cout << "until Writer finishes and Reader1 stops consuming!" << std::endl;
}

// Test buffered channel monopoly behavior
void test_buffered_channel_monopoly() {
    std::cout << "\n\n=== Testing Buffered Channel Monopoly Behavior ===" << std::endl;
    std::cout << "Question: Does Reader1 monopolize buffered channels too?" << std::endl;

    chan_t<int> ch(3); // Buffered channel with capacity 3
    std::vector<int> r1_values, r2_values, r3_values;

    auto writer = [&ch]() -> co_t {
        std::cout << "Writer: Filling buffer with 9 values..." << std::endl;
        for (int i = 1; i <= 9; i++) {
            std::cout << "Writer: Writing " << i << std::endl;
            bool ok = co_await ch.write(i);
            std::cout << "Writer: Wrote " << i << ", ok=" << ok << std::endl;
        }
        std::cout << "Writer: Done" << std::endl;
        co_return;
    };

    auto reader1 = [&ch, &r1_values]() -> co_t {
        std::cout << "Reader1: Starting endless loop..." << std::endl;
        for (int loop = 1; loop <= 9; loop++) {
            std::cout << "Reader1: Loop " << loop << " - reading..." << std::endl;
            auto result = co_await ch.read();
            if (result.has_value()) {
                int val = result.value();
                r1_values.push_back(val);
                std::cout << "Reader1: Got " << val << " (total: " << r1_values.size() << ")" << std::endl;
            }
        }
        std::cout << "Reader1: Finished" << std::endl;
        co_return;
    };

    auto reader2 = [&ch, &r2_values]() -> co_t {
        std::cout << "Reader2: Starting endless loop..." << std::endl;
        for (int loop = 1; loop <= 9; loop++) {
            auto result = co_await ch.read();
            if (result.has_value()) {
                int val = result.value();
                r2_values.push_back(val);
                std::cout << "Reader2: Got " << val << " (total: " << r2_values.size() << ")" << std::endl;
            }
        }
        std::cout << "Reader2: Finished" << std::endl;
        co_return;
    };

    auto reader3 = [&ch, &r3_values]() -> co_t {
        std::cout << "Reader3: Starting endless loop..." << std::endl;
        for (int loop = 1; loop <= 9; loop++) {
            auto result = co_await ch.read();
            if (result.has_value()) {
                int val = result.value();
                r3_values.push_back(val);
                std::cout << "Reader3: Got " << val << " (total: " << r3_values.size() << ")" << std::endl;
            }
        }
        std::cout << "Reader3: Finished" << std::endl;
        co_return;
    };

    co_t w_coro = writer();
    co_t r1_coro = reader1();
    co_t r2_coro = reader2();
    co_t r3_coro = reader3();

    std::cout << "=== Resume Order: Writer, Reader1, Reader2, Reader3 ===" << std::endl;
    w_coro.resume();
    r1_coro.resume();
    r2_coro.resume();
    r3_coro.resume();

    scheduler_t::instance().run();

    std::cout << "\n=== Buffered Channel Results ===" << std::endl;
    std::cout << "Reader1 got " << r1_values.size() << " values: ";
    for (int v : r1_values) std::cout << v << " ";
    std::cout << std::endl;

    std::cout << "Reader2 got " << r2_values.size() << " values: ";
    for (int v : r2_values) std::cout << v << " ";
    std::cout << std::endl;

    std::cout << "Reader3 got " << r3_values.size() << " values: ";
    for (int v : r3_values) std::cout << v << " ";
    std::cout << std::endl;

    size_t max_diff = std::max({r1_values.size(), r2_values.size(), r3_values.size()}) -
                      std::min({r1_values.size(), r2_values.size(), r3_values.size()});

    std::cout << "\n=== Analysis ===" << std::endl;
    if (max_diff <= 1) {
        std::cout << "✓ FAIR: Buffered channels achieve fair distribution!" << std::endl;
        std::cout << "Reason: Buffer allows multiple values to be available simultaneously" << std::endl;
    } else if (r1_values.size() == 9 && r2_values.size() == 0 && r3_values.size() == 0) {
        std::cout << "✗ MONOPOLY: Reader1 monopolizes buffered channels too!" << std::endl;
        std::cout << "Reason: Same ping-pong effect as unbuffered channels" << std::endl;
    } else {
        std::cout << "~ MIXED: Partial fairness, difference = " << max_diff << std::endl;
    }
}

// Test different buffered channel scenarios
void test_buffered_channel_variations() {
    std::cout << "\n\n=== Testing Buffered Channel Variations ===" << std::endl;

    // Test 1: Large buffer
    std::cout << "\n--- Test 1: Large Buffer (capacity 10) ---" << std::endl;
    chan_t<int> ch1(10);
    std::vector<int> r1_vals, r2_vals, r3_vals;

    auto writer1 = [&ch1]() -> co_t {
        for (int i = 1; i <= 9; i++) {
            co_await ch1.write(i);
        }
        co_return;
    };

    auto reader1_1 = [&ch1, &r1_vals]() -> co_t {
        for (int i = 0; i < 9; i++) {
            auto result = co_await ch1.read();
            if (result.has_value()) {
                r1_vals.push_back(result.value());
                std::cout << "R1: " << result.value() << " ";
            }
        }
        co_return;
    };

    auto reader1_2 = [&ch1, &r2_vals]() -> co_t {
        for (int i = 0; i < 9; i++) {
            auto result = co_await ch1.read();
            if (result.has_value()) {
                r2_vals.push_back(result.value());
                std::cout << "R2: " << result.value() << " ";
            }
        }
        co_return;
    };

    auto reader1_3 = [&ch1, &r3_vals]() -> co_t {
        for (int i = 0; i < 9; i++) {
            auto result = co_await ch1.read();
            if (result.has_value()) {
                r3_vals.push_back(result.value());
                std::cout << "R3: " << result.value() << " ";
            }
        }
        co_return;
    };

    co_t w1 = writer1();
    co_t r1_1 = reader1_1();
    co_t r1_2 = reader1_2();
    co_t r1_3 = reader1_3();

    w1.resume();
    r1_1.resume();
    r1_2.resume();
    r1_3.resume();

    scheduler_t::instance().run();

    std::cout << std::endl;
    std::cout << "Large buffer results: R1=" << r1_vals.size()
              << ", R2=" << r2_vals.size()
              << ", R3=" << r3_vals.size() << std::endl;
}

int main() {
    test_even_consumption_scenario();
    test_different_resume_order();
    test_buffered_channel_consumption();
    test_endless_reading_loops();
    test_endless_loops_with_yielding();
    test_reader_dominance_analysis();
    test_fair_endless_loops();
    test_scheduler_queue_tracing();
    test_buffered_channel_monopoly();
    test_buffered_channel_variations();
    return 0;
}
