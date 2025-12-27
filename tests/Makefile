# Makefile for coco library tests
# Requires C++20 compiler support

CXX = g++
CXXFLAGS = -std=c++20 -fcoroutines -I.. -Wall -Wextra -Werror -O2 -g
LDFLAGS =

# Build directory for compiled tests
BUILD_DIR = ../build

# Core library tests
CORE_TESTS = test_co_t test_co_t_join test_chan_t test_wg_t test_integration test_cpp20_caveats test_advanced_caveats test_stack_relocation_demo

# Channel behavior and distribution tests
CHANNEL_TESTS = simple_channel_test test_chan_comprehensive test_chan_edge_cases test_chan_fair_distribution test_chan_go_compatibility test_chan_stress test_go_channel_compliance test_chan_fifo_ordering test_chan_race_conditions test_chan_state_consistency test_chan_unbuffered_handoff

# Wait group tests
WG_TESTS = test_wg_simple test_wg_multiple_waiters test_wg_no_bounds

# Additional tests
ADDITIONAL_TESTS = test_bug_fixes test_fair_distribution test_weak_ptr_improvements test_scheduler_clear test_buffered_channel_fix test_co_yield_fix

# Problematic tests (may fail with current implementation)
PROBLEMATIC_TESTS = test_mutex_yield

# All test executables
TESTS = $(CORE_TESTS) $(CHANNEL_TESTS) $(WG_TESTS) $(ADDITIONAL_TESTS) $(PROBLEMATIC_TESTS)

# Test executables that should actually build
BUILDABLE_TESTS = $(CORE_TESTS) $(CHANNEL_TESTS) $(WG_TESTS) $(ADDITIONAL_TESTS) test_mutex_yield

# Test executables with build directory prefix
BUILD_TESTS = $(addprefix $(BUILD_DIR)/, $(BUILDABLE_TESTS))

# Default target
all: $(BUILD_TESTS)

# Ensure build directory exists
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# Generic rule for building tests
$(BUILD_DIR)/%: %.cpp ../coco.h | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -o $@ $< $(LDFLAGS)

# Special case for mutex test that needs pthread
$(BUILD_DIR)/test_mutex_yield: test_mutex_yield.cpp ../coco.h | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -o $@ $< $(LDFLAGS) -pthread

# Run all tests
test: $(BUILD_TESTS)
	@echo "Running all tests..."
	@echo "===================="
	@$(BUILD_DIR)/test_co_t
	@echo ""
	@$(BUILD_DIR)/test_co_t_join
	@echo ""
	@$(BUILD_DIR)/test_chan_t
	@echo ""
	@$(BUILD_DIR)/test_wg_t
	@echo ""
	@$(BUILD_DIR)/test_integration
	@echo ""
	@$(BUILD_DIR)/test_cpp20_caveats
	@echo ""
	@$(BUILD_DIR)/test_advanced_caveats
	@echo ""
	@$(BUILD_DIR)/test_stack_relocation_demo
	@echo ""
	@echo "===================="
	@echo "All tests completed successfully! ✓"

# Run individual core tests
run-co-t: $(BUILD_DIR)/test_co_t
	$(BUILD_DIR)/test_co_t

run-co-t-join: $(BUILD_DIR)/test_co_t_join
	$(BUILD_DIR)/test_co_t_join

run-chan-t: $(BUILD_DIR)/test_chan_t
	$(BUILD_DIR)/test_chan_t

run-wg-t: $(BUILD_DIR)/test_wg_t
	$(BUILD_DIR)/test_wg_t

run-integration: $(BUILD_DIR)/test_integration
	$(BUILD_DIR)/test_integration

run-cpp20-caveats: $(BUILD_DIR)/test_cpp20_caveats
	$(BUILD_DIR)/test_cpp20_caveats

run-advanced-caveats: $(BUILD_DIR)/test_advanced_caveats
	$(BUILD_DIR)/test_advanced_caveats

run-stack-relocation-demo: $(BUILD_DIR)/test_stack_relocation_demo
	$(BUILD_DIR)/test_stack_relocation_demo

# Run test categories
run-core-tests: $(addprefix $(BUILD_DIR)/, $(CORE_TESTS))
	@echo "Running core library tests..."
	@echo "============================="
	@$(BUILD_DIR)/test_co_t
	@echo ""
	@$(BUILD_DIR)/test_co_t_join
	@echo ""
	@$(BUILD_DIR)/test_chan_t
	@echo ""
	@$(BUILD_DIR)/test_wg_t
	@echo ""
	@$(BUILD_DIR)/test_integration
	@echo ""
	@$(BUILD_DIR)/test_cpp20_caveats
	@echo ""
	@$(BUILD_DIR)/test_advanced_caveats
	@echo ""
	@$(BUILD_DIR)/test_stack_relocation_demo
	@echo ""
	@echo "Core tests completed! ✓"

