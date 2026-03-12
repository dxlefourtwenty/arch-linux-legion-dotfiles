# Makefile for Hellpaper

CC = gcc
SRC = main.c state.c shaders.c utils.c effects.c wallpapers.c config.c scene.c

TARGET = hellpaper
CFLAGS = -Wall -O2
LIBS = -lraylib -lm -pthread #-lGL -ldl -lrt

PREFIX ?= /usr/local
BINDIR = $(PREFIX)/bin

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) $(SRC) -o $(TARGET) $(LIBS)

clean:
	@echo "Cleaning up..."
	rm -f $(TARGET)

install: $(TARGET)
	mkdir -p $(BINDIR)
	install -m 755 $(TARGET) $(BINDIR)

uninstall:
	rm -f $(BINDIR)/$(TARGET)

.PHONY: all clean install uninstall
