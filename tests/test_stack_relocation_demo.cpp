#include "../coco.h"
#include <iostream>
#include <cassert>
#include <vector>
#include <string>

using namespace coco;

// This test attempts to demonstrate stack relocation by creating
// a scenario that might trigger it (large stack usage)

co_t test_stack_relocation_with_large_data() {
    std::cout << "=== Stack Relocation Demonstration ===" << std::endl;
    std::cout << "Creating large stack data to potentially trigger relocation..." << std::endl;
    
    // Create large stack-allocated data
    char large_buffer[8192];  // 8KB buffer
    std::vector<int> large_vector(1000, 42);  // Large vector
    std::string large_string(1000, 'X');     // Large string
    
    // Fill buffer with pattern
    for (int i = 0; i < 8192; i++) {
        large_buffer[i] = static_cast<char>(i % 256);
    }
    
    // Store addresses before suspension
    void* buffer_addr = large_buffer;
    void* vector_addr = large_vector.data();
    void* string_addr = const_cast<char*>(large_string.data());
    
    // Create references and pointers
    char& buffer_ref = large_buffer[100];
    int& vector_ref = large_vector[100];

    char* buffer_ptr = &large_buffer[200];
    int* vector_ptr = &large_vector[200];
    
    std::cout << "Before suspension:" << std::endl;
    std::cout << "  Buffer address: " << buffer_addr << std::endl;
    std::cout << "  Vector data address: " << vector_addr << std::endl;
    std::cout << "  String data address: " << string_addr << std::endl;
    std::cout << "  Buffer[100] via reference: " << static_cast<int>(buffer_ref) << std::endl;
    std::cout << "  Vector[100] via reference: " << vector_ref << std::endl;
    std::cout << "  Buffer[200] via pointer: " << static_cast<int>(*buffer_ptr) << std::endl;
    std::cout << "  Vector[200] via pointer: " << *vector_ptr << std::endl;
    
    // Suspend the coroutine
    std::cout << "\nSuspending coroutine..." << std::endl;
    co_await std::suspend_always{};
    
    std::cout << "\nAfter resumption:" << std::endl;
    std::cout << "  Buffer address: " << static_cast<void*>(large_buffer) << std::endl;
    std::cout << "  Vector data address: " << large_vector.data() << std::endl;
    std::cout << "  String data address: " << static_cast<void*>(const_cast<char*>(large_string.data())) << std::endl;
    
    // Check if addresses changed (indicating stack relocation)
    bool buffer_relocated = (static_cast<void*>(large_buffer) != buffer_addr);
    bool vector_relocated = (large_vector.data() != vector_addr);
    bool string_relocated = (static_cast<void*>(const_cast<char*>(large_string.data())) != string_addr);
    
    if (buffer_relocated || vector_relocated || string_relocated) {
        std::cout << "\n🔄 STACK RELOCATION DETECTED!" << std::endl;
        if (buffer_relocated) {
            std::cout << "  ⚠️  Buffer relocated: " << buffer_addr << " -> " << static_cast<void*>(large_buffer) << std::endl;
        }
        if (vector_relocated) {
            std::cout << "  ⚠️  Vector data relocated: " << vector_addr << " -> " << large_vector.data() << std::endl;
        }
        if (string_relocated) {
            std::cout << "  ⚠️  String data relocated: " << string_addr << " -> " << static_cast<void*>(const_cast<char*>(large_string.data())) << std::endl;
        }
        
        std::cout << "\n⚠️  WARNING: Old references and pointers may be INVALID!" << std::endl;
        std::cout << "  - buffer_ref and buffer_ptr may point to old memory" << std::endl;
        std::cout << "  - vector_ref and vector_ptr may point to old memory" << std::endl;
        
        // We cannot safely test the old references/pointers as they might cause undefined behavior
        std::cout << "\n✅ Data values are preserved (moved to new locations):" << std::endl;
    } else {
        std::cout << "\n📍 No stack relocation detected (implementation-specific)" << std::endl;
        std::cout << "  This doesn't mean relocation can't happen - it's implementation dependent" << std::endl;
        std::cout << "  The caveat still applies: references/pointers may become invalid" << std::endl;
        
        std::cout << "\n✅ References and pointers still work (no relocation occurred):" << std::endl;
        std::cout << "  Buffer[100] via old reference: " << static_cast<int>(buffer_ref) << std::endl;
        std::cout << "  Vector[100] via old reference: " << vector_ref << std::endl;
        std::cout << "  Buffer[200] via old pointer: " << static_cast<int>(*buffer_ptr) << std::endl;
        std::cout << "  Vector[200] via old pointer: " << *vector_ptr << std::endl;
    }
    
    // Verify data integrity regardless of relocation
    std::cout << "  Buffer[100] direct access: " << static_cast<int>(large_buffer[100]) << std::endl;
    std::cout << "  Vector[100] direct access: " << large_vector[100] << std::endl;
    std::cout << "  Buffer[200] direct access: " << static_cast<int>(large_buffer[200]) << std::endl;
    std::cout << "  Vector[200] direct access: " << large_vector[200] << std::endl;
    std::cout << "  String length: " << large_string.length() << std::endl;
    
    // Verify buffer pattern is preserved
    bool pattern_correct = true;
    for (int i = 0; i < 8192; i++) {
        if (large_buffer[i] != static_cast<char>(i % 256)) {
            pattern_correct = false;
            break;
        }
    }
    
    std::cout << "  Buffer pattern integrity: " << (pattern_correct ? "✅ PRESERVED" : "❌ CORRUPTED") << std::endl;
    std::cout << "  Vector data integrity: " << (large_vector[500] == 42 ? "✅ PRESERVED" : "❌ CORRUPTED") << std::endl;
    std::cout << "  String data integrity: " << (large_string[500] == 'X' ? "✅ PRESERVED" : "❌ CORRUPTED") << std::endl;
    
    // Safe approach: re-establish references after suspension
    char& safe_buffer_ref = large_buffer[100];
    int& safe_vector_ref = large_vector[100];
    char* safe_buffer_ptr = &large_buffer[200];
    int* safe_vector_ptr = &large_vector[200];
    
    std::cout << "\n✅ Safe references/pointers established after suspension:" << std::endl;
    std::cout << "  Buffer[100] via new reference: " << static_cast<int>(safe_buffer_ref) << std::endl;
    std::cout << "  Vector[100] via new reference: " << safe_vector_ref << std::endl;
    std::cout << "  Buffer[200] via new pointer: " << static_cast<int>(*safe_buffer_ptr) << std::endl;
    std::cout << "  Vector[200] via new pointer: " << *safe_vector_ptr << std::endl;
    
    std::cout << "\n🎯 KEY TAKEAWAY:" << std::endl;
    std::cout << "  - Variable VALUES are always preserved across suspension" << std::endl;
    std::cout << "  - Variable ADDRESSES may change (stack relocation)" << std::endl;
    std::cout << "  - References/pointers from before suspension may become invalid" << std::endl;
    std::cout << "  - Always re-establish references/pointers after suspension for safety" << std::endl;
    
    co_return;
}

