CC = gcc
CFLAGS = -Wall -Wextra -Iinclude -lpthread

TARGET = server
MOCK_CLIENT = tests/mock_client

SRCS = src/main.c src/http.c src/thread_pool.c
OBJS = $(SRCS:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

$(MOCK_CLIENT): tests/mock_client.c
	$(CC) $(CFLAGS) -o $(MOCK_CLIENT) tests/mock_client.c

mock-client: $(TARGET) $(MOCK_CLIENT)
	./$(TARGET) 8080 4 FCFS & sleep 1 && ./$(MOCK_CLIENT)

clean:
	rm -f $(OBJS) $(TARGET) $(MOCK_CLIENT)

.PHONY: all clean mock-client