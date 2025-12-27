#include "../coco.h"
#include <iostream>

using namespace coco;

int main() {
    std::cout << "Testing scheduler clear..." << std::endl;
    
    // Test 1: Basic coroutine
    {
        std::cout << "Test 1: Basic coroutine" << std::endl;
        auto simple_coro = []() -> co_t {
            std::cout << "Simple coroutine executing" << std::endl;
            co_await std::suspend_always{};
            std::cout << "Simple coroutine after suspend" << std::endl;
            co_return;
        };
        
        co_t coro = simple_coro();
        std::cout << "Created coroutine" << std::endl;
        
        coro.resume();
        std::cout << "Called resume()" << std::endl;
        
        std::cout << "About to call scheduler.run()..." << std::endl;
        scheduler_t::instance().run();
        std::cout << "Finished scheduler.run()" << std::endl;
    }
    
    // Test 2: Clear scheduler
    {
        std::cout << "Test 2: Clear scheduler" << std::endl;
        std::cout << "About to call scheduler.run() to clear..." << std::endl;
        scheduler_t::instance().run();
        std::cout << "Finished clearing scheduler" << std::endl;
    }
    
    std::cout << "All tests completed" << std::endl;
    return 0;
}
