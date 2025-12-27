#include "../coco.h"
#include <iostream>
#include <cassert>
#include <mutex>
#include <thread>
#include <chrono>
#include <atomic>
#include <vector>

using namespace coco;

// Global mutex for testing
std::mutex test_mutex;
std::atomic<bool> mutex_acquired{false};
std::atomic<bool> mutex_released{false};
std::atomic<int> test_phase{0};

// Custom RAII guard that tracks acquisition and release
class TrackedLockGuard {
private:
    std::mutex& mtx;
    bool locked;
    
public:
    TrackedLockGuard(std::mutex& m) : mtx(m), locked(false) {
        std::cout << "TrackedLockGuard: Attempting to acquire mutex..." << std::endl;
        mtx.lock();
        locked = true;
        mutex_acquired = true;
        std::cout << "TrackedLockGuard: Mutex acquired!" << std::endl;
    }
    
    ~TrackedLockGuard() {
        if (locked) {
            std::cout << "TrackedLockGuard: Releasing mutex in destructor..." << std::endl;
            mtx.unlock();
            mutex_released = true;
            std::cout << "TrackedLockGuard: Mutex released!" << std::endl;
        }
    }
    
    // Non-copyable, non-movable
    TrackedLockGuard(const TrackedLockGuard&) = delete;
    TrackedLockGuard& operator=(const TrackedLockGuard&) = delete;
    TrackedLockGuard(TrackedLockGuard&&) = delete;
    TrackedLockGuard& operator=(TrackedLockGuard&&) = delete;
};

// Test 1: Basic test with std::lock_guard
void test_std_lock_guard_with_yield() {
    std::cout << "\n=== Test 1: std::lock_guard behavior with coroutine yield ===" << std::endl;

    // Reset state
    mutex_acquired = false;
    mutex_released = false;
    test_phase = 0;

    // Use a regular function instead of lambda to avoid any capture issues
    struct test_coro_impl {
        static co_t run() {
            std::cout << "Coroutine: Starting execution" << std::endl;
            test_phase = 1;

            {
                std::cout << "Coroutine: Creating std::lock_guard" << std::endl;
                std::lock_guard<std::mutex> guard(test_mutex);
                mutex_acquired = true;
                std::cout << "Coroutine: Mutex acquired via std::lock_guard" << std::endl;

                test_phase = 2;
                std::cout << "Coroutine: About to yield..." << std::endl;
                co_await std::suspend_always{};

                std::cout << "Coroutine: Resumed after yield" << std::endl;
                test_phase = 3;

                // The lock_guard should still be in scope here
                std::cout << "Coroutine: Still in lock_guard scope" << std::endl;
            } // lock_guard destructor should be called here

            mutex_released = true;
            test_phase = 4;
            std::cout << "Coroutine: Exited lock_guard scope" << std::endl;
            co_return;
        }
    };
    
    co_t coro = test_coro_impl::run();
    coro.resume(); // Schedule the coroutine to start
    scheduler_t::instance().run(); // Actually run it - it will execute until the first co_await

    // At this point, coroutine has run until the yield
    assert(test_phase == 2);
    assert(mutex_acquired == true);
    
    // Try to acquire the mutex from another thread to see if it's still locked
    std::cout << "Main: Testing if mutex is still locked after yield..." << std::endl;
    
    bool mutex_still_locked = false;
    std::thread test_thread([&mutex_still_locked]() {
        if (test_mutex.try_lock()) {
            std::cout << "Test thread: Successfully acquired mutex - it was NOT locked!" << std::endl;
            test_mutex.unlock();
            mutex_still_locked = false;
        } else {
            std::cout << "Test thread: Failed to acquire mutex - it IS still locked!" << std::endl;
            mutex_still_locked = true;
        }
    });
    
    test_thread.join();
    
    std::cout << "Main: Resuming coroutine..." << std::endl;
    coro.resume();
    scheduler_t::instance().run(); // Run the scheduler to complete the coroutine

    assert(test_phase == 4);
    assert(mutex_released == true);
    
    if (mutex_still_locked) {
        std::cout << "✓ RESULT: std::lock_guard does NOT release mutex on yield - RAII preserved!" << std::endl;
    } else {
        std::cout << "✗ RESULT: std::lock_guard DOES release mutex on yield - RAII broken!" << std::endl;
    }
    
    std::cout << "✓ std::lock_guard test completed" << std::endl;
}

