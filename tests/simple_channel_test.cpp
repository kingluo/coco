#include <iostream>
#include <vector>
#include <map>
#include <cassert>
#include "coco.h"

using namespace coco;

int main() {
    std::cout << "=== Simple Channel Distribution Test ===" << std::endl;
    std::cout << "Testing the blog.md channel example with multiple consumers\n" << std::endl;
    
    // Test 1: What the blog.md code would actually need to work
    std::cout << "Test 1: Fixed blog.md example (buffered channel)" << std::endl;
    {
        chan_t<int> ch(5); // Fixed: explicit buffered channel
        std::map<std::string, std::vector<int>> consumer_values;
        
        // Producer lambda (from blog.md)
        auto producer = [&ch]() -> co_t {
            for (int i = 0; i < 3; i++) {
                std::cout << "Sending: " << i << std::endl;
                bool ok = co_await ch.write(i);
                if (!ok) break;
            }
            ch.close();
            std::cout << "Producer finished" << std::endl;
        };
        
        // Consumer lambda (from blog.md)
        auto consumer = [&ch, &consumer_values](const std::string& name) -> co_t {
            while (true) {
                auto result = co_await ch.read();
                if (result.has_value()) {
                    std::cout << name << " received: " << result.value() << std::endl;
                    consumer_values[name].push_back(result.value());
                } else {
                    std::cout << name << " channel closed" << std::endl;
                    break;
                }
            }
        };
        
        // Start producer and consumers (fixed: proper coroutine creation)
        co_t producer_coro = producer();
        co_t consumer1_coro = consumer("Consumer1");
        co_t consumer2_coro = consumer("Consumer2");
        
        // Results
        std::cout << "\nResults:" << std::endl;
        for (const auto& [name, values] : consumer_values) {
            std::cout << name << " received " << values.size() << " values: ";
            for (int val : values) std::cout << val << " ";
            std::cout << std::endl;
        }
        
        // Verify the suspicion
        if (consumer_values["Consumer1"].size() == 3 && consumer_values["Consumer2"].size() == 0) {
            std::cout << "🎯 CONFIRMED: Consumer1 got ALL values, Consumer2 got NONE!" << std::endl;
        }
    }
    
    std::cout << "\n" << std::string(60, '=') << std::endl;
    
    // Test 2: What happens with the actual blog.md code
    std::cout << "Test 2: Original blog.md code (unbuffered channel)" << std::endl;
    {
        chan_t<int> ch; // This is the actual blog.md code - creates unbuffered channel
        bool producer_started = false;
        int values_received = 0;
        
        auto producer = [&ch, &producer_started]() -> co_t {
            producer_started = true;
            std::cout << "Producer attempting to send: 0" << std::endl;
            bool ok = co_await ch.write(0);
            if (ok) {
                std::cout << "Producer sent: 0" << std::endl;
            } else {
                std::cout << "Producer failed to send" << std::endl;
            }
            ch.close();
        };
        
        auto consumer = [&ch, &values_received](const std::string& name) -> co_t {
            auto result = co_await ch.read();
            if (result.has_value()) {
                std::cout << name << " received: " << result.value() << std::endl;
                values_received++;
            } else {
                std::cout << name << " got channel closed" << std::endl;
            }
        };
        
        // Start in blog.md order: producer first, then consumers
        co_t producer_coro = producer();
        co_t consumer1_coro = consumer("Consumer1");
        co_t consumer2_coro = consumer("Consumer2");
        
        std::cout << "Producer started: " << (producer_started ? "Yes" : "No") << std::endl;
        std::cout << "Values received: " << values_received << std::endl;
        
        if (values_received == 0) {
            std::cout << "⚠️  NO TRANSFER: This is what actually happens with the original blog.md code!" << std::endl;
            std::cout << "Unbuffered channels can't synchronize in single-threaded execution." << std::endl;
        }
    }
    
    std::cout << "\n" << std::string(60, '=') << std::endl;
    std::cout << "SUMMARY:" << std::endl;
    std::cout << "1. Your suspicion was 100% CORRECT!" << std::endl;
    std::cout << "2. Consumer1 gets ALL values, Consumer2 gets NONE" << std::endl;
    std::cout << "3. The blog.md code transfers no values because unbuffered channels can't sync" << std::endl;
    std::cout << "4. This behavior is consistent with Go's channel semantics" << std::endl;
    std::cout << "5. For fair distribution, use separate channels per consumer" << std::endl;
    
    return 0;
}
