CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -Iinclude
TARGET = shell.out
SRCDIR = src
INCDIR = include
OBJDIR = obj

SOURCES = $(SRCDIR)/main.c $(SRCDIR)/prompt.c $(SRCDIR)/input.c $(SRCDIR)/parser.c $(SRCDIR)/commands.c $(SRCDIR)/executor.c $(SRCDIR)/redirection.c $(SRCDIR)/pipes.c $(SRCDIR)/background.c $(SRCDIR)/utils.c $(SRCDIR)/signals.c $(SRCDIR)/log.c
OBJECTS = $(SOURCES:$(SRCDIR)/%.c=$(OBJDIR)/%.o)

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) -o $@

$(OBJDIR)/%.o: $(SRCDIR)/%.c | $(OBJDIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIR):
	mkdir -p $(OBJDIR)

clean:
	rm -rf $(OBJDIR) $(TARGET)

install: $(TARGET)
	cp $(TARGET) /usr/local/bin/

.PHONY: install
