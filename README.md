# argh.h

[![CI](https://github.com/ilyabrin/argh/actions/workflows/ci.yml/badge.svg)](https://github.com/ilyabrin/argh/actions/workflows/ci.yml)
[![License: MIT](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE)

A single-header command-line argument parser for C. Your options write straight into your variables.

```c
int jobs = 4;
argh_int(&p, 'j', "jobs", &jobs, "Parallel jobs");
```

> **Status: early development (v0.2).** Tested on every push, but the API may still change before v1.0.
> Feedback on the API is very welcome.

## Why argh

- **One file, no dependencies.** Copy `argh.h` into your project. Plain C99 that also compiles as C++.
- **No lookups after parsing.** Options are bound to variables, so a typo in an option name can't fail silently at runtime, and the compiler warns when a variable has the wrong type.
- **Zero heap allocations, no global state.** Strings point into `argv`. Option tables can be `static const`, so they live in read-only memory (flash on microcontrollers).
- **Help and errors included.** `--help`, `--version`, clear error messages and the right exit codes, without writing any of it.
- **Strict by design.** Ambiguous input is an error, never a guess. No octal surprises, no prefix matching, no `-o=file`.
- **As fast as `getopt_long`.** See [Benchmarks](#benchmarks).

## Quick start

Copy `argh.h` next to your code:

```c
#include <stdio.h>

#define ARGH_IMPLEMENTATION
#include "argh.h"

int main(int argc, char **argv)
{
    bool verbose = false;
    int jobs = 4;
    const char *output = "out.txt";
    const char *input = NULL;

    argh_parser p;
    argh_init(&p, "mytool", "Converts things");
    argh_flag(&p, 'v', "verbose", &verbose, "Verbose output");
    argh_int(&p, 'j', "jobs", &jobs, "Parallel jobs");
    argh_string(&p, 'o', "output", &output, "Output file");
    argh_pos(&p, "input", &input, "Input file");

    if (!argh_parse(&p, argc, argv))
        return argh_exit_code(&p);  /* --help, --version or an error was handled */

    printf("verbose=%d jobs=%d output=%s input=%s\n", verbose, jobs, output, input);
    return 0;
}
```

That's the whole setup. Defaults are the values you initialize your variables with, and there is nothing to free.

```console
$ ./mytool -v --jobs=8 data.csv
verbose=1 jobs=8 output=out.txt input=data.csv

$ ./mytool --help
Usage: mytool [OPTIONS] <input>

Converts things

Arguments:
  <input>               Input file

Options:
  -v, --verbose         Verbose output
  -j, --jobs <n>        Parallel jobs (default: 4)
  -o, --output <value>  Output file (default: out.txt)

  -h, --help            Print help

$ ./mytool --jobs many data.csv
mytool: invalid value 'many' for '--jobs': expected an integer
Try 'mytool --help' for more information.
$ echo $?
2
```

`#define ARGH_IMPLEMENTATION` goes in exactly one `.c` file. Other files just `#include "argh.h"`.

## Guide

### Option types

| Builder       | Table macro   | Variable       | Command line                             |
| ------------- | ------------- | -------------- | ---------------------------------------- |
| `argh_flag`   | `ARGH_FLAG`   | `bool`         | `-v`, `--verbose`, `--verbose=false`     |
| `argh_count`  | `ARGH_COUNT`  | `int`          | `-vvv` adds 3                            |
| `argh_int`    | `ARGH_INT`    | `int`          | `-j4`, `-j 4`, `--jobs=4`, `--jobs 0x10` |
| `argh_long`   | `ARGH_LONG`   | `long`         | same as `int`                            |
| `argh_double` | `ARGH_DOUBLE` | `double`       | `--ratio 0.5`, `--ratio=1e-3`            |
| `argh_string` | `ARGH_STRING` | `const char *` | `-o file`, `-ofile`, `--output=file`     |
| `argh_enum`   | `ARGH_ENUM`   | `int` (index)  | `--mode fast`                            |
| `argh_list`   | `ARGH_LIST`   | `argh_values`  | `-I a -I b`                              |
| `argh_pos`    | `ARGH_POS`    | `const char *` | one positional argument                  |
| `argh_rest`   | `ARGH_REST`   | `argh_values`  | all remaining positional arguments       |

Pass `0` as the short name for a long-only option, and `NULL` as the long name for a short-only one.

### Choices, lists and the rest of the arguments

```c
static const char *const formats[] = {"json", "yaml", "toml", NULL};
int format = 0;                           /* index into formats: "json" */
argh_enum(&p, 'f', "format", &format, formats, "Output format");

const char *dirs[16];
argh_values includes = ARGH_VALUES(dirs); /* up to 16 values */
argh_list(&p, 'I', "include", &includes, "Include directory");

argh_values files = {0};
argh_rest(&p, "files", &files, "Files to process");

/* after argh_parse(): */
for (int i = 0; i < files.count; i++)
    puts(files.items[i]);
```

### Required, hidden, negatable

Builder calls return the option, and modifiers can be chained:

```c
argh_required(argh_string(&p, 'o', "output", &output, "Output file"));
argh_negatable(argh_flag(&p, 0, "color", &color, "Colored output")); /* --color / --no-color */
argh_hidden(argh_flag(&p, 0, "debug-dump", &dump, "Not shown in help"));
argh_once(argh_string(&p, 'c', "config", &config, "Giving it twice is an error"));
argh_optional(argh_pos(&p, "output", &out_path, "Positional that may be omitted"));
```

Did the user actually pass an option, or is it the default? Ask by variable:

```c
if (argh_given(&p, &jobs))
    printf("jobs set explicitly\n");
```

### Option tables

For larger tools, or to keep the definitions in read-only memory, describe options as data. It's the same parser underneath.

```c
static struct { bool verbose; int jobs; const char *host; int port; } cfg = {false, 4, "localhost", 5432};

static const argh_opt options[] = {
    ARGH_FLAG('v', "verbose", &cfg.verbose, "Verbose output"),
    ARGH_INT('j', "jobs", &cfg.jobs, "Parallel jobs"),
    ARGH_GROUP("Database"),
    ARGH_STRING(0, "db-host", &cfg.host, "Database host", ARGH_REQUIRED),
    ARGH_INT(0, "db-port", &cfg.port, "Database port"),
    ARGH_END
};

argh_parser p;
argh_init(&p, "mytool", "Does useful things");
argh_table(&p, options);
```

- The last macro argument (flags such as `ARGH_REQUIRED | ARGH_ONCE`) is optional.
- The compiler warns if a variable has the wrong type, for example `ARGH_INT` on a `bool`. In C++ it's an error.
- `argh_table` can be called several times, so each module of a program can define its own options. Tables and builder calls can be mixed.
- `ARGH_GROUP` starts a new section in the help output.

### Commands

Tools like `git` or `docker` group their work into commands: `tool build`, `tool remote add`. Describe them as a table, each with its own options:

```c
static bool verbose, release, force;
static const char *name, *url;

static int build(argh_parser *p, void *app)
{
    printf("building%s\n", release ? " (release)" : "");
    return 0;
}

static const argh_opt build_opts[] = {
    ARGH_FLAG('r', "release", &release, "Optimized build"),
    ARGH_END
};

static const argh_opt add_opts[] = {
    ARGH_FLAG('f', "force", &force, "Overwrite an existing remote"),
    ARGH_POS("name", &name, "Remote name"),
    ARGH_POS("url", &url, "Remote URL"),
    ARGH_END
};

static const argh_cmd remote_cmds[] = {
    ARGH_CMD("add", "Add a remote", add_opts),
    ARGH_CMD("list", "List remotes", NULL),
    ARGH_CMD_END
};

static const argh_cmd commands[] = {
    ARGH_CMD("build", "Build the project", build_opts, build),
    ARGH_CMD_GROUP("remote", "Manage remotes", remote_cmds),
    ARGH_CMD_END
};

int main(int argc, char **argv)
{
    argh_parser p;
    argh_init(&p, "tool", "Builds things");
    argh_flag(&p, 'v', "verbose", &verbose, "Verbose output");   /* a global option */
    argh_commands(&p, commands);

    if (!argh_parse(&p, argc, argv))
        return argh_exit_code(&p);

    if (argh_command(&p) == &remote_cmds[0])
        printf("adding %s -> %s\n", name, url);
    return argh_run(&p, NULL);   /* calls the handler of the selected command, if any */
}
```

- `ARGH_CMD(name, help, options[, handler])`: the handler is optional. Pass `NULL` for a command without options.
- `ARGH_CMD_GROUP(name, help, subcommands)`: a command that only holds other commands, like `remote`.
- Options added to the parser itself are **global**: they work before and after the command name (`tool -v build` and `tool build -v`). A command's own options only work after its name.
- Dispatch either way: `argh_run(&p, app)` calls the handler and passes `app` through, or `argh_command(&p)` returns the selected command for your own `switch`.

Every level gets its own help, and `tool help remote add` works like `tool remote add --help`:

```console
$ ./tool remote add --help
Usage: tool remote add [OPTIONS] <name> <url>

Add a remote

Arguments:
  <name>         Remote name
  <url>          Remote URL

Options:
  -f, --force    Overwrite an existing remote

Global options:
  -v, --verbose  Verbose output

  -h, --help     Print help
```

When a command is missing, the error lists what's available:

```console
$ ./tool remote
tool: 'remote' needs a command

Commands:
  add   Add a remote
  list  List remotes

Try 'tool remote --help' for more information.
```

Command names match exactly, like options. A program with commands can't have positional arguments of its own, and neither can a command group: positionals belong to the commands that do the work.

### Help and version

`-h`/`--help` always works. `-V`/`--version` works once you set a version:

```c
argh_version(&p, "1.4.2");   /* ./mytool --version  ->  mytool 1.4.2 */
```

Help shows the defaults from your variables before parsing, so they are always accurate. `argh_parse` returns `false` after printing help, and `argh_exit_code` returns 0.

Need `-h` for something else, like `--host`? Turn the built-ins off with `argh_set_flags(&p, ARGH_NO_AUTO_HELP)` and call `argh_print_help(&p)` yourself.

### Errors

On an error, `argh_parse` prints a one-line message plus a hint to stderr and returns `false`. `argh_exit_code` then returns 2, the Unix convention for usage errors.

Typos in long options and command names get a suggestion:

```console
$ ./tool --verbsoe build
tool: unknown option '--verbsoe' (did you mean '--verbose'?)
Try 'tool --help' for more information.

$ ./tool remote ad origin https://example.com/app.git
tool: unknown command 'ad' (did you mean 'add'?)
Try 'tool remote --help' for more information.
```

Suggestions only name options and commands that are valid at that point, never hidden options, and only when the match is close (up to 2 edits, counting a swap of two letters as one). They are computed only after an error, so they cost nothing on a successful parse. `ARGH_NO_SUGGEST` removes them.

To handle errors yourself:

```c
const argh_error *err = argh_last_error(&p);    /* err->code: ARGH_E_UNKNOWN_OPTION, ... */

char message[200];
argh_format_error(&p, message, sizeof message); /* same text, no stdio needed */
```

To send all output somewhere else (a log, a UART, a test buffer), set a writer:

```c
static void my_writer(void *ctx, int to_stderr, const char *text, size_t len)
{
    /* write len bytes of text */
}

argh_set_writer(&p, my_writer, NULL);
```

### Parsing rules

| Input                 | Meaning                                                                     |
| --------------------- | --------------------------------------------------------------------------- |
| `-abc`                | `-a -b -c` when all are flags                                               |
| `-vo file`, `-vofile` | flags, then an option with a value                                          |
| `-o -x`               | an option that needs a value always takes the next argument (`-n -5` works) |
| `--`                  | everything after it is positional. Never taken as a value                   |
| `-`                   | a positional argument (stdin by convention)                                 |
| `tool a -v b`         | options and positionals can be mixed                                        |

argh is strict where other parsers guess:

- `-o=file` is an error: write `-o file` or `--output=file`.
- `--verb` does not match `--verbose`. Prefix matching breaks scripts when new options are added.
- `010` is ten, not eight. Hex needs `0x`.
- Numbers are checked completely: `10abc`, `" 5"` and out-of-range values are errors.
- A negative number on its own (`-5`) is an unknown option. Pass it after `--`.

`argh_parse` reorders `argv` in place so that positional arguments come first, in their original order. That's what lets `argh_rest` point into `argv` without copying. With `argh_set_flags(&p, ARGH_POSIX)`, parsing stops at the first positional argument, which suits wrapper tools like `sudo` or `time`.

### Configuration

Define before including `argh.h`:

| Macro              | Default | Meaning                                                        |
| ------------------ | ------: | -------------------------------------------------------------- |
| `ARGH_BUILDER_CAP` |      32 | Options that builder calls can add. `0` if you only use tables |
| `ARGH_MAX_OPTS`    |      64 | Options on the active command path, all tables combined        |
| `ARGH_MAX_TABLES`  |       8 | Tables per parser. The builder counts as one                   |
| `ARGH_MAX_DEPTH`   |       4 | Levels of nested commands                                      |
| `ARGH_NO_SUGGEST`  |         | Define to remove "did you mean" suggestions (about 0.8 KB)     |

Mistakes in the definitions, such as two options with the same name or a missing variable, are reported by `argh_parse` as `ARGH_E_CONFIG`. The checks for duplicate names and for the command tree run in builds without `NDEBUG`.

## API reference

```c
/* Setup */
void argh_init(argh_parser *p, const char *name, const char *about);  /* name NULL: from argv[0] */
void argh_version(argh_parser *p, const char *version);
void argh_set_flags(argh_parser *p, unsigned flags);                   /* ARGH_POSIX, ARGH_NO_AUTO_HELP */
void argh_set_writer(argh_parser *p, argh_write_fn write, void *ctx);
void argh_table(argh_parser *p, const argh_opt *table);
void argh_commands(argh_parser *p, const argh_cmd *commands);

/* Builder: each returns the option, or NULL when ARGH_BUILDER_CAP is exceeded */
argh_opt *argh_flag  (argh_parser *p, char s, const char *l, bool *target, const char *help);
argh_opt *argh_count (argh_parser *p, char s, const char *l, int *target, const char *help);
argh_opt *argh_int   (argh_parser *p, char s, const char *l, int *target, const char *help);
argh_opt *argh_long  (argh_parser *p, char s, const char *l, long *target, const char *help);
argh_opt *argh_double(argh_parser *p, char s, const char *l, double *target, const char *help);
argh_opt *argh_string(argh_parser *p, char s, const char *l, const char **target, const char *help);
argh_opt *argh_enum  (argh_parser *p, char s, const char *l, int *target, const char *const *choices, const char *help);
argh_opt *argh_list  (argh_parser *p, char s, const char *l, argh_values *target, const char *help);
argh_opt *argh_pos   (argh_parser *p, const char *name, const char **target, const char *help);
argh_opt *argh_rest  (argh_parser *p, const char *name, argh_values *target, const char *help);
argh_opt *argh_group (argh_parser *p, const char *title);

/* Modifiers: accept NULL, return their argument */
argh_opt *argh_required(argh_opt *o);
argh_opt *argh_optional(argh_opt *o);
argh_opt *argh_hidden(argh_opt *o);
argh_opt *argh_negatable(argh_opt *o);
argh_opt *argh_once(argh_opt *o);
argh_opt *argh_metavar(argh_opt *o, const char *metavar);   /* "<file>" instead of "<value>" */

/* Parsing and results */
bool argh_parse(argh_parser *p, int argc, char **argv);
int argh_exit_code(const argh_parser *p);                    /* 0, or 2 after an error */
bool argh_given(const argh_parser *p, const void *target);
const argh_cmd *argh_command(const argh_parser *p);         /* selected command, or NULL */
int argh_run(argh_parser *p, void *user);                     /* calls its handler */
const argh_error *argh_last_error(const argh_parser *p);
size_t argh_format_error(const argh_parser *p, char *buf, size_t size);
void argh_print_help(const argh_parser *p);
```

## Upgrading from 0.1

v0.2 replaces the API. The idea stays the same, but values now go straight into variables:

```c
/* v0.1 */
argh_Parser parser;
argh_init(&parser, argc, argv);
argh_add(&parser, "n", "count", ARGH_INT, "10", "Iterations");
if (!argh_parse(&parser)) { argh_print_error(&parser); return 1; }
int count = argh_get_int(&parser, "count");
argh_free(&parser);

/* v0.2 */
int count = 10;
argh_parser p;
argh_init(&p, NULL, NULL);
argh_int(&p, 'n', "count", &count, "Iterations");
if (!argh_parse(&p, argc, argv)) return argh_exit_code(&p);
```

| v0.1                               | v0.2                                   |
| ---------------------------------- | -------------------------------------- |
| `argh_Parser`                      | `argh_parser`                          |
| `argh_add(..., ARGH_BOOL, ...)`    | `argh_flag(...)`                       |
| `ARGH_FLOAT`                       | `argh_double` (no separate float type) |
| default value as a string          | initialize the variable                |
| `argh_get_*(&p, "name")`           | read the variable                      |
| `argh_has(&p, "name")`             | `argh_given(&p, &variable)`            |
| `argh_require(&p, "name")`         | `argh_required(argh_...(...))`         |
| `parser.positional[i]`             | `argh_pos` / `argh_rest`               |
| define and check your own `--help` | built in                               |
| `argh_free`                        | nothing to free                        |

## Known limitations

Planned for upcoming versions:

- **Custom value types** through a callback and constraints between options arrive in v0.3.
- **A reduced build for microcontrollers** (no stdio, no help text) arrives in v0.4. Today argh adds about 12 KB of code and text on Linux.
- Floating-point values follow the C locale's decimal separator, like `strtod`.

## Benchmarks

Time to set up a parser with 30 options and parse 17 arguments, release builds (`-O2 -DNDEBUG`), from CI:

| Platform           | argh (table) | argh (builder) | getopt_long |
| ------------------ | -----------: | -------------: | ----------: |
| macOS, Clang       |       447 ns |         499 ns |      476 ns |
| Linux, GCC         |       549 ns |         618 ns |      511 ns |
| Linux, Clang       |       628 ns |         671 ns |      625 ns |
| Windows, MinGW GCC |       934 ns |         979 ns |      853 ns |

argh makes zero heap allocations. Details, memory, code size and the method: [BENCHMARKS.md](BENCHMARKS.md). Run them with `make bench`.

## Running the tests

```sh
make test     # test suite
make cxx      # check that argh.h compiles as C++
```

## Contributing

Bug reports, tests and fixes are welcome. See [CONTRIBUTING.md](CONTRIBUTING.md) for how to build, test and send a pull request, and [CHANGELOG.md](CHANGELOG.md) for what changed between versions.

## License

[MIT](LICENSE)
