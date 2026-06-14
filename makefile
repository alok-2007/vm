CC = gcc
CFLAGS  = -Wall -Wextra -std=c11 -g

SRC = $(wildcard src/*.c)
TESTS = $(wildcard tests/*.c)
OBJ = $(SRC:.c=.o)

TARGET = vm

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(OBJ) -o $(TARGET) 

%.o : %.c
	$(CC) $(CFLAGS) -c $< -o $@

test: $(SRC) $(TESTS)
	$(CC) $(SRC) $(TESTS) -o test
	./test
clean:
	rm -f $(OBJ) $(TARGET)
.PHONY: all clean 