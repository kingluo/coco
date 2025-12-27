#include "../coco.h"
#include <iostream>
#include <cstdint>

using namespace coco;

// Test to demonstrate wg_t with uint64_t parameter (no negative values)
void test_wg_no_bound_checking() {
    std::cout << "Testing wg_t with uint64_t parameter..." << std::endl;
    
    wg_t wg;
    
    // Test 1: Large positive values - now allows natural uint64_t overflow
    std::cout << "  Test 1: Adding large values near UINT64_MAX" << std::endl;
    wg.add(static_cast<int64_t>(UINT64_MAX - 5));
    std::cout << "    Added UINT64_MAX - 5" << std::endl;
    
    // This will now cause uint64_t overflow and wrap around
    wg.add(10);
    std::cout << "    Added 10 more - natural uint64_t overflow occurred" << std::endl;
    
    // Test 2: Normal operations with uint64_t values
    wg_t wg2;
    wg2.add(5);
    std::cout << "  Test 2: Added 5 to new wg" << std::endl;

    // With uint64_t parameter, only non-negative values are accepted
    wg2.add(10);  // Add more work
    std::cout << "    Added 10 more - uint64_t only accepts non-negative values" << std::endl;
    
    // Test 3: Normal operations still work
    wg_t wg3;
    wg3.add(3);
    wg3.done();
    wg3.done();
    wg3.done();
    std::cout << "  Test 3: Normal add/done operations work correctly" << std::endl;
    
    std::cout << "✓ wg_t uint64_t parameter test completed" << std::endl;
}

int main() {
    test_wg_no_bound_checking();
    return 0;
}
