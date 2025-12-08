# Compilador y flags
CC = gcc
CFLAGS = -Wall -Wextra -std=c11

# Archivos fuente
SRC = lexer.c parser.c board.c test.c

# Objetos generados
OBJ = $(SRC:.c=.o)

# Carpeta donde se guarda el ejecutable
BUILD = build
TARGET = $(BUILD)/chess_compiler

# Regla principal
all: $(BUILD) $(TARGET)

# Crear carpeta build si no existe
$(BUILD):
	mkdir -p $(BUILD)

# Enlazar todos los objetos para crear el ejecutable
$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJ)

# Compilar cada archivo .c a .o
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Limpiar archivos generados
clean:
	rm -f *.o
	rm -rf $(BUILD)

.PHONY: all clean
