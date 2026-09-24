# Changelog

All notable changes to argh.h are listed here.

The format follows [Keep a Changelog](https://keepachangelog.com/en/1.1.0/), and the project uses [Semantic Versioning](https://semver.org/). Until v1.0, minor versions (0.x) may change the API.

## [Unreleased]

### Added

- **Commands**, like `git remote add`: `argh_commands` with `ARGH_CMD` and `ARGH_CMD_GROUP` tables, nested up to `ARGH_MAX_DEPTH` levels. Options of the parser are global and work before and after the command name.
- `argh_command` returns the selected command, `argh_run` calls its handler.
- Help for every command level, with a `Global options` section, and `tool help <command>`.
- A missing command is an error that lists the available commands.
- "Did you mean" suggestions for mistyped long options and commands: `unknown option '--verbsoe' (did you mean '--verbose'?)`. Also available as `argh_error.suggestion`. `ARGH_NO_SUGGEST` removes them.
- **Your own value types**: describe a type once as a constant `argh_type` (value name, parse function, optional format function) and use it with `argh_custom` / `ARGH_CUSTOM`. The parse function's reason appears in the error message, and the format function lets help show the default.
- `ARGH_NO_COMMANDS` removes command support, so programs without commands keep the code size of v0.2.

## [0.2.0] - 2026-09-24

v0.2 replaces the API. See [Upgrading from 0.1](README.md#upgrading-from-01).

### Changed

- **Options are bound to variables.** Builder calls (`argh_flag`, `argh_int`, `argh_string`, ...) or `static const` tables (`ARGH_FLAG`, `ARGH_INT`, ...) write parsed values straight into your variables. Defaults are the values the variables start with.
- **Zero heap allocations** and no global state. Strings point into `argv`. `argh_free` is gone.
- `argh_Parser` is now `argh_parser`, and `argh_parse` takes `argc` and `argv`.
- `argv` is reordered in place so that positional arguments come first.
- Stricter input: no octal numbers, no prefix matching of long options, `-o=file` is an error, numbers must be valid as a whole.
- About 2.7x faster than v0.1 and on par with `getopt_long`. See [BENCHMARKS.md](BENCHMARKS.md).

### Added

- Option kinds: counters (`-vvv`), `long`, enums, repeatable lists, named positionals, the rest of the positionals.
- Per-option flags: required, optional, hidden, negatable (`--no-color`), once.
- Built-in `-h`/`--help` and `-V`/`--version`, with defaults shown from your variables.
- Help sections with `ARGH_GROUP`.
- Error messages that name the option and the expected value, exit code 2 for usage errors, structured errors via `argh_last_error` and `argh_format_error`.
- `argh_given` to tell whether an option was passed.
- `argh_set_writer` to send all output elsewhere.
- POSIX mode that stops at the first positional argument.
- Compile-time warning when a table option points to a variable of the wrong type.
- Checks for mistakes in the definitions, such as duplicate names.
- The header compiles as C++, checked in CI.
- Benchmarks for parse speed, heap allocations, parser size and code size, compared with `getopt_long`. Run them with `make bench`, results in [BENCHMARKS.md](BENCHMARKS.md).
- Security policy ([SECURITY.md](SECURITY.md)) with private vulnerability reporting.
- Contribution guide ([CONTRIBUTING.md](CONTRIBUTING.md)) and this changelog.

### Removed

- `argh_add`, `argh_get_*`, `argh_has`, `argh_require`, `argh_free`, `argh_print_error` and the `ARGH_FLOAT` type.

## [0.1.0] - 2026-09-24

First public release.

### Added

- Single-header parser for C99: short (`-v`) and long (`--verbose`) options, `--name=value`, combined flags (`-abc`), `--` to end options.
- Typed values: `bool`, `int`, `float`, `double`, strings, with validation and range checks.
- Required options, positional arguments, generated help text.
- CI on Linux, macOS and Windows with GCC, Clang, MSVC and sanitizers.

### Fixed

Compared with the code before the public release:

- `--` is no longer taken as the value of an option.
- An invalid boolean value (`--flag=maybe`) no longer reads an uninitialized variable.
- Missing `<limits.h>` include, and a non-portable `strcasecmp` dependency.
- The test runner no longer reports failed tests as passed.

### Removed

- `argh_set_description` and `argh_set_help_width`, which had no effect.

[Unreleased]: https://github.com/ilyabrin/argh/compare/v0.2.0...HEAD
[0.2.0]: https://github.com/ilyabrin/argh/compare/v0.1.0...v0.2.0
[0.1.0]: https://github.com/ilyabrin/argh/releases/tag/v0.1.0
