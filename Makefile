# ---- Configuration --------------------------------------------------------
TARGET   := sim
CC       := gcc
CSTD     := c11
CFLAGS   := -std=$(CSTD) -Wall -Wextra -Wpedantic -Iinclude -Isrc -pthread
LDFLAGS  := -pthread

SRC_DIR   := src
BUILD_DIR := build
BIN       := $(BUILD_DIR)/$(TARGET)

# ---- Sources / objects ----------------------------------------------------
SRCS := $(wildcard $(SRC_DIR)/*.c)
OBJS := $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(SRCS))
DEPS := $(OBJS:.o=.d)

# ---- Build type -----------------------------------------------------------
# Usage: make            -> debug build (default)
#        make BUILD=release
BUILD ?= debug
ifeq ($(BUILD),release)
    CFLAGS += -O2 -DNDEBUG
else
    CFLAGS += -O0 -g
endif

# ---- Targets --------------------------------------------------------------
.PHONY: all run clean

all: $(BIN)

$(BIN): $(OBJS)
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -MMD -MP -c $< -o $@

run: $(BIN)
	./$(BIN)

clean:
	$(RM) -r $(BUILD_DIR)

-include $(DEPS)