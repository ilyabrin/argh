# Benchmarks

How fast and how small argh.h is, compared with `getopt_long`, the parser most C programs already use.

Run them yourself:

```sh
make bench
```

CI runs the same benchmarks on Linux, macOS and Windows (MinGW) on every push. CI does not compare the timings, but it does check that both parsers read identical values from the workload.

## Current results (v0.1.0)

This is the baseline for the new API in v0.2, which has concrete targets: zero heap allocations, a parser under 256 bytes, and parse speed on par with `getopt_long`.

Machine: Intel Core i5-12400F, Windows 11, GCC 13.2 (MinGW-w64).

### Parse speed

30 options defined, 17 arguments on the command line (short and long options, `--name=value`, combined flags `-abc`, numbers, positionals). One iteration is the full job a program does at startup: set up the parser, parse, read every value back.

| Parser      | Time per parse | Heap allocations |
| ----------- | -------------: | ---------------: |
| argh v0.1   |        2.76 µs |               11 |
| getopt_long |        0.91 µs |      not counted |

argh v0.1 is about 3x slower than `getopt_long`. CI shows the same ratio on Linux (GCC, Clang) and macOS (Clang), so this is not specific to one machine. The main costs are copying every value to the heap and looking options up by string twice: once while parsing and once in each `argh_get_*` call. The v0.2 design removes both.

In absolute terms, both finish in a few microseconds, far below anything a user can notice at program startup. The numbers matter for embedded targets and for tools that parse many command lines, such as shells, test runners and fuzzers.

### Memory

|                       | argh v0.1            |
| --------------------- | -------------------- |
| `sizeof(argh_Parser)` | 4,880 bytes          |
| Heap allocations      | 11 for this workload |

Where the 11 come from: each of the 6 option values is copied to the heap, the 2 `--name=value` options are copied a second time and also allocate a temporary buffer for the name lookup (+4), and the array that tracks the copies is allocated once (+1).

The parser reserves fixed arrays for 64 options, 128 positionals and 8 errors, whether you use them or not. That is fine on a desktop and too much for a microcontroller with a few KB of RAM.

### Code size

The `.text` added to a minimal 3-option program, compared to the same program without a parser. Built with `-Os -ffunction-sections -fdata-sections -Wl,--gc-sections`.

| Parser      | Code added |
| ----------- | ---------: |
| argh v0.1   |    5.6 KB  |
| getopt_long |   25.4 KB  |

Read this one with care:

- **getopt_long is only counted where it is linked statically**, as in MinGW. On Linux it lives in the shared C library and adds about 0.6 KB (CI measurement), while argh adds about 7.5 KB. For desktop Linux, getopt is the smaller option.
- macOS is not measured: its `size` tool reports page-aligned segments, which hides differences of a few KB.
- On MinGW, `getopt_long` pulls in error-printing and locale code, which explains most of its size.
- Functions that argh uses from the C library itself (`strtol`, `strtod`, `printf`) are not counted when the C library is shared.

A fair size comparison for embedded targets needs a static, bare-metal build. That comes with the ARM build planned for v0.4.

## Method

- Source: [bench/bench_parse.c](bench/bench_parse.c) (speed and memory), [bench/size.sh](bench/size.sh) (code size).
- Speed: best of 5 runs, 200,000 iterations each, `-O2`. Each iteration gets a fresh copy of `argv`, because `getopt_long` reorders it.
- Allocations: argh's `malloc`/`realloc` calls are counted by wrapping them in the benchmark.
- The `getopt_long` code does the same work as argh: typed values, numbers converted with `strtol`/`strtod`.
