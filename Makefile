PROG := g1g2g3phone
SRCS := phone_udp.c fft.c
OBJS := $(SRCS:%.c=%.o)
DEPS := $(SRCS:%.c=%.d)

CC := gcc

all: $(PROG)

-include $(DEPS)

$(PROG): $(OBJS)
	$(CC) -o $@ $^

%.o: %.c
	$(CC) -c -Wall -g -lm -MMD -MP $<

clean:
	rm -f $(PROG) $(OBJS) $(DEPS)