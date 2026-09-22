CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -pedantic -g
SRC = src/main.c src/loader.c src/mmu.c src/decoder.c src/cpu.c src/alu.c
OBJ = $(SRC:.c=.o)
TARGET = vmx

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f src/*.o $(TARGET)c/alu.o src/cpu.o tests/test_cpu.c
	$(CC) $(CFLAGS) -o test_runner tests/test_cpu.c src/mmu.o src/alu.o src/cpu.o
	./test_runner
	rm -f test_runner

clean:
	rm -f src/*.o $(TARGET)
	rm -f src/*.o $(TARGET) test_runner(CC) $(CFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f src/*.o $(TARGET)^

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f src/*.o $(TARGET)