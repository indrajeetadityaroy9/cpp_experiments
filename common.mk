# Compiler settings (can be overridden)
CXX ?= g++
CXXSTD ?= c++23
OPTIMIZATION ?= -O3

# Warning flags
WARNINGS = -Wall -Wextra -Werror

# Combined flags
CXXFLAGS_BASE = -std=$(CXXSTD) $(WARNINGS) $(OPTIMIZATION)

# Auto-dependency generation flags
DEPFLAGS = -MMD -MP

# Root directory (relative to including Makefile)
ROOT_DIR ?= ..

# Vendor directory for third-party code
VENDOR_DIR = $(ROOT_DIR)/vendor

# Catch2 support
CATCH2_HPP = $(VENDOR_DIR)/catch_amalgamated.hpp
CATCH2_CPP = $(VENDOR_DIR)/catch_amalgamated.cpp
CATCH2_INC = -I$(VENDOR_DIR)

# Build directory (optional, for projects that want organized output)
BUILD_DIR ?= build

# Common phony targets
.PHONY: all clean test benchmark help

# Include auto-generated dependencies if they exist
-include $(DEPS)

# Help target
help:
	@echo "Common targets:"
	@echo "  all       - Build main target(s)"
	@echo "  test      - Build and run tests"
	@echo "  benchmark - Build and run benchmarks"
	@echo "  clean     - Remove build artifacts"
	@echo ""
	@echo "Variables:"
	@echo "  CXX=$(CXX)"
	@echo "  CXXSTD=$(CXXSTD)"
	@echo "  OPTIMIZATION=$(OPTIMIZATION)"
