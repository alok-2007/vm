CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -g
LDFLAGS = -lm -ldl

# Source definitions
SRC = main.c $(wildcard src/*.c)
OBJ = $(SRC:.c=.o)

# Test definitions (Filters out main.c to prevent duplicate main() functions)
TEST_SRC = $(wildcard src/*.c) $(wildcard tests/*.c)
TEST_OBJ = $(TEST_SRC:.c=.o)

TARGET = vm
TEST_TARGET = test_runner

all: $(TARGET)

# Build the main VM executable
$(TARGET): $(OBJ)
	$(CC) $(OBJ) -o $(TARGET) $(LDFLAGS)

# Rule for building object files in root or src/
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Build and run tests safely
test: $(TEST_OBJ)
	$(CC) $(CFLAGS) $(TEST_OBJ) -o $(TEST_TARGET) $(LDFLAGS)
	./$(TEST_TARGET) ./tests/tests_vasm ./tests/expected.txt

clean:
	rm -f main.o src/*.o tests/*.o
	rm -f $(TARGET) $(TEST_TARGET) test

.PHONY: all clean test