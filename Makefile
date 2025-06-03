CC := gcc
CFLAGS := -std=c17 -s -Wall -Wextra -Iinclude/ -Iinclude/engine/*.h -Iinclude/elements/*.h -Iinclude/utils/*.h
RELEASE_CFLAGS := -Werror -O3 -ffast-math
TEST_CFLAGS := -g -DDEBUG -fsanitize=address
LDFLAGS := -lSDL2 -lSDL2_ttf -lSDL2_image -lyaml

SRC_DIR := src
BUILD_DIR := build

SRCS := $(wildcard $(SRC_DIR)/*.c)
CORE_SRCS := $(wildcard $(SRC_DIR)/engine/*.c)
ELEMENTS_SRCS := $(wildcard $(SRC_DIR)/elements/*.c)
UTILS_SRCS := $(wildcard $(SRC_DIR)/utils/*.c)

ALL_SRCS = $(SRCS) $(CORE_SRCS) $(ELEMENTS_SRCS) $(UTILS_SRCS)

RELEASE_OUTPUT := $(BUILD_DIR)/Game
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
