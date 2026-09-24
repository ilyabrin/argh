# Changelog

All notable changes to argh.h are listed here.

The format follows [Keep a Changelog](https://keepachangelog.com/en/1.1.0/), and the project uses [Semantic Versioning](https://semver.org/). Until v1.0, minor versions (0.x) may change the API.

## [Unreleased]

### Added

- Benchmarks for parse speed, heap allocations, parser size and code size, compared with `getopt_long`. Run them with `make bench`, results in [BENCHMARKS.md](BENCHMARKS.md).
- Security policy ([SECURITY.md](SECURITY.md)) with private vulnerability reporting.
- Contribution guide ([CONTRIBUTING.md](CONTRIBUTING.md)) and this changelog.

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

[Unreleased]: https://github.com/ilyabrin/argh/compare/v0.1.0...HEAD
[0.1.0]: https://github.com/ilyabrin/argh/releases/tag/v0.1.0
