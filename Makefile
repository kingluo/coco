# Top-level Makefile for coco library
# Delegates to subdirectory Makefiles

EXAMPLES_DIR = examples
TESTS_DIR = tests

.PHONY: all examples test clean help

# Default target
all: examples test

# Delegate to examples/Makefile
examples:
	@$(MAKE) -C $(EXAMPLES_DIR)

# Delegate to tests/Makefile
test:
	@$(MAKE) -C $(TESTS_DIR) run-all-tests

# Delegate clean to subdirectories
clean:
	@$(MAKE) -C $(EXAMPLES_DIR) clean
	@$(MAKE) -C $(TESTS_DIR) clean
	@rm -rf build

# Help
help:
	@echo "Top-level targets:"
	@echo "  all       - Build examples and run all tests"
	@echo "  examples  - Build all examples (delegates to examples/Makefile)"
	@echo "  test      - Run all tests (delegates to tests/Makefile)"
	@echo "  clean     - Clean all build artifacts"
	@echo "  help      - Show this help"
	@echo ""
	@echo "For more options:"
	@echo "  cd examples && make help"
	@echo "  cd tests && make help"
