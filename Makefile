SHELL := /bin/bash

CC = gcc
CFLAGS = -Wall -Werror -g -I./include
SRC_DIR = src
BUILD_DIR = build
DEBUG_DIR = debug
TEST_DIR = tests
SRCS = $(wildcard $(SRC_DIR)/*.c)
OBJS = $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/obj/%.o,$(SRCS))
DEBUG_OBJS = $(patsubst $(SRC_DIR)/%.c,$(DEBUG_DIR)/obj/%.o,$(SRCS))
EXEC = ./mini_fs
DEBUG_EXEC = $(DEBUG_DIR)/bin/program

.PHONY: release debug all run check clean

release: directories_build $(EXEC)
	@echo "Release build completed."

debug: CFLAGS += -DDEBUG -O0
debug: directories_debug $(DEBUG_EXEC)
	@echo "Debug build completed."

all: release debug

$(EXEC): $(OBJS)
	$(CC) $^ -o $@

$(DEBUG_EXEC): $(DEBUG_OBJS)
	$(CC) $^ -o $@

$(BUILD_DIR)/obj/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

$(DEBUG_DIR)/obj/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

run:
	make release
	$(EXEC)

check: release
	@rm -f $(TEST_DIR)/output.txt
	@touch $(TEST_DIR)/output.txt
	@sed 's/\r$$//' $(TEST_DIR)/commands.txt | while IFS= read -r line; do \
		eval "set -- $$line"; \
		./$(EXEC) "$$@" >> $(TEST_DIR)/output.txt 2>&1; \
	done
	@diff -u <(sed 's/\r$$//' $(TEST_DIR)/expected_output.txt) <(sed 's/\r$$//' $(TEST_DIR)/output.txt) || (echo "Output mismatch"; exit 1)

directories_build:
	mkdir -p $(BUILD_DIR)/obj

directories_debug:
	mkdir -p $(DEBUG_DIR)/obj $(DEBUG_DIR)/bin

clean:
	rm -rf $(BUILD_DIR) $(DEBUG_DIR) $(EXEC)
	@echo "Cleaned up build and debug directories."