co_t test_multiple_suspensions_with_references() {
    std::cout << "\n=== Multiple Suspensions with References Test ===" << std::endl;
    
    int value1 = 100;
    int value2 = 200;
    std::string text = "Original";
    
    // Track addresses through multiple suspensions
    void* addr1_initial = &value1;
    void* addr2_initial = &value2;
    void* text_addr_initial = const_cast<char*>(text.data());
    
    std::cout << "Initial addresses:" << std::endl;
    std::cout << "  value1: " << addr1_initial << std::endl;
    std::cout << "  value2: " << addr2_initial << std::endl;
    std::cout << "  text data: " << text_addr_initial << std::endl;
    
    // First suspension
    co_await std::suspend_always{};
    
    void* addr1_after_1st = &value1;
    void* addr2_after_1st = &value2;
    void* text_addr_after_1st = const_cast<char*>(text.data());
    
    std::cout << "\nAfter 1st suspension:" << std::endl;
    std::cout << "  value1: " << addr1_after_1st << " (moved: " << (addr1_after_1st != addr1_initial ? "YES" : "NO") << ")" << std::endl;
    std::cout << "  value2: " << addr2_after_1st << " (moved: " << (addr2_after_1st != addr2_initial ? "YES" : "NO") << ")" << std::endl;
    std::cout << "  text data: " << text_addr_after_1st << " (moved: " << (text_addr_after_1st != text_addr_initial ? "YES" : "NO") << ")" << std::endl;
    std::cout << "  Values: " << value1 << ", " << value2 << ", '" << text << "'" << std::endl;
    
    // Modify values
    value1 = 150;
    value2 = 250;
    text = "Modified";
    
    // Second suspension
    co_await std::suspend_always{};
    
    void* addr1_after_2nd = &value1;
    void* addr2_after_2nd = &value2;
    void* text_addr_after_2nd = const_cast<char*>(text.data());
    
    std::cout << "\nAfter 2nd suspension:" << std::endl;
    std::cout << "  value1: " << addr1_after_2nd << " (moved from 1st: " << (addr1_after_2nd != addr1_after_1st ? "YES" : "NO") << ")" << std::endl;
    std::cout << "  value2: " << addr2_after_2nd << " (moved from 1st: " << (addr2_after_2nd != addr2_after_1st ? "YES" : "NO") << ")" << std::endl;
    std::cout << "  text data: " << text_addr_after_2nd << " (moved from 1st: " << (text_addr_after_2nd != text_addr_after_1st ? "YES" : "NO") << ")" << std::endl;
    std::cout << "  Values: " << value1 << ", " << value2 << ", '" << text << "'" << std::endl;
    
    std::cout << "\n✅ Conclusion: Values preserved, addresses may change between suspensions" << std::endl;
    
    co_return;
}

int main() {
    std::cout << "C++20 Coroutine Stack Relocation Demonstration" << std::endl;
    std::cout << "===============================================" << std::endl;
    std::cout << "This test attempts to demonstrate the potential for stack relocation" << std::endl;
    std::cout << "and the resulting invalidation of references and pointers." << std::endl;
    std::cout << std::endl;
    
    // Test 1: Large data stack relocation
    auto test1 = test_stack_relocation_with_large_data();
    test1.resume();
    
    // Test 2: Multiple suspensions
    auto test2 = test_multiple_suspensions_with_references();
    test2.resume(); // First suspension
    test2.resume(); // Second suspension
    
    std::cout << "\n===============================================" << std::endl;
    std::cout << "Stack relocation demonstration completed!" << std::endl;
    std::cout << "Note: Stack relocation is implementation-dependent." << std::endl;
    std::cout << "The caveat applies regardless of whether relocation occurs in this run." << std::endl;
    
    return 0;
}
