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

Release builds (`-O2 -DNDEBUG`) on CI runners, v0.3:

| Platform           | argh (table) | argh (builder) | getopt_long |
| ------------------ | -----------: | -------------: | ----------: |
| macOS, Clang       |       422 ns |         450 ns |      399 ns |
| Linux, Clang       |       707 ns |         780 ns |      629 ns |
| Linux, GCC         |       723 ns |         807 ns |      618 ns |
| Windows, MinGW GCC |     1,098 ns |       1,147 ns |      890 ns |

argh takes 6% to 23% longer than `getopt_long` while also validating every value and supporting commands, rules and generated help. In absolute terms that is a fraction of a microsecond, once, at program start. The builder costs a little more than the table because it fills in the option list on every run.

CI runners are shared machines and their timings vary between runs: the same v0.2 code measured 0% to 10% behind `getopt_long` there. On one machine, v0.3 is about 3% slower than v0.2 (1,018 ns against 993 ns), the cost of the checks for commands and rules.

### Debug builds

Without `NDEBUG`, `argh_parse` also checks the definitions for mistakes such as duplicate option names. The check compares every pair of options, and with 30 options it adds about 1.5 µs. That is a one-time cost at program start, like an `assert`, and it catches real mistakes. Release builds with `-DNDEBUG` skip it.

### Compared with v0.1

Same machine (Intel Core i5-12400F, Windows 11, MinGW GCC 13.2), same workload:

|                  |  v0.1.0 |                    v0.3 (table) |
| ---------------- | ------: | ------------------------------: |
| Time per parse   | 2.73 µs |                         1.02 µs |
| Heap allocations |      11 |                               0 |
| `sizeof` parser  | 4,880 B | 248 B + 1,848 B builder storage |

v0.1 copied every value to the heap and looked options up by name twice: once while parsing and once in every `argh_get_*` call. Since v0.2 values go straight into variables, so both costs are gone.

## Memory

- **Heap:** zero allocations, on success, on errors and when printing help. The benchmark counts calls to `malloc`/`realloc`, and valgrind confirms it on Linux.
- **Parser:** 248 bytes of state (208 with `ARGH_NO_COMMANDS`), plus storage for the options that builder calls add: `ARGH_BUILDER_CAP` (default 32) times `sizeof(argh_opt)`. `argh_opt` is 56 bytes on 64-bit systems and 28 bytes on 32-bit microcontrollers. Programs that only use tables can set `ARGH_BUILDER_CAP` to 0.
- **Option tables:** a `static const` table goes into read-only memory. On a Cortex-M4 each option takes 28 bytes of flash and no RAM.

## Code size

The `.text` added to a minimal 3-option program, compared with the same program without a parser. Built with `-Os -ffunction-sections -fdata-sections -Wl,--gc-sections`.

| Platform           | v0.3    | v0.3 reduced | v0.2    | v0.1   | getopt_long |
| ------------------ | ------: | -----------: | ------: | -----: | ----------: |
| Linux, GCC (CI)    | 17.1 KB |      13.7 KB | 12.1 KB | 7.3 KB |      0.6 KB |
| Linux, Clang (CI)  | 20.9 KB |              | 13.8 KB | 7.5 KB |      0.5 KB |
| Windows, MinGW GCC | 17.6 KB |              | 11.1 KB | 5.6 KB |     25.4 KB |

"Reduced" is `-DARGH_NO_COMMANDS -DARGH_NO_SUGGEST`, for programs that don't need commands or "did you mean" suggestions.

Read this one with care:

- **Each version is bigger than the last** because it does more. v0.2 added formatted help, detailed errors, enums, lists and definition checks; v0.3 added commands, suggestions, custom types and rules. On Linux, `size` counts read-only data as text, so the help and error strings are part of these numbers.
- **Unused features cost less than they look.** The rule checker is only linked when `argh_rules` is called, and the two `ARGH_NO_*` flags remove commands and suggestions.
- **getopt_long is only counted where it is linked statically**, as in MinGW. On Linux it lives in the shared C library and adds almost nothing to your binary, so for desktop Linux getopt is the smaller choice. On MinGW it pulls in error-printing and locale code.
- A build for microcontrollers, without stdio and help text, is planned for v0.4. It will be measured with a static bare-metal ARM build.
- macOS is not measured: its `size` tool reports page-aligned segments, which hides differences of a few KB.

## Method

- Source: [bench/bench_parse.c](bench/bench_parse.c) (speed and memory), [bench/size.sh](bench/size.sh) (code size).
- Speed: best of 5 runs, 200,000 iterations each, `-O2 -DNDEBUG`. Each iteration gets a fresh copy of `argv`, because both parsers reorder it.
- Allocations: calls to `malloc`/`realloc` are counted by wrapping them in the benchmark.
- The `getopt_long` code does the same work as argh: typed values, numbers converted with `strtol`/`strtod`.
- CI runners are shared machines, so absolute times vary between runs. Compare columns within one row.
