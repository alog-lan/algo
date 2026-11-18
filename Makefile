CC = gcc
CFLAGS = -Wall -Wextra -O2 -Isrc
SRC = src/token.c src/lexer.c src/ast.c src/parser.c src/codegen.c src/main.c
TARGET = bin/algoc

all: $(TARGET)

$(TARGET): $(SRC)
	mkdir -p bin
	$(CC) $(CFLAGS) $(SRC) -o $(TARGET)

clean:
	rm -rf bin build *.o *.s dastur
