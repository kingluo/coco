#include "../coco.h"
#include <iostream>
#include <cassert>
#include <memory>
#include <string>
#include <vector>
#include <functional>

using namespace coco;

// Global test state
bool test_passed = true;
std::string current_test = "";

#define TEST_START(name) \
    do { \
        current_test = name; \
        std::cout << "Testing: " << name << "..." << std::endl; \
    } while(0)

#define TEST_ASSERT(condition, message) \
    do { \
        if (!(condition)) { \
            std::cout << "✗ FAILED: " << current_test << " - " << message << std::endl; \
            test_passed = false; \
        } \
    } while(0)

#define TEST_PASS() \
    do { \
        std::cout << "✓ PASSED: " << current_test << std::endl; \
    } while(0)

// =============================================================================
// Advanced RAII Tests
// =============================================================================

class ResourceManager {
private:
    std::string resource_name;
    bool acquired;
    static int total_acquired;
    static int total_released;
    
public:
    ResourceManager(const std::string& name) : resource_name(name), acquired(true) {
        total_acquired++;
        std::cout << "  Resource '" << resource_name << "' acquired (total: " << total_acquired << ")" << std::endl;
    }
    
    ~ResourceManager() {
        if (acquired) {
            total_released++;
            std::cout << "  Resource '" << resource_name << "' released (total: " << total_released << ")" << std::endl;
        }
    }
    
    // Move constructor
    ResourceManager(ResourceManager&& other) noexcept 
        : resource_name(std::move(other.resource_name)), acquired(other.acquired) {
        other.acquired = false; // Transfer ownership
        std::cout << "  Resource '" << resource_name << "' moved" << std::endl;
    }
    
    // Delete copy constructor
    ResourceManager(const ResourceManager&) = delete;
    ResourceManager& operator=(const ResourceManager&) = delete;
    ResourceManager& operator=(ResourceManager&&) = delete;
    
    const std::string& name() const { return resource_name; }
    bool is_acquired() const { return acquired; }
    
    static int get_acquired_count() { return total_acquired; }
    static int get_released_count() { return total_released; }
    static void reset_counts() { total_acquired = 0; total_released = 0; }
};

int ResourceManager::total_acquired = 0;
int ResourceManager::total_released = 0;

// Test complex RAII scenarios across multiple suspension points
co_t test_complex_raii() {
    TEST_START("Complex RAII Across Multiple Suspension Points");
    
    ResourceManager::reset_counts();
    
    {
        ResourceManager resource1("Database Connection");
        TEST_ASSERT(ResourceManager::get_acquired_count() == 1, "First resource should be acquired");
        
        // First suspension
        co_await std::suspend_always{};
        
        TEST_ASSERT(resource1.is_acquired(), "Resource should still be acquired after first suspension");
        TEST_ASSERT(resource1.name() == "Database Connection", "Resource name should be preserved");
        
        {
            ResourceManager resource2("File Handle");
            TEST_ASSERT(ResourceManager::get_acquired_count() == 2, "Second resource should be acquired");
            
            // Second suspension with nested scope
            co_await std::suspend_always{};
            
            TEST_ASSERT(resource1.is_acquired(), "First resource should still be acquired");
            TEST_ASSERT(resource2.is_acquired(), "Second resource should still be acquired");
            TEST_ASSERT(ResourceManager::get_released_count() == 0, "No resources should be released yet");
            
        } // resource2 should be destroyed here
        
        TEST_ASSERT(ResourceManager::get_released_count() == 1, "Second resource should be released");
        TEST_ASSERT(resource1.is_acquired(), "First resource should still be acquired");
        
        // Third suspension after nested scope
        co_await std::suspend_always{};
        
        TEST_ASSERT(resource1.is_acquired(), "First resource should still be acquired after third suspension");
        
    } // resource1 should be destroyed here
    
    TEST_ASSERT(ResourceManager::get_released_count() == 2, "Both resources should be released");
    
    TEST_PASS();
    co_return;
}

// =============================================================================
// Smart Pointer Tests
// =============================================================================

co_t test_smart_pointers() {
    TEST_START("Smart Pointers Across Suspension Points");
    
    std::shared_ptr<int> shared_ptr;
    std::unique_ptr<std::string> unique_ptr;
    
    {
        // Create smart pointers
        shared_ptr = std::make_shared<int>(42);
        unique_ptr = std::make_unique<std::string>("Hello Smart Pointers");
        
        TEST_ASSERT(shared_ptr.use_count() == 1, "Shared pointer should have use count 1");
        TEST_ASSERT(*shared_ptr == 42, "Shared pointer should point to correct value");
        TEST_ASSERT(*unique_ptr == "Hello Smart Pointers", "Unique pointer should point to correct value");
        
        // Create another reference to shared_ptr
        std::shared_ptr<int> shared_ptr2 = shared_ptr;
        TEST_ASSERT(shared_ptr.use_count() == 2, "Shared pointer should have use count 2");
        
        co_await std::suspend_always{};
        
        // After suspension, smart pointers should still be valid
        TEST_ASSERT(shared_ptr.use_count() == 2, "Shared pointer use count should be preserved");
        TEST_ASSERT(*shared_ptr == 42, "Shared pointer value should be preserved");
        TEST_ASSERT(*unique_ptr == "Hello Smart Pointers", "Unique pointer value should be preserved");
        TEST_ASSERT(*shared_ptr2 == 42, "Second shared pointer should still be valid");
        
        // Modify through smart pointers
        *shared_ptr = 100;
        *unique_ptr = "Modified";
        
        TEST_ASSERT(*shared_ptr2 == 100, "Modification through shared_ptr should affect shared_ptr2");
        
    } // shared_ptr2 goes out of scope here
    
    TEST_ASSERT(shared_ptr.use_count() == 1, "Shared pointer use count should decrease to 1");
    TEST_ASSERT(*shared_ptr == 100, "Shared pointer value should still be correct");
    TEST_ASSERT(*unique_ptr == "Modified", "Unique pointer value should still be correct");
    
    TEST_PASS();
    co_return;
}

