TARGET = bin/server
SRC = $(wildcard *.c)
OBJ = $(patsubst %.c, %.o, $(SRC))

run: clean default
	./$(TARGET)

default: $(TARGET)

clean:
	mkdir -p bin
	rm -f *.o
	rm -f bin/*

$(TARGET): $(OBJ)
	gcc -o $@ $?

obj/%.o : src/%.c
	gcc -c $< -o $@ -Iinclude


