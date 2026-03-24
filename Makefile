
CC      = gcc
CFLAGS  = -Wall -Wextra -O3 -Iinclude
LDFLAGS = -lm

# 目錄設定
SRC_DIR = src
OBJ_DIR = obj
INC_DIR = include
BIN_DIR = .

SRCS    = $(wildcard $(SRC_DIR)/yxsh_*.c $(SRC_DIR)/my_*.c)
OBJS    = $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(SRCS))

TARGET  = $(BIN_DIR)/yxsh 
BENCH = $(BIN_DIR)/bench

all: $(OBJ_DIR) $(TARGET) $(BENCH)

$(TARGET): $(OBJS)
	@echo "Linking $@..."
	$(CC) $(CFLAGS) $^ $(SRC_DIR)/main.c -o $@ $(LDFLAGS)
	@echo "Build successful! Run with ./yxsh"

$(BENCH): $(OBJS)
	$(CC) $(CFLAGS) $^ $(SRC_DIR)/pure_bench.c -o $@ $(LDFLAGS)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@echo "Compiling $<..."
	$(CC) $(CFLAGS) -c $< -o $@
$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)
clean:
	@echo "Cleaning up..."
	rm -rf $(OBJ_DIR)/*.o $(TARGET) $(BENCH)

rebuild: clean all
.PHONY: all clean rebuild
