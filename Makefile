CC = gcc
CFLAGS = -Wall -Wextra -O2 -Isrc
SRC = src/token.c src/lexer.c src/ast.c src/parser.c src/codegen.c src/main.c
TARGET = bin/algo

all: $(TARGET)

$(TARGET): $(SRC)
	mkdir -p bin
	$(CC) $(CFLAGS) $(SRC) -o $(TARGET)
	ln -sf algo bin/algoc

test: $(TARGET)
	./$(TARGET) sinov

sayt: $(TARGET)
	./$(TARGET) yig sayt.algo -o bin/sayt

web: sayt
	./bin/sayt

clean:
	rm -rf bin build *.o *.s dastur test* /tmp/algo_*
