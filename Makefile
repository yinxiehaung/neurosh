
CC      = gcc
CFLAGS  = -Wall -Wextra -O3 -Iinclude
LDFLAGS = -lm

# 目錄設定
SRC_DIR = src
OBJ_DIR = obj
INC_DIR = include
BIN_DIR = .

SRCS    = $(wildcard $(SRC_DIR)/*.c)
OBJS    = $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(SRCS))

TARGET  = $(BIN_DIR)/yxsh

all: $(OBJ_DIR) $(TARGET)
$(TARGET): $(OBJS)
	@echo "Linking $@..."
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)
	@echo "Build successful! Run with ./yxsh"

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@echo "Compiling $<..."
	$(CC) $(CFLAGS) -c $< -o $@
$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)
clean:
	@echo "Cleaning up..."
	rm -rf $(OBJ_DIR)/*.o $(TARGET)

rebuild: clean all
.PHONY: all clean rebuild
