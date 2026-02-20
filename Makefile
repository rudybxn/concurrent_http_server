CC     = gcc
CFLAGS = -Wall -Wextra -Iinclude -lpthread

TARGET      = server
MOCK_CLIENT = tests/mock_client

# NOTE: This can be overridden by passing arguments to make. 
PORT     = 8080
THREADS  = 4
SCHEDULE = FCFS

SRCS = src/main.c src/http.c src/thread_pool.c
OBJS = $(SRCS:.c=.o)

# Build the server only
all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Build and run the mock client against the server
$(MOCK_CLIENT): tests/mock_client.c
	$(CC) $(CFLAGS) -o $(MOCK_CLIENT) tests/mock_client.c

# Capture the server's PID to ensure server is killed after the test
test: $(TARGET) $(MOCK_CLIENT)
	./$(TARGET) $(PORT) $(THREADS) $(SCHEDULE) & \
	SERVER_PID=$$! && \
	sleep 1 && \
	echo "Server PID: $$SERVER_PID\n" && \
	./$(MOCK_CLIENT); \
	kill $$SERVER_PID && \
	echo "\n" && \
	echo "Server (PID $$SERVER_PID) killed." && \
	echo "Mock client finished.\n"

# Remove all build artifacts
clean:
	rm -f $(OBJS) $(TARGET) $(MOCK_CLIENT)

.PHONY: all test clean