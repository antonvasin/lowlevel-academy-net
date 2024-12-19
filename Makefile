TARGET_SRV = bin/dbserver
TARGET_CLI = bin/dbcli

SRC_SRV = $(wildcard src/srv/*.c)
OBJ_SRV = $(SRC_SRV:src/srv/%.c=obj/srv/%.o)

SRC_CLI = $(wildcard src/cli/*.c)
OBJ_CLI = $(SRC_CLI:src/cli/%.c=obj/cli/%.o)

PORT = 5555

# run: clean default
# 	./$(TARGET_SRV) -f ./mynewdb.db -n -p $(PORT) &
# 	./$(TARGET_CLI) -h 127.0.0.1 -p $(PORT)
# 	kill -9 $$(pidof dbserver)

run: clean default
	@trap 'kill $$(jobs -p) 2>/dev/null' EXIT; \
	{ \
		./$(TARGET_SRV) -f ./mynewdb.db -n -p $(PORT) & \
		server_pid=$$!; \
		sleep 1; \
		{ timeout 2s ./$(TARGET_CLI) -h 127.0.0.1 -p $(PORT) -a "John Doe,123 Parkway Dr.,50" || true; } \
	}
default: $(TARGET_SRV) $(TARGET_CLI)

clean:
	mkdir -p obj/srv
	mkdir -p obj/cli
	rm -f obj/srv/*.o
	rm -f bin/*
	rm -f *.db

$(TARGET_SRV): $(OBJ_SRV)
	gcc -o $@ $?

$(OBJ_SRV): obj/srv/%.o: src/srv/%.c
	gcc -c $< -o $@ -Iinclude

$(TARGET_CLI): $(OBJ_CLI)
	gcc -o $@ $?

$(OBJ_CLI): obj/cli/%.o: src/cli/%.c
	gcc -c $< -o $@ -Iinclude
