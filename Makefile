BUILD_DIR := output
TARGET := $(BUILD_DIR)/hello
PROCESSES ?= 4

.PHONY: all configure build run clean

all: build

configure:
	cmake -S . -B $(BUILD_DIR)

build: configure
	cmake --build $(BUILD_DIR)

run: build
	mpirun -np $(PROCESSES) ./$(TARGET)

clean:
	cmake -E rm -rf $(BUILD_DIR)
