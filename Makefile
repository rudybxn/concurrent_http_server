CC = gcc
CFLAGS = -Wall -Wextra -Iinclude -lpthread

TARGET = server

SRCS = src/main.c src/http.c src/thread_pool.c
OBJS = $(SRCS:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: all clean