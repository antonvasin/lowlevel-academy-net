SERVER = bin/server
CLIENT = bin/client
# SRC = $(wildcard src/*.c)
# OBJ = $(patsubst src/%.c, obj/%.o, $(SRC))

run: clean default
	./$(SERVER)

default: $(CLIENT)

clean:
	mkdir -p bin
	# mkdir -p obj
	# rm -f obj/*.o
	rm -f bin/*
	rm -f *.db

$(SERVER): server.c
	gcc -o $(SERVER) server.c

$(CLIENT): $(SERVER) client.c
	gcc -o $(CLIENT) client.c

# obj/%.o : src/%.c
# 	gcc -c $< -o $@ -Iinclude
