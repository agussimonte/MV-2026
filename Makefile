CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -pedantic -g
SRC = src/main.c src/loader.c src/mmu.c src/decoder.c src/cpu.c src/disasm.c
OBJ = $(SRC:.c=.o)
TARGET = vmx

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f src/*.o $(TARGET)