#include "../coco.h"
#include <iostream>
#include <cassert>
#include <vector>
#include <string>

using namespace coco;

// Test 1: Unbuffered channel direct handoff
void test_unbuffered_direct_handoff() {
    std::cout << "=== Test 1: Unbuffered Channel Direct Handoff ===" << std::endl;

    chan_t<int> ch(0); // Unbuffered
    bool sender_completed = false;
    bool receiver_completed = false;
    int received_value = -1;

    std::cout << "Channel cap: " << ch.cap() << ", size: " << ch.size() << std::endl;

    // Sender coroutine
    auto sender = [&]() -> co_t {
        std::cout << "Sender: About to send 42" << std::endl;
        bool ok = co_await ch.write(42);
        std::cout << "Sender: Send completed, ok=" << ok << std::endl;
        sender_completed = true;
        co_return;
    };

    // Receiver coroutine
    auto receiver = [&]() -> co_t {
        std::cout << "Receiver: About to receive" << std::endl;
        auto result = co_await ch.read();
        std::cout << "Receiver: Received, has_value=" << result.has_value() << std::endl;
        if (result.has_value()) {
            received_value = result.value();
            std::cout << "Receiver: Value=" << received_value << std::endl;
        }
        receiver_completed = true;
        co_return;
    };

    // Start receiver first, then sender
    co_t receiver_coro = receiver();
    co_t sender_coro = sender();

    std::cout << "Starting receiver..." << std::endl;
    receiver_coro.resume();

    std::cout << "Starting sender..." << std::endl;
    sender_coro.resume();

    std::cout << "Running scheduler..." << std::endl;
    scheduler_t::instance().run();

    std::cout << "Results: sender_completed=" << sender_completed
              << ", receiver_completed=" << receiver_completed
              << ", received_value=" << received_value << std::endl;

    assert(sender_completed);
    assert(receiver_completed);
    assert(received_value == 42);
    std::cout << "✓ Unbuffered direct handoff test passed" << std::endl;
}

// Test 2: Buffered channel behavior
void test_buffered_channel() {
    std::cout << "\n=== Test 2: Buffered Channel Behavior ===" << std::endl;
    
    chan_t<int> ch(2); // Buffered with capacity 2
    std::vector<int> sent_values;
    std::vector<int> received_values;
    
    auto sender = [&]() -> co_t {
        for (int i = 1; i <= 3; i++) {
            std::cout << "Sender: Sending " << i << std::endl;
            bool ok = co_await ch.write(i);
            if (ok) {
                sent_values.push_back(i);
                std::cout << "Sender: Successfully sent " << i << std::endl;
            } else {
                std::cout << "Sender: Failed to send " << i << std::endl;
                break;
            }
        }
        co_return;
    };
    
    auto receiver = [&]() -> co_t {
        for (int i = 0; i < 3; i++) {
            std::cout << "Receiver: About to receive" << std::endl;
            auto result = co_await ch.read();
            if (result.has_value()) {
                received_values.push_back(result.value());
                std::cout << "Receiver: Received " << result.value() << std::endl;
            } else {
                std::cout << "Receiver: Channel closed" << std::endl;
                break;
            }
        }
        co_return;
    };
    
    co_t sender_coro = sender();
    co_t receiver_coro = receiver();
    
    sender_coro.resume();
    receiver_coro.resume();
    scheduler_t::instance().run();
    
    std::cout << "Sent: ";
    for (int v : sent_values) std::cout << v << " ";
    std::cout << "\nReceived: ";
    for (int v : received_values) std::cout << v << " ";
    std::cout << std::endl;
    
    assert(sent_values.size() == 3);
    assert(received_values.size() == 3);
    assert(sent_values == received_values);
    std::cout << "✓ Buffered channel test passed" << std::endl;
}

// Test 3: Closed channel behavior
void test_closed_channel() {
    std::cout << "\n=== Test 3: Closed Channel Behavior ===" << std::endl;
    
    chan_t<int> ch(1);
    
    auto test_coro = [&]() -> co_t {
        // Send a value
        bool ok1 = co_await ch.write(100);
        assert(ok1);
        std::cout << "Sent 100 to channel" << std::endl;
        
        // Close the channel
        ch.close();
        std::cout << "Channel closed" << std::endl;
        
        // Try to send to closed channel (should fail)
        bool ok2 = co_await ch.write(200);
        assert(!ok2);
        std::cout << "Send to closed channel failed as expected" << std::endl;
        
        // Read the buffered value
        auto result1 = co_await ch.read();
        assert(result1.has_value());
        assert(result1.value() == 100);
        std::cout << "Read buffered value: " << result1.value() << std::endl;
        
        // Read from closed empty channel (should return nullopt)
        auto result2 = co_await ch.read();
        assert(!result2.has_value());
        std::cout << "Read from closed empty channel returned nullopt" << std::endl;
        
        co_return;
    };
    
    co_t coro = test_coro();
    coro.resume();
    scheduler_t::instance().run();
    
    std::cout << "✓ Closed channel test passed" << std::endl;
}

// Test 4: FIFO ordering for multiple readers/writers
void test_fifo_ordering() {
    std::cout << "\n=== Test 4: FIFO Ordering ===" << std::endl;

    chan_t<int> ch(0); // Unbuffered
    std::vector<int> received_order;
    std::vector<std::string> receiver_order;

    auto receiver = [&](const std::string& name) -> co_t {
        auto result = co_await ch.read();
        if (result.has_value()) {
            received_order.push_back(result.value());
            receiver_order.push_back(name);
            std::cout << name << " received: " << result.value() << std::endl;
        }
        co_return;
    };

    auto sender = [&](int value) -> co_t {
        bool ok = co_await ch.write(value);
        std::cout << "Sent " << value << ", ok=" << ok << std::endl;
        co_return;
    };

    // Start multiple receivers first
    co_t r1 = receiver("R1");
    co_t r2 = receiver("R2");
    co_t r3 = receiver("R3");

    // Then start senders
    co_t s1 = sender(10);
    co_t s2 = sender(20);
    co_t s3 = sender(30);

    // Resume in order
    r1.resume();
    r2.resume();
    r3.resume();
    s1.resume();
    s2.resume();
    s3.resume();

    scheduler_t::instance().run();

    std::cout << "Received order: ";
    for (int v : received_order) std::cout << v << " ";
    std::cout << "\nReceiver order: ";
    for (const auto& r : receiver_order) std::cout << r << " ";
    std::cout << std::endl;

    // Should maintain value order (FIFO for values)
    assert(received_order.size() == 3);
    assert(received_order[0] == 10);
    assert(received_order[1] == 20);
    assert(received_order[2] == 30);
    // Note: Receiver order may vary depending on scheduler implementation
    // The important thing is that values are received in correct order

    std::cout << "✓ FIFO ordering test passed" << std::endl;
}

int main() {
    std::cout << "Testing Go Channel Compliance\n" << std::endl;

    test_unbuffered_direct_handoff();
    test_buffered_channel();
    test_closed_channel();
    test_fifo_ordering();

    std::cout << "\n=== All Tests Completed ===" << std::endl;
    return 0;
}
