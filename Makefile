# Makefile for argh.h
# Works with GCC, Clang and MinGW on Linux, macOS and Windows.
#
#   make            build tests and example
#   make test       build and run tests
#   make CC=clang   use a different compiler

CFLAGS ?= -std=c99 -Wall -Wextra -Wpedantic -Werror -O2

ifeq ($(OS),Windows_NT)
    EXE = .exe
    RM  = cmd /C del /Q
    # MinGW ships gcc but no "cc"
    ifeq ($(origin CC),default)
        CC = gcc
    endif
else
    EXE =
    RM  = rm -f
endif

.PHONY: all test clean

all: test_argh$(EXE) example$(EXE)

test: test_argh$(EXE)
	./test_argh$(EXE)

test_argh$(EXE): tests/test_argh.c argh.h
	$(CC) $(CFLAGS) -o $@ tests/test_argh.c $(LDFLAGS)

example$(EXE): example.c argh.h
	$(CC) $(CFLAGS) -o $@ example.c $(LDFLAGS)

clean:
	-$(RM) test_argh$(EXE) example$(EXE)
