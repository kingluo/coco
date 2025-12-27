CXX = g++
CXXFLAGS = -std=c++20 -fcoroutines -I.. -Wall -Wextra -Werror -O2 -g
WEBSERVER_LDFLAGS = -luring

# Build directory
BUILD_DIR = ../build

# Example executables
EXAMPLES = channel_and_waitgroup webserver join_example
BUILD_EXAMPLES = $(addprefix $(BUILD_DIR)/, $(EXAMPLES))

all: $(BUILD_EXAMPLES)

# Ensure build directory exists
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/channel_and_waitgroup: channel_and_waitgroup.cpp ../coco.h | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -o $@ $< $(LDFLAGS)

$(BUILD_DIR)/webserver: webserver.cpp ../coco.h | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -o $@ $< $(WEBSERVER_LDFLAGS)

$(BUILD_DIR)/join_example: join_example.cpp ../coco.h | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -o $@ $< $(LDFLAGS)

run-webserver: $(BUILD_DIR)/webserver
	@$(BUILD_DIR)/webserver

clean:
	rm -f $(BUILD_EXAMPLES)

help:
	@echo "Available targets:"
	@echo "  all           - Build all examples"
	@echo "  clean         - Remove built examples"
	@echo "  run-webserver - Build and run webserver example"
	@echo ""
	@echo "Individual examples:"
	@echo "  channel_and_waitgroup - Build channel and waitgroup example"
	@echo "  webserver             - Build webserver example (requires liburing)"
	@echo "  join_example          - Build join example"

.PHONY: all run-webserver clean help
