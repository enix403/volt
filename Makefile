CC := /usr/bin/clang
CFLAGS := -std=c11 -I src

SRC_DIR := src/volt src/volt/code src/volt/debugging src/volt/scanning src/volt/compiling
BUILD_DIR := build/bin
OBJ_DIR := build/obj
TARGET := $(BUILD_DIR)/volt

SRCS := $(wildcard $(addsuffix /*.c, $(SRC_DIR)))
OBJS := $(patsubst src/%, $(OBJ_DIR)/%, $(SRCS:.c=.o))

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJS)
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) $^ -o $@

$(OBJ_DIR)/%.o: src/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(OBJ_DIR) $(BUILD_DIR)




# CC := /usr/bin/clang
# CFLAGS := -std=c11 -I src
# # CFLAGS := -std=c11 -I src -Wall -Wextra -Wpedantic

# SRC_DIR := src/volt src/volt/code src/volt/debugging src/volt/scanning src/volt/compiling
# BUILD_DIR := build/bin
# TARGET := $(BUILD_DIR)/volt

# SRCS := $(wildcard $(addsuffix /*.c, $(SRC_DIR)))
# OBJS := $(SRCS:.c=.o)

# .PHONY: all clean

# all: $(TARGET)

# $(TARGET): $(OBJS)
# 	@mkdir -p $(BUILD_DIR)
# 	$(CC) $(CFLAGS) $^ -o $@

# %.o: %.c
# 	$(CC) $(CFLAGS) -c $< -o $@

# clean:
# 	rm -rf $(OBJS) $(TARGET)


# # -Wnewline-eof
# # -Wstrict-prototypes
# # -Wunused-parameter
# # -Wdeprecated-non-prototype