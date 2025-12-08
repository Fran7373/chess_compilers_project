CC = gcc
CFLAGS = -Wall -Wextra -g -Iinclude

SRC = ./lexer.c ./parser.c ./semant.c ./test.c
OBJ = $(SRC:.c=.o)

TARGET = build/chess_compiler

all: $(TARGET)

$(TARGET): $(OBJ)
	@mkdir -p build
	$(CC) $(CFLAGS) $(OBJ) -o $(TARGET)
	@echo "✔ Compilación exitosa. Ejecuta: ./$(TARGET)"

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f src/*.o
	rm -f $(TARGET)
	@echo "✔ Limpieza completa."

run: all
	./$(TARGET)
