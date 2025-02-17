
CC := gcc
CFLAGS := -Wall -Wextra -O2 -g

SRCS := src/main.c src/args.c src/util.c
OBJS := $(SRCS:.c=.o)

all: $(OBJS)
	$(CC) $(OBJS) -o parallel_prefix_sum

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) parallel_prefix_sum

