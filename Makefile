CC = gcc
CFLAGS = -Wall -Wextra -g
TARGET = cpu_sim
SRCS = main.c cpu.c parser.c hashmap.c segment.c
OBJS = $(SRCS:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)

re: clean all

.PHONY: all clean re
