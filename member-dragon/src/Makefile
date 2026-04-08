CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -g
TARGET = sql_processor
SRCS = main.c schema.c parser.c file_storage.c executor.c
OBJS = $(SRCS:.c=.o)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c $<

clean:
	rm -f *.o $(TARGET)

.PHONY: clean
