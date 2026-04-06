# Makefile for DE-10 Roulette Game
# Author: Fernando Godinez
# Course: SWE-450 Embedded Systems

# Compiler and flags
CC = gcc
CFLAGS = -Iincludes -Ilib -Isrc -Wall -Wextra -g -std=c99
LDFLAGS = 

# Source files
SRC = src/main.c \
      src/peripherals/led.c \
      src/peripherals/switch.c \
      src/peripherals/display.c \
      src/peripherals/hex.c \
      src/game-logic/game-logic.c

# Object files
OBJ = $(SRC:.c=.o)

# Target executable
TARGET = roulette_game

# Default target
all: $(TARGET)

# Link object files
$(TARGET): $(OBJ)
	$(CC) $(OBJ) -o $(TARGET) $(LDFLAGS)
	@echo "Build complete: $(TARGET)"

# Compile source files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Clean build artifacts
clean:
	rm -f src/*.o src/peripherals/*.o src/game-logic/*.o $(TARGET)
	@echo "Clean complete"

# Rebuild
rebuild: clean all

# Help
help:
	@echo "Available targets:"
	@echo "  all      - Build the project (default)"
	@echo "  clean    - Remove build artifacts"
	@echo "  rebuild  - Clean and build"
	@echo "  help     - Show this help message"

.PHONY: all clean rebuild help