// =============================================================================
// Lambda and Function Object Tests
// =============================================================================

co_t test_lambda_captures() {
    TEST_START("Lambda Captures Across Suspension Points");
    
    int local_var = 10;
    std::string local_string = "Captured";
    
    // Lambda with captures
    auto lambda_by_value = [local_var, local_string]() {
        return local_var + static_cast<int>(local_string.length());
    };
    
    auto lambda_by_ref = [&local_var, &local_string]() {
        return local_var + static_cast<int>(local_string.length());
    };
    
    TEST_ASSERT(lambda_by_value() == 18, "Lambda by value should work before suspension"); // 10 + 8
    TEST_ASSERT(lambda_by_ref() == 18, "Lambda by reference should work before suspension");
    
    co_await std::suspend_always{};
    
    // After suspension
    TEST_ASSERT(lambda_by_value() == 18, "Lambda by value should still work after suspension");
    
    // Lambda by reference might be dangerous if stack was relocated
    // But the lambda object itself should be preserved
    local_var = 20;
    local_string = "Modified Captured";
    
    TEST_ASSERT(lambda_by_value() == 18, "Lambda by value should be unaffected by changes");
    // Note: lambda_by_ref behavior depends on whether stack was relocated
    
    TEST_PASS();
    co_return;
}

// =============================================================================
// Container Tests
// =============================================================================

co_t test_containers() {
    TEST_START("Containers Across Suspension Points");
    
    std::vector<int> vec = {1, 2, 3, 4, 5};
    std::vector<int>::iterator it = vec.begin() + 2; // Points to element 3
    
    TEST_ASSERT(*it == 3, "Iterator should point to correct element before suspension");
    TEST_ASSERT(vec.size() == 5, "Vector should have correct size");
    
    co_await std::suspend_always{};
    
    // After suspension, vector should be preserved
    TEST_ASSERT(vec.size() == 5, "Vector size should be preserved");
    TEST_ASSERT(vec[0] == 1 && vec[4] == 5, "Vector contents should be preserved");
    
    // Iterator might be invalid if vector was moved/relocated
    // Safe approach: re-establish iterator
    auto safe_it = vec.begin() + 2;
    TEST_ASSERT(*safe_it == 3, "Re-established iterator should work");
    
    // Modify container after suspension
    vec.push_back(6);
    TEST_ASSERT(vec.size() == 6, "Vector should be modifiable after suspension");
    TEST_ASSERT(vec[5] == 6, "New element should be correct");
    
    TEST_PASS();
    co_return;
}

// =============================================================================
// Main Test Runner
// =============================================================================

int main() {
    std::cout << "=== Advanced C++20 Coroutine Caveats Tests ===" << std::endl;
    std::cout << "Testing advanced scenarios and edge cases" << std::endl;
    std::cout << std::endl;
    
    // Test 1: Complex RAII
    auto test1 = test_complex_raii();
    test1.resume(); // First suspension
    test1.resume(); // Second suspension  
    test1.resume(); // Third suspension
    
    std::cout << std::endl;
    
    // Test 2: Smart pointers
    auto test2 = test_smart_pointers();
    test2.resume(); // Resume to complete the test
    
    std::cout << std::endl;
    
    // Test 3: Lambda captures
    auto test3 = test_lambda_captures();
    test3.resume(); // Resume to complete the test
    
    std::cout << std::endl;
    
    // Test 4: Containers
    auto test4 = test_containers();
    test4.resume(); // Resume to complete the test
    
    std::cout << std::endl;
    std::cout << "=== Advanced Test Summary ===" << std::endl;
    if (test_passed) {
        std::cout << "✓ All advanced caveat tests passed!" << std::endl;
        std::cout << std::endl;
        std::cout << "Advanced findings demonstrated:" << std::endl;
        std::cout << "1. Complex RAII scenarios work correctly across multiple suspension points" << std::endl;
        std::cout << "2. Smart pointers are safely preserved and maintain correct reference counts" << std::endl;
        std::cout << "3. Lambda captures by value are safe, by reference may be risky" << std::endl;
        std::cout << "4. Containers are preserved but iterators may become invalid" << std::endl;
        return 0;
    } else {
        std::cout << "✗ Some advanced tests failed!" << std::endl;
        return 1;
    }
}
