# argh.h - Fast Argument Parsing Library for C

A high-performance, single-header, cross-platform argument parsing library written in pure C.

## Features

- **Single Header**: Just copy `argh.h` to your project
- **Pure C**: No C++ dependencies, works with C99 and later
- **Cross-Platform**: Works on Windows, Linux, macOS, and embedded systems
- **High Performance**: Minimal allocations, optimized for speed
- **Type-Safe**: Support for bool, int, float, double, and string types
- **Flexible**: Short (`-v`) and long (`--verbose`) options
- **User-Friendly**: Automatic help generation and clear error messages

## Quick Start

```c
#define ARGH_IMPLEMENTATION
#include "argh.h"

int main(int argc, char** argv) {
    argh_Parser parser;
    argh_init(&parser, argc, argv);

    /* Define options */
    argh_add(&parser, "v", "verbose", ARGH_BOOL, NULL, "Enable verbose output");
    argh_add(&parser, "o", "output", ARGH_STRING, "out.txt", "Output file");
    argh_add(&parser, "n", "count", ARGH_INT, "10", "Number of iterations");
    argh_require(&parser, "output");  /* Make --output required */

    /* Parse arguments */
    if (!argh_parse(&parser)) {
        argh_print_error(&parser);
        argh_print_help(&parser);
        return 1;
    }

    /* Check for help request */
    if (parser.help_requested) {
        argh_print_help(&parser);
        return 0;
    }

    /* Get values */
    bool verbose = argh_get_bool(&parser, "verbose");
    const char* output = argh_get_string(&parser, "output");
    int count = argh_get_int(&parser, "count");

    /* Get positional arguments */
    for (size_t i = 0; i < parser.positional_count; i++) {
        printf("File: %s\n", parser.positional[i]);
    }

    argh_free(&parser);
    return 0;
}
```

## API Reference

### Initialization

```c
void argh_init(argh_Parser* parser, int argc, char** argv);
```

Initialize the parser with command-line arguments.

```c
void argh_free(argh_Parser* parser);
```

Free allocated resources.

### Adding Options

```c
void argh_add(argh_Parser* parser,
              const char* short_name,
              const char* long_name,
              argh_Type type,
              const char* default_value,
              const char* description);
```

Add an option definition.

**Parameters:**

- `short_name`: Short option name (e.g., `"v"` for `-v`), can be `NULL`
- `long_name`: Long option name (e.g., `"verbose"` for `--verbose`), required
- `type`: One of `ARGH_BOOL`, `ARGH_STRING`, `ARGH_INT`, `ARGH_FLOAT`, `ARGH_DOUBLE`
- `default_value`: Default value if not specified, can be `NULL`
- `description`: Help text description

```c
void argh_require(argh_Parser* parser, const char* long_name);
```

Mark an option as required.

### Parsing

```c
bool argh_parse(argh_Parser* parser);
```

Parse command-line arguments. Returns `true` on success.

### Getting Values

```c
int argh_get_int(argh_Parser* parser, const char* name);
bool argh_get_bool(argh_Parser* parser, const char* name);
float argh_get_float(argh_Parser* parser, const char* name);
double argh_get_double(argh_Parser* parser, const char* name);
const char* argh_get_string(argh_Parser* parser, const char* name);
```

Get typed values by option name (short or long).

```c
bool argh_has(argh_Parser* parser, const char* name);
```

Check if an option was provided on the command line.

### Error Handling

```c
bool argh_has_errors(argh_Parser* parser);
void argh_print_error(argh_Parser* parser);
const char* argh_error_string(argh_ErrorCode code);
```

### Help Generation

```c
void argh_print_help(argh_Parser* parser);
void argh_set_help_width(argh_Parser* parser, int width);
```

## Usage Examples

### Boolean Flags

```c
/* -v, --verbose */
argh_add(&parser, "v", "verbose", ARGH_BOOL, NULL, "Enable verbose mode");

/* Usage: prog -v */
/* Usage: prog --verbose */
bool verbose = argh_get_bool(&parser, "verbose");
```

### String Values

```c
/* -o FILE, --output FILE */
argh_add(&parser, "o", "output", ARGH_STRING, "default.txt", "Output file");

/* Usage: prog -o myfile.txt */
/* Usage: prog --output=myfile.txt */
/* Usage: prog --output myfile.txt */
const char* output = argh_get_string(&parser, "output");
```

### Numeric Values

```c
/* -n NUM, --count NUM */
argh_add(&parser, "n", "count", ARGH_INT, "10", "Iteration count");

/* Usage: prog -n 100 */
/* Usage: prog --count=100 */
int count = argh_get_int(&parser, "count");

/* Float values */
argh_add(&parser, "r", "ratio", ARGH_FLOAT, "1.0", "Ratio");
float ratio = argh_get_float(&parser, "ratio");
```

### Positional Arguments

```c
/* After parsing, access positional arguments */
for (size_t i = 0; i < parser.positional_count; i++) {
    printf("Argument %zu: %s\n", i, parser.positional[i]);
}
```

### Combined Short Options

```c
/* Define multiple boolean flags */
argh_add(&parser, "a", NULL, ARGH_BOOL, NULL, "Option A");
argh_add(&parser, "b", NULL, ARGH_BOOL, NULL, "Option B");
argh_add(&parser, "c", NULL, ARGH_BOOL, NULL, "Option C");

/* Usage: prog -abc (equivalent to -a -b -c) */
```

### End of Options

```c
/* Use -- to separate options from positional arguments */
/* Usage: prog -v -- -not-an-option file.txt */
/* Everything after -- is treated as positional */
```

## Configuration

Define these macros before including the header to customize limits:

```c
#define ARGH_MAX_OPTIONS 64        /* Maximum number of options */
#define ARGH_MAX_POSITIONAL 128    /* Maximum positional arguments */
#define ARGH_MAX_ERRORS 8          /* Maximum errors to store */

#define ARGH_IMPLEMENTATION
#include "argh.h"
```

## Error Codes

| Code                           | Description                   |
| ------------------------------ | ----------------------------- |
| `ARGH_ERR_NONE`                | No error                      |
| `ARGH_ERR_UNKNOWN_OPTION`      | Unknown option provided       |
| `ARGH_ERR_MISSING_VALUE`       | Required value missing        |
| `ARGH_ERR_INVALID_VALUE`       | Value format invalid          |
| `ARGH_ERR_REQUIRED_MISSING`    | Required option not provided  |
| `ARGH_ERR_TOO_MANY_POSITIONAL` | Too many positional arguments |

## License

MIT License - Free for personal and commercial use.
