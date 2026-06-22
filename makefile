CC = gcc

CFLAGS = -Wall -Wextra -std=c11 -g
LDFLAGS = -lm -ldl

SRC = $(wildcard src/*.c)
TESTS = $(wildcard tests/*.c)
OBJ = $(SRC:.c=.o)

TARGET = vm

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(OBJ) -o $(TARGET) $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

test:
	$(CC) $(CFLAGS) $(SRC) $(TESTS) -o test $(LDFLAGS)
	./test ./tests/tests_vasm ./tests/expected.txt

clean:
	rm -f $(OBJ) $(TARGET) test

.PHONY: all clean test