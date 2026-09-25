# Makefile for argh.h
# Works with GCC, Clang and MinGW on Linux, macOS and Windows.
#
#   make            build tests and examples
#   make test       build and run tests
#   make examples   build the examples in examples/
#   make smoke      run the examples and check their output
#   make bench      run benchmarks (speed and code size)
#   make cxx        check that argh.h compiles as C++
#   make CC=clang   use a different compiler

CFLAGS ?= -std=c99 -Wall -Wextra -Wpedantic -Werror -O2

ifeq ($(OS),Windows_NT)
    EXE = .exe
    # cmd's del wants backslashes
    RM  = cmd /C del /Q
    fixpath = $(subst /,\,$(1))
    # MinGW ships gcc but no "cc"
    ifeq ($(origin CC),default)
        CC = gcc
    endif
else
    EXE =
    RM  = rm -f
    fixpath = $(1)
endif

EXAMPLES = examples/wc$(EXE) examples/logship$(EXE) examples/pkg/pkg$(EXE)
PKG_SRC  = examples/pkg/main.c examples/pkg/install.c examples/pkg/remote.c examples/pkg/exec.c

.PHONY: all test examples smoke bench cxx clean

all: test_argh$(EXE) $(EXAMPLES)

test: test_argh$(EXE)
	./test_argh$(EXE)

test_argh$(EXE): tests/test_argh.c argh.h
	$(CC) $(CFLAGS) -o $@ tests/test_argh.c $(LDFLAGS)

examples: $(EXAMPLES)

examples/wc$(EXE): examples/wc.c argh.h
	$(CC) $(CFLAGS) -o $@ examples/wc.c $(LDFLAGS)

examples/logship$(EXE): examples/logship.c argh.h
	$(CC) $(CFLAGS) -o $@ examples/logship.c $(LDFLAGS)

examples/pkg/pkg$(EXE): $(PKG_SRC) examples/pkg/pkg.h argh.h
	$(CC) $(CFLAGS) -o $@ $(PKG_SRC) $(LDFLAGS)

smoke: $(EXAMPLES)
	sh examples/smoke.sh

# Benchmarks measure a release build: C11 for timespec_get, -O2, NDEBUG
bench: bench_parse$(EXE)
	./bench_parse$(EXE)
	sh bench/size.sh $(CC)

bench_parse$(EXE): bench/bench_parse.c argh.h
	$(CC) -std=c11 -O2 -DNDEBUG -Wall -Wextra -o $@ bench/bench_parse.c

cxx: tests/cxx_check.cpp argh.h
	$(CXX) -std=c++11 -Wall -Wextra -Wpedantic -Werror -o cxx_check$(EXE) tests/cxx_check.cpp

clean:
	-$(RM) $(call fixpath,test_argh$(EXE) bench_parse$(EXE) cxx_check$(EXE) $(EXAMPLES))