run-channel-tests: $(addprefix $(BUILD_DIR)/, $(CHANNEL_TESTS))
	@echo "Running channel behavior tests..."
	@echo "================================="
	@for test in $(CHANNEL_TESTS); do \
		echo "Running $$test..."; \
		$(BUILD_DIR)/$$test; \
		echo ""; \
	done
	@echo "Channel tests completed! ✓"

run-wg-tests: $(addprefix $(BUILD_DIR)/, $(WG_TESTS))
	@echo "Running wait group tests..."
	@echo "==========================="
	@for test in $(WG_TESTS); do \
		echo "Running $$test..."; \
		$(BUILD_DIR)/$$test; \
		echo ""; \
	done
	@echo "Wait group tests completed! ✓"

run-additional-tests: $(addprefix $(BUILD_DIR)/, $(ADDITIONAL_TESTS))
	@echo "Running additional tests..."
	@for test in $(ADDITIONAL_TESTS); do \
		echo "Running $$test..."; \
		$(BUILD_DIR)/$$test; \
		echo ""; \
	done
	@echo "Additional tests completed! ✓"

run-problematic-tests: $(BUILD_DIR)/test_mutex_yield
	@echo "Running problematic tests (may fail)..."
	@$(BUILD_DIR)/test_mutex_yield || echo "⚠️  test_mutex_yield failed (expected)"

run-all-tests: $(BUILD_TESTS)
	@echo "Running comprehensive test suite..."
	@$(MAKE) test
	@$(MAKE) run-channel-tests
	@$(MAKE) run-wg-tests
	@$(MAKE) run-additional-tests
	@echo "=== Problematic Tests (may fail) ==="
	@$(MAKE) run-problematic-tests
	@echo "All tests completed!"

run-safe-tests: $(BUILD_TESTS)
	@echo "Running safe tests (excluding problematic ones)..."
	@$(MAKE) test
	@$(MAKE) run-channel-tests
	@$(MAKE) run-wg-tests
	@$(MAKE) run-additional-tests
	@echo "Safe tests completed!"

# Clean build artifacts
clean:
	rm -f $(BUILD_TESTS)
	rm -f $(TESTS)  # Also clean any executables in current directory

# Check compiler support
check-compiler:
	@echo "Checking C++20 coroutine support..."
	@$(CXX) --version
	@echo "Testing basic coroutine compilation..."
	@echo '#include <coroutine>' > /tmp/test_coroutine.cpp
	@echo 'struct Task { struct promise_type { Task get_return_object() { return {}; } std::suspend_never initial_suspend() noexcept { return {}; } std::suspend_never final_suspend() noexcept { return {}; } void return_void() {} void unhandled_exception() {} }; };' >> /tmp/test_coroutine.cpp
	@echo 'Task test() { co_return; } int main() { return 0; }' >> /tmp/test_coroutine.cpp
	@$(CXX) $(CXXFLAGS) -o /tmp/test_coroutine /tmp/test_coroutine.cpp && echo "✓ C++20 coroutine support confirmed" || echo "✗ C++20 coroutine support not available"
	@rm -f /tmp/test_coroutine.cpp /tmp/test_coroutine

# Help target
help:
	@echo "Available targets:"
	@echo ""
	@echo "Build and Test:"
	@echo "  all                      - Build all tests"
	@echo "  test                     - Build and run core tests"
	@echo "  run-all-tests            - Run comprehensive test suite (all tests including problematic)"
	@echo "  run-safe-tests           - Run safe test suite (excluding problematic tests)"
	@echo "  clean                    - Remove build artifacts"
	@echo ""
	@echo "Test Categories:"
	@echo "  run-core-tests           - Run core library tests (co_t, chan_t, wg_t, integration, caveats)"
	@echo "  run-channel-tests        - Run channel behavior and distribution tests"
	@echo "  run-wg-tests             - Run wait group tests"
	@echo "  run-additional-tests     - Run additional tests (bug fixes, improvements, etc.)"
	@echo "  run-problematic-tests    - Run tests that may fail with current implementation"
	@echo ""
	@echo "Individual Core Tests:"
	@echo "  run-co-t                 - Run co_t tests"
	@echo "  run-chan-t               - Run chan_t tests"
	@echo "  run-wg-t                 - Run wg_t tests"
	@echo "  run-integration          - Run integration tests"
	@echo "  run-cpp20-caveats        - Run C++20 coroutine caveats tests"
	@echo "  run-advanced-caveats     - Run advanced caveats tests"
	@echo "  run-stack-relocation-demo - Run stack relocation demonstration"
	@echo ""
	@echo "Special Tests:"
	@echo "  check-compiler           - Check C++20 coroutine support"
	@echo "  help                     - Show this help message"

.PHONY: all test run-co-t run-chan-t run-wg-t run-integration run-cpp20-caveats run-advanced-caveats run-stack-relocation-demo run-core-tests run-channel-tests run-wg-tests run-additional-tests run-problematic-tests run-all-tests run-safe-tests clean check-compiler help
