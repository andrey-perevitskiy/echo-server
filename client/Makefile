CC = gcc
CFLAGS = -g -Wall -I../common/include
LDFLAGS = -g
SRCDIR = src
OBJDIR = obj
SRCDIR_COMMON = ../common/src
OBJDIR_COMMON = ../common/obj
OBJS_COMMON = $(patsubst %.o,$(OBJDIR_COMMON)/%.o,freectx.o)
BINDIR = ../bin
BIN = client

.PHONY: all
all: $(BIN)

$(BIN): $(OBJDIR)/$(BIN).o $(OBJS_COMMON)
	@mkdir -p $(BINDIR)
	$(CC) -o $(BINDIR)/$@ $^ $(LDFLAGS)

$(OBJDIR)/%.o: $(SRCDIR)/%.c
	@mkdir -p $(OBJDIR)
	$(CC) $(CFLAGS) -c $^ -o $@

$(OBJDIR_COMMON)/%.o: $(SRCDIR_COMMON)/%.c
	@mkdir -p $(OBJDIR_COMMON)
	$(CC) $(CFLAGS) -c $^ -o $@

.PHONY: clean
clean:
	rm -f $(BINDIR)/$(BIN)
	rm -rf obj
