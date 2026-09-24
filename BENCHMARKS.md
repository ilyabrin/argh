# Benchmarks

How fast and how small argh.h is, compared with `getopt_long`, the parser most C programs already use.

Run them yourself:

```sh
make bench
```

CI runs the same benchmarks on Linux, macOS and Windows (MinGW) on every push. CI does not compare the timings, but it does check that every parser reads identical values from the workload.

## Parse speed

The workload: 30 options defined, 17 arguments on the command line (short and long options, `--name=value`, combined flags `-abc`, numbers, positionals). One iteration is the full job a program does at startup: set up the parser, parse, read every value back.

argh is measured two ways: with builder calls (`argh_int(&p, ...)`) and with a `static const` table (`ARGH_INT(...)`).

Release builds (`-O2 -DNDEBUG`) on CI runners:

| Platform           | argh (table) | argh (builder) | getopt_long |
| ------------------ | -----------: | -------------: | ----------: |
| macOS, Clang       |       447 ns |         499 ns |      476 ns |
| Linux, GCC         |       549 ns |         618 ns |      511 ns |
| Linux, Clang       |       628 ns |         671 ns |      625 ns |
| Windows, MinGW GCC |       934 ns |         979 ns |      853 ns |

argh is within about 10% of `getopt_long`, and faster on macOS. The builder costs a little more than the table because it fills in the option list on every run.

### Debug builds

Without `NDEBUG`, `argh_parse` also checks the definitions for duplicate option names. The check compares every pair of options, and with 30 options it adds about 1.5 µs. That is a one-time cost at program start, like an `assert`, and it catches real mistakes. Release builds with `-DNDEBUG` skip it.

### Compared with v0.1

Same machine (Intel Core i5-12400F, Windows 11, MinGW GCC 13.2), same workload:

|                  |  v0.1.0 |                    v0.2 (table) |
| ---------------- | ------: | ------------------------------: |
| Time per parse   | 2.73 µs |                         0.99 µs |
| Heap allocations |      11 |                               0 |
| `sizeof` parser  | 4,880 B | 184 B + 1,848 B builder storage |

v0.1 copied every value to the heap and looked options up by name twice: once while parsing and once in every `argh_get_*` call. v0.2 writes values straight into variables, so both costs are gone.

## Memory

- **Heap:** zero allocations, on success, on errors and when printing help. The benchmark counts calls to `malloc`/`realloc`, and valgrind confirms it on Linux.
- **Parser:** 184 bytes of state, plus storage for the options that builder calls add: `ARGH_BUILDER_CAP` (default 32) times `sizeof(argh_opt)`. `argh_opt` is 56 bytes on 64-bit systems and 28 bytes on 32-bit microcontrollers. Programs that only use tables can set `ARGH_BUILDER_CAP` to 0.
- **Option tables:** a `static const` table goes into read-only memory. On a Cortex-M4 each option takes 28 bytes of flash and no RAM.

## Code size

The `.text` added to a minimal 3-option program, compared with the same program without a parser. Built with `-Os -ffunction-sections -fdata-sections -Wl,--gc-sections`.

| Platform           | argh v0.2 | argh v0.1 | getopt_long |
| ------------------ | --------: | --------: | ----------: |
| Linux, GCC (CI)    |   12.1 KB |    7.3 KB |      0.6 KB |
| Linux, Clang (CI)  |   13.8 KB |    7.5 KB |      0.5 KB |
| Windows, MinGW GCC |   11.1 KB |    5.6 KB |     25.4 KB |

Read this one with care:

- **v0.2 is bigger than v0.1** because it does more: formatted help with defaults, detailed error messages, enums, lists, negatable flags and definition checks. On Linux, `size` counts read-only data as text, so the help and error strings are part of these numbers.
- **getopt_long is only counted where it is linked statically**, as in MinGW. On Linux it lives in the shared C library and adds almost nothing to your binary, so for desktop Linux getopt is the smaller choice. On MinGW it pulls in error-printing and locale code.
- A reduced build for microcontrollers, without stdio and help text, is planned for v0.4. It will be measured with a static bare-metal ARM build.
- macOS is not measured: its `size` tool reports page-aligned segments, which hides differences of a few KB.

## Method

- Source: [bench/bench_parse.c](bench/bench_parse.c) (speed and memory), [bench/size.sh](bench/size.sh) (code size).
- Speed: best of 5 runs, 200,000 iterations each, `-O2 -DNDEBUG`. Each iteration gets a fresh copy of `argv`, because both parsers reorder it.
- Allocations: calls to `malloc`/`realloc` are counted by wrapping them in the benchmark.
- The `getopt_long` code does the same work as argh: typed values, numbers converted with `strtol`/`strtod`.
- CI runners are shared machines, so absolute times vary between runs. Compare columns within one row.
