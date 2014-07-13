PROG := g1g2g3phone
SRCS := phone_udp.c fft.c
OBJS := $(SRCS:%.c=%.o)
DEPS := $(SRCS:%.c=%.d)
LIBS := -lm

CC := gcc

all: $(PROG)

-include $(DEPS)

$(PROG): $(OBJS)
	$(CC) -o $@ $^ $(LIBS)

%.o: %.c
	$(CC) -c -Wall -g -MMD -MP $<

clean:
	rm -f $(PROG) $(OBJS) $(DEPS)