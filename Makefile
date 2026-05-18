CC     = gcc
CFLAGS = -O2 -Wall -Iinclude -Isrc
LDFLAGS = -lm
SRCS   = $(wildcard src/*.c src/*/*.c src/*/*/*.c)
OBJDIR = build
BINARY = $(OBJDIR)/alceffect
OBJS   = $(patsubst %.c,$(OBJDIR)/%.o,$(SRCS))

all: $(BINARY)

$(BINARY): $(OBJS)
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -o $@ $(OBJS) $(LDFLAGS)

$(OBJDIR)/%.o: %.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -rf $(OBJDIR) *.o alceffect
