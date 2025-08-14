BUILD_DIR := build
TARGET := $(BUILD_DIR)/bookslice

# Pass arguments as: make run RUN_ARGS="path/to/book.pdf"
RUN_ARGS ?=

.PHONY: all build clean rebuild run install

all: build

build:
	cmake -B $(BUILD_DIR) -S .
	cmake --build $(BUILD_DIR)

clean:
	@echo "Removing build directory"
	rm -rf $(BUILD_DIR)

rebuild: clean build

run: build
	@echo "Running $(TARGET) $(RUN_ARGS)"
	@$(TARGET) $(RUN_ARGS)

install:
	cmake --install $(BUILD_DIR)
