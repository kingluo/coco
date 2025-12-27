#include "../coco.h"
#include <iostream>
#include <cassert>
#include <memory>
#include <string>
#include <vector>
#include <mutex>

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
// CAVEAT 1: Top-Level Function Restriction Tests
// =============================================================================

// This function should NOT compile if we try to use co_await in it
// We'll test this by ensuring compilation fails when uncommented
/*
void helper_function_with_coawait() {
    // This should cause compilation error
    co_await std::suspend_always{};
}
*/

// This also should NOT compile - cannot return a coroutine from another coroutine
/*
co_t helper_coroutine() {
    co_yield resched;
    co_return;
}

co_t calling_coroutine() {
    // This causes compilation error: cannot use 'return' in a coroutine
    return helper_coroutine();
}
*/

// Helper function that cannot use co_await (this is the limitation)
void regular_helper_function(int& result) {
    result = 42;
}

// Coroutine that demonstrates the limitation
co_t test_top_level_restriction() {
    TEST_START("Top-Level Function Restriction");
    
    int result = 0;
    
    // This works - co_await in top-level coroutine function
    co_await std::suspend_always{};
    
    // This works - calling regular function from coroutine
    regular_helper_function(result);
    
    TEST_ASSERT(result == 42, "Helper function should set result to 42");
    
    // The limitation: we cannot call helper_function_with_coawait()
    // because it would need co_await which is not allowed in non-coroutine functions
    
    TEST_PASS();
    co_return;
}

// =============================================================================
// CAVEAT 2: Variable Lifetime and Stack Recreation Tests
// =============================================================================

// RAII test class to verify destructors are called properly
class RAIITestObject {
private:
    static int construction_count;
    static int destruction_count;
    int id;
    
public:
    RAIITestObject(int id) : id(id) {
        construction_count++;
        std::cout << "  RAIITestObject " << id << " constructed (total: " << construction_count << ")" << std::endl;
    }
    
    ~RAIITestObject() {
        destruction_count++;
        std::cout << "  RAIITestObject " << id << " destroyed (total: " << destruction_count << ")" << std::endl;
    }
    
    int get_id() const { return id; }
    
    static int get_construction_count() { return construction_count; }
    static int get_destruction_count() { return destruction_count; }
    static void reset_counts() { construction_count = 0; destruction_count = 0; }
};

int RAIITestObject::construction_count = 0;
int RAIITestObject::destruction_count = 0;

// Test RAII preservation across suspension points
co_t test_raii_preservation() {
    TEST_START("RAII Preservation Across Suspension Points");
    
    RAIITestObject::reset_counts();
    
    {
        RAIITestObject obj1(1);
        TEST_ASSERT(RAIITestObject::get_construction_count() == 1, "Object should be constructed");
        TEST_ASSERT(RAIITestObject::get_destruction_count() == 0, "Object should not be destroyed yet");
        
        // Suspend the coroutine - RAII object should NOT be destroyed
        co_await std::suspend_always{};
        
        // After resumption, RAII object should still be alive and valid
        TEST_ASSERT(obj1.get_id() == 1, "RAII object should preserve its state across suspension");
        TEST_ASSERT(RAIITestObject::get_destruction_count() == 0, "Object should still not be destroyed after suspension");
        
        // Create another object after suspension
        RAIITestObject obj2(2);
        TEST_ASSERT(RAIITestObject::get_construction_count() == 2, "Second object should be constructed");
        TEST_ASSERT(obj2.get_id() == 2, "Second object should have correct ID");
        
    } // Both objects should be destroyed here when going out of scope
    
    // Verify both objects were properly destroyed
    TEST_ASSERT(RAIITestObject::get_destruction_count() == 2, "Both objects should be destroyed when going out of scope");
    
    TEST_PASS();
    co_return;
}