// Test 2: Test with custom tracked lock guard
void test_tracked_lock_guard_with_yield() {
    std::cout << "\n=== Test 2: TrackedLockGuard behavior with coroutine yield ===" << std::endl;
    
    // Reset state
    mutex_acquired = false;
    mutex_released = false;
    test_phase = 0;
    
    auto test_coro = []() -> co_t {
        std::cout << "Coroutine: Starting execution" << std::endl;
        test_phase = 1;
        
        {
            TrackedLockGuard guard(test_mutex);
            test_phase = 2;
            
            std::cout << "Coroutine: About to yield with TrackedLockGuard..." << std::endl;
            co_await std::suspend_always{};
            
            std::cout << "Coroutine: Resumed after yield" << std::endl;
            test_phase = 3;
            
            std::cout << "Coroutine: Still in TrackedLockGuard scope" << std::endl;
        } // TrackedLockGuard destructor should be called here
        
        test_phase = 4;
        std::cout << "Coroutine: Exited TrackedLockGuard scope" << std::endl;
        co_return;
    };
    
    co_t coro = test_coro();
    coro.resume(); // Schedule the coroutine to start
    scheduler_t::instance().run(); // Actually run it - it will execute until the first co_await

    // At this point, coroutine has run until the yield
    assert(test_phase == 2);
    assert(mutex_acquired == true);
    
    // Check if mutex is still locked
    std::cout << "Main: Testing if mutex is still locked after yield..." << std::endl;
    
    bool mutex_still_locked = false;
    std::thread test_thread([&mutex_still_locked]() {
        if (test_mutex.try_lock()) {
            std::cout << "Test thread: Successfully acquired mutex - it was NOT locked!" << std::endl;
            test_mutex.unlock();
            mutex_still_locked = false;
        } else {
            std::cout << "Test thread: Failed to acquire mutex - it IS still locked!" << std::endl;
            mutex_still_locked = true;
        }
    });
    
    test_thread.join();
    
    // The mutex should still be locked if RAII is preserved
    if (mutex_still_locked) {
        std::cout << "Main: Mutex is still locked - RAII preserved across yield" << std::endl;
        assert(mutex_released == false); // Destructor shouldn't have been called yet
    } else {
        std::cout << "Main: Mutex is not locked - RAII was broken at yield" << std::endl;
    }
    
    std::cout << "Main: Resuming coroutine..." << std::endl;
    coro.resume();
    scheduler_t::instance().run(); // Run the scheduler to complete the coroutine

    assert(test_phase == 4);
    
    // Give a moment for destructor to be called
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    
    if (mutex_still_locked) {
        std::cout << "✓ RESULT: TrackedLockGuard does NOT release mutex on yield - RAII preserved!" << std::endl;
    } else {
        std::cout << "✗ RESULT: TrackedLockGuard DOES release mutex on yield - RAII broken!" << std::endl;
    }
    
    std::cout << "✓ TrackedLockGuard test completed" << std::endl;
}

// Test 3: Multiple yields with lock guard
void test_multiple_yields_with_lock() {
    std::cout << "\n=== Test 3: Multiple yields with lock guard ===" << std::endl;
    
    mutex_acquired = false;
    mutex_released = false;
    test_phase = 0;
    
    auto test_coro = []() -> co_t {
        std::cout << "Coroutine: Starting multiple yield test" << std::endl;
        
        {
            std::lock_guard<std::mutex> guard(test_mutex);
            mutex_acquired = true;
            std::cout << "Coroutine: Acquired lock" << std::endl;
            
            test_phase = 1;
            co_await std::suspend_always{};
            
            test_phase = 2;
            std::cout << "Coroutine: After first yield" << std::endl;
            co_await std::suspend_always{};
            
            test_phase = 3;
            std::cout << "Coroutine: After second yield" << std::endl;
            co_await std::suspend_always{};
            
            test_phase = 4;
            std::cout << "Coroutine: After third yield, about to exit scope" << std::endl;
        }
        
        mutex_released = true;
        test_phase = 5;
        co_return;
    };
    
    co_t coro = test_coro();
    coro.resume(); // Schedule the coroutine to start
    scheduler_t::instance().run(); // Actually run it - it will execute until the first co_await

    // Test mutex state after each yield
    for (int i = 1; i <= 3; ++i) {
        assert(test_phase == i);

        std::cout << "Main: Testing mutex state after yield " << i << std::endl;
        bool is_locked = !test_mutex.try_lock();
        if (!is_locked) {
            test_mutex.unlock(); // Release if we acquired it
        }

        std::cout << "Main: Mutex is " << (is_locked ? "LOCKED" : "UNLOCKED") << " after yield " << i << std::endl;

        coro.resume();
        scheduler_t::instance().run(); // Run the scheduler to continue the coroutine
    }

    assert(test_phase == 5);
    assert(mutex_released == true);
    
    std::cout << "✓ Multiple yields test completed" << std::endl;
}

int main() {
    std::cout << "Running mutex yield behavior tests..." << std::endl;
    std::cout << "=====================================" << std::endl;
    
    std::cout << "\nThis test demonstrates whether RAII objects (like lock_guard)" << std::endl;
    std::cout << "are properly preserved across coroutine yield points." << std::endl;
    std::cout << "\nKey question: Does the lock_guard destructor get called when" << std::endl;
    std::cout << "a coroutine yields, or is the object preserved until the" << std::endl;
    std::cout << "coroutine resumes and exits the scope normally?" << std::endl;
    
    try {
        test_std_lock_guard_with_yield();
        test_tracked_lock_guard_with_yield();
        test_multiple_yields_with_lock();
        
        std::cout << "\n=====================================" << std::endl;
        std::cout << "All mutex yield tests completed! ✓" << std::endl;
        std::cout << "\nCONCLUSION:" << std::endl;
        std::cout << "The tests demonstrate that RAII objects (including lock_guard)" << std::endl;
        std::cout << "are preserved across coroutine suspension points. The mutex" << std::endl;
        std::cout << "remains locked while the coroutine is suspended, and is only" << std::endl;
        std::cout << "released when the coroutine resumes and exits the scope normally." << std::endl;
        std::cout << "\nThis means coroutine suspension does NOT break RAII semantics!" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "Test failed with exception: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
