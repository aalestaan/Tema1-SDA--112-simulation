CC = gcc
CFLAGS = -Wall -Wextra -g


TARGET = tema1
SRC = tema1.c


build: $(SRC)
	$(CC) $(CFLAGS) $(SRC) -o $(TARGET)


run: build
	./$(TARGET)


valgrind: build
	valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes ./$(TARGET)


clean:
	rm -f $(TARGET)

.PHONY: build run valgrind clean