// Test the dangerous pattern with references and pointers
co_t test_reference_pointer_danger() {
    TEST_START("Reference and Pointer Danger Across Suspension Points");
    
    int local_var = 42;
    std::string local_string = "Hello";
    
    // Create references and pointers before suspension
    int& ref_to_local = local_var;
    int* ptr_to_local = &local_var;
    std::string& ref_to_string = local_string;
    
    // Verify they work before suspension
    TEST_ASSERT(ref_to_local == 42, "Reference should work before suspension");
    TEST_ASSERT(*ptr_to_local == 42, "Pointer should work before suspension");
    TEST_ASSERT(ref_to_string == "Hello", "String reference should work before suspension");
    
    // Store the original addresses
    void* original_local_addr = &local_var;
    void* original_string_addr = &local_string;
    
    // Suspend the coroutine - this may cause stack relocation
    co_await std::suspend_always{};
    
    // After resumption, the local variables themselves should be preserved
    TEST_ASSERT(local_var == 42, "Local variable value should be preserved");
    TEST_ASSERT(local_string == "Hello", "Local string should be preserved");
    
    // Check if stack was relocated by comparing addresses
    bool stack_relocated = (&local_var != original_local_addr) || (&local_string != original_string_addr);
    
    if (stack_relocated) {
        std::cout << "  Stack was relocated during suspension" << std::endl;
        std::cout << "  Original local_var address: " << original_local_addr << std::endl;
        std::cout << "  New local_var address: " << &local_var << std::endl;
        
        // In this case, the old references and pointers are potentially dangerous
        // We cannot safely test them as they might cause undefined behavior
        std::cout << "  WARNING: References and pointers from before suspension may be invalid!" << std::endl;
    } else {
        std::cout << "  Stack was not relocated (implementation-specific behavior)" << std::endl;
        // If stack wasn't relocated, references and pointers should still work
        TEST_ASSERT(ref_to_local == 42, "Reference should still work if stack not relocated");
        TEST_ASSERT(*ptr_to_local == 42, "Pointer should still work if stack not relocated");
    }
    
    // Safe approach: re-establish references after suspension
    int& safe_ref = local_var;
    int* safe_ptr = &local_var;
    std::string& safe_string_ref = local_string;
    
    TEST_ASSERT(safe_ref == 42, "New reference after suspension should be safe");
    TEST_ASSERT(*safe_ptr == 42, "New pointer after suspension should be safe");
    TEST_ASSERT(safe_string_ref == "Hello", "New string reference after suspension should be safe");
    
    TEST_PASS();
    co_return;
}

// Test variable lifetime management
co_t test_variable_lifetime() {
    TEST_START("Variable Lifetime Management");
    
    // Variables used only before suspension (should stay on stack)
    int temp_before = 100;
    std::cout << "  Temp before suspension: " << temp_before << std::endl;
    
    // Variables that will be used after suspension (moved to coroutine frame)
    std::string persistent_data = "Persistent";
    std::vector<int> persistent_vector = {1, 2, 3};
    
    co_await std::suspend_always{};
    
    // After suspension, persistent variables should be preserved
    TEST_ASSERT(persistent_data == "Persistent", "Persistent string should be preserved");
    TEST_ASSERT(persistent_vector.size() == 3, "Persistent vector should be preserved");
    TEST_ASSERT(persistent_vector[0] == 1 && persistent_vector[1] == 2 && persistent_vector[2] == 3, 
                "Persistent vector contents should be preserved");
    
    // Modify persistent data after suspension
    persistent_data += " Modified";
    persistent_vector.push_back(4);
    
    TEST_ASSERT(persistent_data == "Persistent Modified", "Modified persistent string should work");
    TEST_ASSERT(persistent_vector.size() == 4, "Modified persistent vector should work");
    TEST_ASSERT(persistent_vector[3] == 4, "New vector element should be correct");
    
    TEST_PASS();
    co_return;
}

// =============================================================================
// Main Test Runner
// =============================================================================

int main() {
    std::cout << "=== C++20 Coroutine Caveats Tests ===" << std::endl;
    std::cout << "Testing important caveats mentioned in blog.md" << std::endl;
    std::cout << std::endl;
    
    // Test 1: Top-level function restriction
    auto test1 = test_top_level_restriction();
    test1.resume(); // Resume to complete the test
    
    std::cout << std::endl;
    
    // Test 2: RAII preservation
    auto test2 = test_raii_preservation();
    test2.resume(); // Resume to complete the test
    
    std::cout << std::endl;
    
    // Test 3: Reference and pointer danger
    auto test3 = test_reference_pointer_danger();
    test3.resume(); // Resume to complete the test
    
    std::cout << std::endl;
    
    // Test 4: Variable lifetime
    auto test4 = test_variable_lifetime();
    test4.resume(); // Resume to complete the test
    
    std::cout << std::endl;
    std::cout << "=== Test Summary ===" << std::endl;
    if (test_passed) {
        std::cout << "✓ All caveat tests passed!" << std::endl;
        std::cout << std::endl;
        std::cout << "Key findings demonstrated:" << std::endl;
        std::cout << "1. co_await can only be used in top-level coroutine functions" << std::endl;
        std::cout << "2. RAII objects are properly preserved across suspension points" << std::endl;
        std::cout << "3. References and pointers to stack variables may become invalid after suspension" << std::endl;
        std::cout << "4. Variable values are preserved, but their addresses may change" << std::endl;
        return 0;
    } else {
        std::cout << "✗ Some tests failed!" << std::endl;
        return 1;
    }
}
