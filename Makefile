CC      = gcc
CFLAGS  = -Wall -Wextra -Iinclude
LDFLAGS = -lpthread

TARGET      = server
MOCK_CLIENT = tests/mock_client

# NOTE: These can be overridden by passing arguments to make.
# e.g., make test PORT=9000 THREADS=8 BUFSIZE=32 SCHEDULE=SFF
PORT     = 8080
THREADS  = 4
BUFSIZE  = 16
SCHEDULE = FCFS

# ---- Source files ------------------------------------------------
SRCS = src/main.c src/http.c src/thread_pool.c src/bounded_buffer.c
OBJS = $(SRCS:.c=.o)

# ---- Test source files -------------------------------------------
TEST_BUF_SRCS  = tests/test_buffer.c src/bounded_buffer.c
TEST_POOL_SRCS = tests/test_thread_pool.c src/bounded_buffer.c src/thread_pool.c

# ---- Build the server --------------------------------------------
all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS) $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# ---- Build the mock client ---------------------------------------
$(MOCK_CLIENT): tests/mock_client.c
	$(CC) $(CFLAGS) -o $(MOCK_CLIENT) tests/mock_client.c

# ---- Integration test: start server, run mock client, shut down --
#  1. Ensure www/ exists with a test file
#  2. Start the server in background with our flags
#  3. Sleep to let it initialize the thread pool
#  4. Run mock client — should get a 200 response
#  5. Kill the server and report result
test: $(TARGET) $(MOCK_CLIENT)
	@mkdir -p www
	@if [ ! -f www/index.html ]; then \
		echo '<html><body><h1>It works!</h1></body></html>' > www/index.html; \
	fi
	@echo "--- Starting server on port $(PORT) with $(THREADS) threads ---"
	@./$(TARGET) -p $(PORT) -t $(THREADS) -b $(BUFSIZE) -s $(SCHEDULE) & \
	SERVER_PID=$$!; \
	sleep 1; \
	echo "Server PID: $$SERVER_PID"; \
	echo "--- Running mock client ---"; \
	./$(MOCK_CLIENT); \
	CLIENT_EXIT=$$?; \
	kill $$SERVER_PID 2>/dev/null; \
	wait $$SERVER_PID 2>/dev/null; \
	echo "--- Server (PID $$SERVER_PID) stopped ---"; \
	if [ $$CLIENT_EXIT -eq 0 ]; then \
		echo "TEST PASSED"; \
	else \
		echo "TEST FAILED"; \
		exit 1; \
	fi

# ---- Unit tests: buffer and thread pool in isolation -------------
test-buffer: $(TEST_BUF_SRCS)
	$(CC) $(CFLAGS) -o tests/test_buffer $(TEST_BUF_SRCS) $(LDFLAGS)
	./tests/test_buffer

test-pool: $(TEST_POOL_SRCS)
	$(CC) $(CFLAGS) -o tests/test_pool $(TEST_POOL_SRCS) $(LDFLAGS)
	./tests/test_pool

# ---- Run all tests -----------------------------------------------
test-all: test-buffer test-pool test

# ---- Remove all build artifacts ----------------------------------
clean:
	rm -f $(OBJS) $(TARGET) $(MOCK_CLIENT)
	rm -f tests/test_buffer tests/test_pool

.PHONY: all test test-buffer test-pool test-all clean