CC := clang
CFLAGS := -std=c17 -s -Wall -Wextra -Iinclude/ -Iinclude/engine/*.h -Iinclude/elements/*.h -Iinclude/utils/*.h
RELEASE_CFLAGS := -Werror -O3 -ffast-math -DNDEBUG
TEST_CFLAGS := -g -O0 -DDEBUG -fsanitize=address
LDFLAGS :=

ECS := $(wildcard ecs/*.c)
ENGINE := $(wildcard engine/*.c)
TYPES := $(wildcard types/*.c)
UTILS_SRCS := $(wildcard utils/*.c)
MAIN := testmain.c

ALL_SRCS = $(ECS) $(ENGINE) $(TYPES) $(UTILS_SRCS) $(MAIN)

BUILD_DIR := build
RELEASE_OUTPUT := $(BUILD_DIR)/Engine
TEST_OUTPUT := $(BUILD_DIR)/test

all: game

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

game: $(BUILD_DIR) $(RELEASE_OUTPUT)

$(RELEASE_OUTPUT): $(ALL_SRCS)
	@echo "Compiling release build..."
	$(CC) $(CFLAGS) $(RELEASE_CFLAGS) $^ $(LDFLAGS) -o $@
	@echo "Build successful: $(RELEASE_OUTPUT)"

.PHONY: test
test: $(BUILD_DIR) $(TEST_OUTPUT)

$(TEST_OUTPUT): $(ALL_SRCS)
	@echo "Compiling test build..."
	$(CC) $(CFLAGS) $(TEST_CFLAGS) $^ $(LDFLAGS) -o $@
	@echo "Build successful: $(TEST_OUTPUT)"

.PHONY: clean
clean:
	rm -rf $(BUILD_DIR)
	@echo "Cleaned build files."
