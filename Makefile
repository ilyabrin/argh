# Makefile for argh.h
# Works with GCC, Clang, MinGW, and Unix make

CC = gcc
CFLAGS = -std=c99 -Wall -Wextra -Wpedantic -Werror -O2 -D_CRT_SECURE_NO_WARNINGS
LDFLAGS =
EXE = .exe

.PHONY: all test clean

all: test_argh$(EXE) example$(EXE)

test: test_argh$(EXE)
	./test_argh$(EXE)

test_argh$(EXE): tests/test_argh.c argh.h
	$(CC) $(CFLAGS) -o $@ tests/test_argh.c $(LDFLAGS)

example$(EXE): example.c argh.h
	$(CC) $(CFLAGS) -o $@ example.c $(LDFLAGS)

clean:
	@echo Cleaning...
