PROG := g1g2g3phone
SRCS := phone_udp.c fft.c
OBJS := $(SRCS:%.c=%.o)
DEPS := $(SRCS:%.c=%.d)
LIBS := -lm
CVLIB := `pkg-config --libs opencv`
CVINC := `pkg-config --cflags opencv`

CC := gcc

all: $(PROG)

-include $(DEPS)

$(PROG): $(OBJS)
	$(CC) -o $@ $^ $(LIBS) $(CVLIB)

%.o: %.c
	$(CC) -c -Wall -g -MMD -MP $< $(CVINC)

clean:
	rm -f $(PROG) $(OBJS) $(DEPS)