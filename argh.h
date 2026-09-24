/*
 * argh.h - v0.2.0 - Single-header command-line argument parser for C
 *
 * Status: early development. The API may change before v1.0.
 *
 * Options write straight into your variables. No heap allocations, no global
 * state, and option tables can be `static const` (read-only memory).
 *
 * USAGE
 *   In exactly one .c file:
 *       #define ARGH_IMPLEMENTATION
 *       #include "argh.h"
 *   Everywhere else just #include "argh.h".
 *
 * EXAMPLE
 *   int main(int argc, char **argv) {
 *       bool verbose = false;
 *       int jobs = 4;
 *
 *       argh_parser p;
 *       argh_init(&p, "mytool", "Does useful things");
 *       argh_flag(&p, 'v', "verbose", &verbose, "Verbose output");
 *       argh_int(&p, 'j', "jobs", &jobs, "Parallel jobs");
 *
 *       if (!argh_parse(&p, argc, argv))
 *           return argh_exit_code(&p);  // --help, --version or an error
 *
 *       // use verbose and jobs
 *   }
 *
 * CONFIGURATION (define before including)
 *   ARGH_BUILDER_CAP  options that argh_flag()/argh_int()/... can add (32)
 *   ARGH_MAX_OPTS     options on the active command path, all tables (64)
 *   ARGH_MAX_TABLES   tables per parser, the builder counts as one (8)
 *   ARGH_MAX_DEPTH    levels of nested commands (4)
 *   ARGH_NO_SUGGEST   no "did you mean" suggestions in error messages
 *   ARGH_NO_COMMANDS  no commands: smaller code for programs without them
 *
 * LICENSE: MIT (see end of file)
 */

#ifndef ARGH_H_INCLUDED
#define ARGH_H_INCLUDED

#include <stdbool.h>
#include <stddef.h>

#ifndef ARGH_BUILDER_CAP
#define ARGH_BUILDER_CAP 32
#endif

#ifndef ARGH_MAX_OPTS
#define ARGH_MAX_OPTS 64
#endif

#ifndef ARGH_MAX_TABLES
#define ARGH_MAX_TABLES 8
#endif

#ifndef ARGH_MAX_DEPTH
#define ARGH_MAX_DEPTH 4
#endif

#ifdef __cplusplus
extern "C"
{
#endif

    /* ============================================================================
     * Types
     * ============================================================================ */

    /* What an option entry is. Stored in argh_opt.kind. */
    enum argh_kind
    {
        ARGH_K_END = 0, /* table terminator */
        ARGH_K_FLAG,    /* bool:          -v, --verbose, --no-verbose */
        ARGH_K_COUNT,   /* int:           -vvv */
        ARGH_K_INT,     /* int:           -j 4, --jobs=4 */
        ARGH_K_LONG,    /* long:          same as int */
        ARGH_K_DOUBLE,  /* double:        --ratio 0.5 */
        ARGH_K_STRING,  /* const char *:  -o file */
        ARGH_K_ENUM,    /* int (index):   --mode fast */
        ARGH_K_LIST,    /* argh_values:   -I a -I b */
        ARGH_K_POS,     /* const char *:  positional argument */
        ARGH_K_REST,    /* argh_values:   all remaining positionals */
        ARGH_K_GROUP,   /* help section heading */
        ARGH_K_CUSTOM   /* your type:     --size 10M, see argh_type */
    };

    /* Per-option flags. Stored in argh_opt.flags, combine with |. */
    enum argh_opt_flag
    {
        ARGH_REQUIRED = 1 << 0,  /* must be given */
        ARGH_OPTIONAL = 1 << 1,  /* positional may be omitted */
        ARGH_HIDDEN = 1 << 2,    /* not shown in help */
        ARGH_NEGATABLE = 1 << 3, /* flag also accepts --no-<name> */
        ARGH_ONCE = 1 << 4       /* giving it twice is an error */
    };

    /* Parser-wide flags for argh_set_flags(). */
    enum argh_parser_flag
    {
        ARGH_POSIX = 1 << 0,       /* stop parsing options at the first positional */
        ARGH_NO_AUTO_HELP = 1 << 1 /* no built-in -h/--help and -V/--version */
    };

    typedef enum argh_err
    {
        ARGH_E_NONE = 0,
        ARGH_E_UNKNOWN_OPTION,      /* --verbos */
        ARGH_E_MISSING_VALUE,       /* --jobs at the end of the line */
        ARGH_E_INVALID_VALUE,       /* --jobs abc, --mode slow */
        ARGH_E_OUT_OF_RANGE,        /* --jobs 99999999999 */
        ARGH_E_UNEXPECTED_VALUE,    /* --count=3 on a counter */
        ARGH_E_SHORT_EQUALS,        /* -o=file (use -o file) */
        ARGH_E_UNEXPECTED_ARGUMENT, /* a positional nobody asked for */
        ARGH_E_MISSING_REQUIRED,    /* required option or positional absent */
        ARGH_E_REPEATED,            /* ARGH_ONCE option given twice */
        ARGH_E_TOO_MANY_VALUES,     /* list buffer full */
        ARGH_E_CONFIG,              /* mistake in the option definitions */
        ARGH_E_UNKNOWN_COMMAND,     /* tool remtoe */
        ARGH_E_MISSING_COMMAND      /* tool (when a command is required) */
    } argh_err;

    /* One option, positional or help heading. Build it with the ARGH_* macros
     * or the argh_* builder functions rather than by hand. */
    typedef struct argh_opt
    {
        char short_name;       /* 'v' for -v, 0 for none */
        const char *long_name; /* "verbose" for --verbose, positional name */
        unsigned char kind;    /* enum argh_kind */
        unsigned char flags;   /* enum argh_opt_flag */
        void *target;          /* variable the value is written to */
        const void *extra;     /* ARGH_K_ENUM: NULL-terminated choices,
                                  ARGH_K_CUSTOM: const argh_type * */
        const char *help;      /* help text, group title for ARGH_K_GROUP */
        const char *metavar;   /* value name in help, NULL for a default */
    } argh_opt;

    /* A list of strings: repeated options (ARGH_K_LIST) or the remaining
     * positionals (ARGH_K_REST). Strings point into argv. */
    typedef struct argh_values
    {
        const char **items;
        int count;
        int capacity; /* ARGH_K_LIST only: size of items */
    } argh_values;

    /* A value type of your own, for ARGH_CUSTOM / argh_custom. Define it once
     * as a constant and reuse it for any number of options:
     *
     *     static const argh_type size_type = {"<size>", parse_size, format_size};
     */
    typedef struct argh_type
    {
        /* Name of the value in help, such as "<size>". NULL for "<value>". */
        const char *metavar;

        /* Converts text and stores it in *target. Returns NULL on success, or
         * a short reason on failure, which ends up in the error message:
         * "invalid value '10Q' for '--size': <reason>". */
        const char *(*parse)(const char *text, void *target);

        /* Optional: writes the current value of *target as text, so help can
         * show the default. Return false to show none. NULL for no default. */
        bool (*format)(const void *target, char *buf, size_t size);
    } argh_type;

    /* Initializer for a list backed by a fixed array:
     *     const char *buf[8];
     *     argh_values includes = ARGH_VALUES(buf); */
#define ARGH_VALUES(array) {(array), 0, (int)(sizeof(array) / sizeof((array)[0]))}

    typedef struct argh_error
    {
        argh_err code;
        int argv_index;      /* argv position of the problem, -1 if none */
        const argh_opt *opt; /* option involved, if any */
        const char *value;   /* offending text, if any */
        char short_name;     /* offending short option, if any */
        const char *detail;  /* extra context for ARGH_E_CONFIG */
        const char *suggestion; /* closest valid name for an unknown option
                                   or command (without dashes), or NULL */
    } argh_error;

    struct argh_parser;

    /* A command: `tool <name> ...`. Build tables of them with ARGH_CMD,
     * ARGH_CMD_GROUP and ARGH_CMD_END. */
    typedef struct argh_cmd
    {
        const char *name;
        const char *help;
        const argh_opt *opts;         /* options and positionals, NULL if none */
        const struct argh_cmd *subs;  /* subcommands, NULL for a leaf */
        int (*run)(struct argh_parser *p, void *user); /* handler, NULL if none */
    } argh_cmd;

    /* Output sink for help, version and error messages. */
    typedef void (*argh_write_fn)(void *ctx, int to_stderr, const char *text, size_t len);

    /* Lives on the stack. Fields are internal: use the functions below. */
    typedef struct argh_parser
    {
        const char *argh__name;
        const char *argh__about;
        const char *argh__version;
        unsigned argh__flags;
        int argh__status;
        const char *argh__config_problem;

        const argh_opt *argh__tables[ARGH_MAX_TABLES];
        int argh__table_count;

        argh_opt argh__builder[ARGH_BUILDER_CAP + 1];
        int argh__builder_count;

        unsigned char argh__seen[(ARGH_MAX_OPTS + 7) / 8];

        argh_write_fn argh__write;
        void *argh__write_ctx;

#ifndef ARGH_NO_COMMANDS
        const argh_cmd *argh__commands;
        const argh_cmd *argh__path[ARGH_MAX_DEPTH];
        int argh__depth;
#endif

        argh_error argh__error;
    } argh_parser;

    /* ============================================================================
     * Setup
     * ============================================================================ */

    /* name: program name for help and errors, NULL to take it from argv[0].
     * about: one-line description for help, can be NULL. */
    void argh_init(argh_parser *p, const char *name, const char *about);

    /* Enables -V/--version, printing "<name> <version>". */
    void argh_version(argh_parser *p, const char *version);

    /* ARGH_POSIX, ARGH_NO_AUTO_HELP */
    void argh_set_flags(argh_parser *p, unsigned flags);

    /* Redirects all output. The default writes to stdout and stderr. */
    void argh_set_writer(argh_parser *p, argh_write_fn write, void *ctx);

    /* Adds an option table ending with ARGH_END. Tables and builder calls can
     * be mixed; options appear in help in the order they were added. */
    void argh_table(argh_parser *p, const argh_opt *table);

#ifndef ARGH_NO_COMMANDS
    /* Sets the top-level commands, a table ending with ARGH_CMD_END. Options
     * added with argh_table() and builder calls become global options that
     * work before and after the command name. */
    void argh_commands(argh_parser *p, const argh_cmd *commands);
#endif

    /* ============================================================================
     * Builder: add options one call at a time
     *
     * Each call returns the new option, or NULL if ARGH_BUILDER_CAP is exceeded
     * (argh_parse then fails with ARGH_E_CONFIG). Modifiers accept NULL.
     * ============================================================================ */

    argh_opt *argh_flag(argh_parser *p, char short_name, const char *long_name, bool *target, const char *help);
    argh_opt *argh_count(argh_parser *p, char short_name, const char *long_name, int *target, const char *help);
    argh_opt *argh_int(argh_parser *p, char short_name, const char *long_name, int *target, const char *help);
    argh_opt *argh_long(argh_parser *p, char short_name, const char *long_name, long *target, const char *help);
    argh_opt *argh_double(argh_parser *p, char short_name, const char *long_name, double *target, const char *help);
    argh_opt *argh_string(argh_parser *p, char short_name, const char *long_name, const char **target, const char *help);
    argh_opt *argh_enum(argh_parser *p, char short_name, const char *long_name, int *target,
                        const char *const *choices, const char *help);
    argh_opt *argh_list(argh_parser *p, char short_name, const char *long_name, argh_values *target, const char *help);
    argh_opt *argh_pos(argh_parser *p, const char *name, const char **target, const char *help);
    argh_opt *argh_rest(argh_parser *p, const char *name, argh_values *target, const char *help);
    argh_opt *argh_custom(argh_parser *p, char short_name, const char *long_name, void *target,
                          const argh_type *type, const char *help);
    argh_opt *argh_group(argh_parser *p, const char *title);

    argh_opt *argh_required(argh_opt *opt);
    argh_opt *argh_optional(argh_opt *opt);
    argh_opt *argh_hidden(argh_opt *opt);
    argh_opt *argh_negatable(argh_opt *opt);
    argh_opt *argh_once(argh_opt *opt);
    argh_opt *argh_metavar(argh_opt *opt, const char *metavar);

    /* ============================================================================
     * Parsing and results
     * ============================================================================ */

    /* Returns true when the program should continue. Returns false after
     * printing help, the version, or an error; then return argh_exit_code().
     * argv is reordered in place: positionals end up first, in order. */
    bool argh_parse(argh_parser *p, int argc, char **argv);

    /* 0 after --help/--version or success, 2 after a usage error. */
    int argh_exit_code(const argh_parser *p);

    /* True if the option bound to target appeared on the command line. */
    bool argh_given(const argh_parser *p, const void *target);

#ifndef ARGH_NO_COMMANDS
    /* The command that was selected (the deepest one), NULL without commands. */
    const argh_cmd *argh_command(const argh_parser *p);

    /* Calls the selected command's handler and returns its result, or 0 if the
     * command has no handler. */
    int argh_run(argh_parser *p, void *user);
#endif

    /* The error from the last argh_parse(), code ARGH_E_NONE if none. */
    const argh_error *argh_last_error(const argh_parser *p);

    /* Formats the last error as one line without a trailing newline.
     * Returns the full length, like snprintf; output is truncated to fit. */
    size_t argh_format_error(const argh_parser *p, char *buf, size_t size);

    void argh_print_help(const argh_parser *p);

    /* ============================================================================
     * Table macros
     *
     *   static const argh_opt opts[] = {
     *       ARGH_FLAG('v', "verbose", &verbose, "Verbose output"),
     *       ARGH_STRING('o', "output", &out, "Output file", ARGH_REQUIRED),
     *       ARGH_END
     *   };
     *
     * The last argument (flags) is optional. The compiler warns if the
     * variable has the wrong type for the option.
     * ============================================================================ */

/* The traditional MSVC preprocessor passes __VA_ARGS__ on as one token.
 * An extra expansion pass splits it into separate arguments. */
#define ARGH__EXPAND(x) x
#define ARGH__FIRST(...) ARGH__EXPAND(ARGH__FIRST_(__VA_ARGS__, ~))
#define ARGH__FIRST_(a, ...) a
#define ARGH__SECOND(...) ARGH__EXPAND(ARGH__SECOND_(__VA_ARGS__, 0, ~))
#define ARGH__SECOND_(a, b, ...) b

/* Constant-expression type check: both ?: branches must be compatible */
#define ARGH__TARGET(type, ptr) ((void *)(1 ? (ptr) : (type *)0))

#define ARGH__OPT(s, l, kind, type, target, extra, ...)                                    \
    {                                                                                      \
        (char)(s), (l), (unsigned char)(kind), (unsigned char)(ARGH__SECOND(__VA_ARGS__)), \
            ARGH__TARGET(type, target), (const void *)(extra), ARGH__FIRST(__VA_ARGS__), NULL \
    }

#define ARGH_FLAG(s, l, target, ...) ARGH__OPT(s, l, ARGH_K_FLAG, bool, target, NULL, __VA_ARGS__)
#define ARGH_COUNT(s, l, target, ...) ARGH__OPT(s, l, ARGH_K_COUNT, int, target, NULL, __VA_ARGS__)
#define ARGH_INT(s, l, target, ...) ARGH__OPT(s, l, ARGH_K_INT, int, target, NULL, __VA_ARGS__)
#define ARGH_LONG(s, l, target, ...) ARGH__OPT(s, l, ARGH_K_LONG, long, target, NULL, __VA_ARGS__)
#define ARGH_DOUBLE(s, l, target, ...) ARGH__OPT(s, l, ARGH_K_DOUBLE, double, target, NULL, __VA_ARGS__)
#define ARGH_STRING(s, l, target, ...) ARGH__OPT(s, l, ARGH_K_STRING, const char *, target, NULL, __VA_ARGS__)
#define ARGH_ENUM(s, l, target, choices, ...) \
    ARGH__OPT(s, l, ARGH_K_ENUM, int, target, choices, __VA_ARGS__)
#define ARGH_LIST(s, l, target, ...) ARGH__OPT(s, l, ARGH_K_LIST, argh_values, target, NULL, __VA_ARGS__)
#define ARGH_POS(name, target, ...) ARGH__OPT(0, name, ARGH_K_POS, const char *, target, NULL, __VA_ARGS__)
#define ARGH_REST(name, target, ...) ARGH__OPT(0, name, ARGH_K_REST, argh_values, target, NULL, __VA_ARGS__)
/* No type check here: the argh_type decides what target points to */
#define ARGH_CUSTOM(s, l, target, type, ...)                                                        \
    {                                                                                               \
        (char)(s), (l), (unsigned char)ARGH_K_CUSTOM, (unsigned char)(ARGH__SECOND(__VA_ARGS__)),   \
            (void *)(target), (const void *)(type), ARGH__FIRST(__VA_ARGS__), NULL                  \
    }
#define ARGH_GROUP(title) {0, NULL, (unsigned char)ARGH_K_GROUP, 0, NULL, NULL, (title), NULL}
#define ARGH_END {0, NULL, (unsigned char)ARGH_K_END, 0, NULL, NULL, NULL, NULL}

    /* ============================================================================
     * Command macros
     *
     *   static const argh_cmd commands[] = {
     *       ARGH_CMD("build", "Build the project", build_opts, run_build),
     *       ARGH_CMD("clean", "Remove build output", NULL),
     *       ARGH_CMD_GROUP("remote", "Manage remotes", remote_commands),
     *       ARGH_CMD_END
     *   };
     *
     * ARGH_CMD(name, help, options[, handler]): the handler is optional.
     * ARGH_CMD_GROUP(name, help, subcommands): a command that only holds
     * other commands.
     * ============================================================================ */

#ifndef ARGH_NO_COMMANDS
#define ARGH_CMD(name, help, ...) \
    {(name), (help), ARGH__FIRST(__VA_ARGS__), NULL, ARGH__SECOND(__VA_ARGS__)}
#define ARGH_CMD_GROUP(name, help, subs) {(name), (help), NULL, (subs), NULL}
#define ARGH_CMD_END {NULL, NULL, NULL, NULL, NULL}
#endif

#ifdef __cplusplus
}
#endif

#endif /* ARGH_H_INCLUDED */

/* ============================================================================
 * Implementation
 * ============================================================================ */

#ifdef ARGH_IMPLEMENTATION
#ifndef ARGH_IMPLEMENTATION_DONE
#define ARGH_IMPLEMENTATION_DONE

#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef __cplusplus
extern "C"
{
#endif

    enum
    {
        ARGH__S_READY = 0,
        ARGH__S_OK,
        ARGH__S_HELP,
        ARGH__S_VERSION,
        ARGH__S_ERROR
    };

/* Widest left column in help before descriptions move to the next line */
#define ARGH__HELP_COLUMN_MAX 30

/* Iterates all entries on the active path: the parser's tables, then the
 * options of each selected command. idx is a stable index for the "seen"
 * bitset. The _T form takes the slot counter name so loops can be nested. */
#define ARGH__EACH_T(p, t, o, idx)                                                     \
    for (int t = 0, idx = 0; t < argh__slot_count(p); t++)                             \
        for (const argh_opt *o = argh__slot(p, t); o->kind != ARGH_K_END; o++, idx++)
#define ARGH__EACH(p, o, idx) ARGH__EACH_T(p, argh__t, o, idx)

    static const argh_opt argh__empty_table[1] = {ARGH_END};

/* Command state. With ARGH_NO_COMMANDS these are constants, and the compiler
 * drops every branch that deals with commands. */
#ifdef ARGH_NO_COMMANDS
#define ARGH__DEPTH(p) 0
#define ARGH__LEAF(p) ((const argh_cmd *)NULL)
#else
#define ARGH__DEPTH(p) ((p)->argh__depth)
#define ARGH__LEAF(p) ((p)->argh__depth ? (p)->argh__path[(p)->argh__depth - 1] : (const argh_cmd *)NULL)
#endif

    /* Slots: tables first, then one per command on the path */
    static int argh__slot_count(const argh_parser *p)
    {
        return p->argh__table_count + ARGH__DEPTH(p);
    }

    static const argh_opt *argh__slot(const argh_parser *p, int slot)
    {
        const argh_opt *t = NULL;
        if (slot < p->argh__table_count)
            return p->argh__tables[slot];
#ifndef ARGH_NO_COMMANDS
        t = p->argh__path[slot - p->argh__table_count]->opts;
#endif
        return t ? t : argh__empty_table;
    }

    /* Subcommands expected at the current level, NULL if none */
    static const argh_cmd *argh__level_commands(const argh_parser *p)
    {
#ifdef ARGH_NO_COMMANDS
        (void)p;
        return NULL;
#else
        return p->argh__depth ? p->argh__path[p->argh__depth - 1]->subs : p->argh__commands;
#endif
    }

#ifndef ARGH_NO_COMMANDS
    static const argh_cmd *argh__find_command(const argh_cmd *cmds, const char *name)
    {
        for (; cmds && cmds->name; cmds++)
            if (strcmp(cmds->name, name) == 0)
                return cmds;
        return NULL;
    }
#endif

    /* ------------------------------------------------------------------------
     * Output helpers
     * ------------------------------------------------------------------------ */

    static void argh__stdio_write(void *ctx, int to_stderr, const char *text, size_t len)
    {
        (void)ctx;
        fwrite(text, 1, len, to_stderr ? stderr : stdout);
    }

    static void argh__outn(const argh_parser *p, int err, const char *s, size_t n)
    {
        if (s && n)
            p->argh__write(p->argh__write_ctx, err, s, n);
    }

    static void argh__out(const argh_parser *p, int err, const char *s)
    {
        if (s)
            argh__outn(p, err, s, strlen(s));
    }

    static void argh__spaces(const argh_parser *p, int n)
    {
        static const char spaces[] = "                                ";
        while (n > 0)
        {
            int k = n < 32 ? n : 32;
            argh__outn(p, 0, spaces, (size_t)k);
            n -= k;
        }
    }

    /* Bounded string builder: tracks the full length like snprintf */
    typedef struct
    {
        char *buf;
        size_t size;
        size_t len;
    } argh__sb;

    static void argh__sb_putn(argh__sb *b, const char *s, size_t n)
    {
        size_t i;
        for (i = 0; i < n && s[i]; i++)
        {
            if (b->len + 1 < b->size)
                b->buf[b->len] = s[i];
            b->len++;
        }
        if (b->size)
            b->buf[b->len < b->size ? b->len : b->size - 1] = '\0';
    }

    static void argh__sb_put(argh__sb *b, const char *s)
    {
        if (s)
            argh__sb_putn(b, s, strlen(s));
    }

    static void argh__sb_char(argh__sb *b, char c)
    {
        argh__sb_putn(b, &c, 1);
    }

    /* ------------------------------------------------------------------------
     * Option lookup
     * ------------------------------------------------------------------------ */

    static bool argh__is_option_kind(int kind)
    {
        return kind != ARGH_K_POS && kind != ARGH_K_REST && kind != ARGH_K_GROUP;
    }

    static bool argh__takes_value(const argh_opt *o)
    {
        switch (o->kind)
        {
        case ARGH_K_INT:
        case ARGH_K_LONG:
        case ARGH_K_DOUBLE:
        case ARGH_K_STRING:
        case ARGH_K_ENUM:
        case ARGH_K_LIST:
        case ARGH_K_CUSTOM:
            return true;
        default:
            return false;
        }
    }

    static const argh_opt *argh__find_long(const argh_parser *p, const char *name, size_t len, int *index)
    {
        ARGH__EACH(p, o, i)
        {
            /* The first-character check skips most strncmp calls */
            if (argh__is_option_kind(o->kind) && o->long_name && o->long_name[0] == name[0] &&
                strncmp(o->long_name, name, len) == 0 && o->long_name[len] == '\0')
            {
                *index = i;
                return o;
            }
        }
        return NULL;
    }

    static const argh_opt *argh__find_short(const argh_parser *p, char c, int *index)
    {
        ARGH__EACH(p, o, i)
        {
            if (argh__is_option_kind(o->kind) && o->short_name == c)
            {
                *index = i;
                return o;
            }
        }
        return NULL;
    }

    /* Indices past ARGH_MAX_OPTS are ignored here and reported as a
     * configuration error after the scan */
    static bool argh__seen(const argh_parser *p, int index)
    {
        if (index >= ARGH_MAX_OPTS)
            return false;
        return (p->argh__seen[index >> 3] >> (index & 7)) & 1u;
    }

    static void argh__mark(argh_parser *p, int index)
    {
        if (index < ARGH_MAX_OPTS)
            p->argh__seen[index >> 3] = (unsigned char)(p->argh__seen[index >> 3] | (1u << (index & 7)));
    }

    /* ------------------------------------------------------------------------
     * Value conversion: strict, whole string, no surrounding spaces
     * ------------------------------------------------------------------------ */

    /* Decimal or 0x hex with an optional sign. Octal is never used. */
    static bool argh__int_syntax(const char *s, int *base)
    {
        if (*s == '+' || *s == '-')
            s++;
        if (s[0] == '0' && (s[1] == 'x' || s[1] == 'X'))
        {
            s += 2;
            *base = 16;
            if (!*s)
                return false;
            for (; *s; s++)
                if (!isxdigit((unsigned char)*s))
                    return false;
            return true;
        }
        *base = 10;
        if (!*s)
            return false;
        for (; *s; s++)
            if (!isdigit((unsigned char)*s))
                return false;
        return true;
    }

    static argh_err argh__parse_long(const char *s, long lo, long hi, long *out)
    {
        int base;
        char *end;
        long v;
        if (!argh__int_syntax(s, &base))
            return ARGH_E_INVALID_VALUE;
        errno = 0;
        v = strtol(s, &end, base);
        if (errno == ERANGE || v < lo || v > hi)
            return ARGH_E_OUT_OF_RANGE;
        *out = v;
        return ARGH_E_NONE;
    }

    static argh_err argh__parse_double(const char *s, double *out)
    {
        char *end;
        double v;
        /* Rejects leading spaces, "inf" and "nan", which strtod accepts */
        if (!(*s == '+' || *s == '-' || *s == '.' || isdigit((unsigned char)*s)))
            return ARGH_E_INVALID_VALUE;
        v = strtod(s, &end);
        if (end == s || *end != '\0')
            return ARGH_E_INVALID_VALUE;
        if (!isfinite(v))
            return ARGH_E_OUT_OF_RANGE;
        *out = v;
        return ARGH_E_NONE;
    }

    static bool argh__ieq(const char *a, const char *b)
    {
        for (; *a && *b; a++, b++)
            if (tolower((unsigned char)*a) != tolower((unsigned char)*b))
                return false;
        return *a == *b;
    }

    static argh_err argh__parse_bool(const char *s, bool *out)
    {
        if (argh__ieq(s, "true") || argh__ieq(s, "yes") || argh__ieq(s, "on") || strcmp(s, "1") == 0)
            *out = true;
        else if (argh__ieq(s, "false") || argh__ieq(s, "no") || argh__ieq(s, "off") || strcmp(s, "0") == 0)
            *out = false;
        else
            return ARGH_E_INVALID_VALUE;
        return ARGH_E_NONE;
    }

    /* Converts and stores one value. v is NULL for flags and counters. */
    /* reason: set to the argh_type's explanation when a custom parse fails */
    static argh_err argh__store(const argh_opt *o, const char *v, bool negated, const char **reason)
    {
        long l;
        double d;
        bool b;
        argh_err e;

        switch (o->kind)
        {
        case ARGH_K_FLAG:
            if (!v)
            {
                *(bool *)o->target = !negated;
                return ARGH_E_NONE;
            }
            if ((e = argh__parse_bool(v, &b)) != ARGH_E_NONE)
                return e;
            *(bool *)o->target = b;
            return ARGH_E_NONE;
        case ARGH_K_COUNT:
            if (*(int *)o->target < INT_MAX)
                (*(int *)o->target)++;
            return ARGH_E_NONE;
        case ARGH_K_INT:
            if ((e = argh__parse_long(v, INT_MIN, INT_MAX, &l)) != ARGH_E_NONE)
                return e;
            *(int *)o->target = (int)l;
            return ARGH_E_NONE;
        case ARGH_K_LONG:
            if ((e = argh__parse_long(v, LONG_MIN, LONG_MAX, &l)) != ARGH_E_NONE)
                return e;
            *(long *)o->target = l;
            return ARGH_E_NONE;
        case ARGH_K_DOUBLE:
            if ((e = argh__parse_double(v, &d)) != ARGH_E_NONE)
                return e;
            *(double *)o->target = d;
            return ARGH_E_NONE;
        case ARGH_K_STRING:
            *(const char **)o->target = v;
            return ARGH_E_NONE;
        case ARGH_K_ENUM:
        {
            const char *const *choices = (const char *const *)o->extra;
            int i;
            for (i = 0; choices && choices[i]; i++)
            {
                if (strcmp(choices[i], v) == 0)
                {
                    *(int *)o->target = i;
                    return ARGH_E_NONE;
                }
            }
            return ARGH_E_INVALID_VALUE;
        }
        case ARGH_K_LIST:
        {
            argh_values *list = (argh_values *)o->target;
            if (list->count >= list->capacity)
                return ARGH_E_TOO_MANY_VALUES;
            list->items[list->count++] = v;
            return ARGH_E_NONE;
        }
        case ARGH_K_CUSTOM:
            *reason = ((const argh_type *)o->extra)->parse(v, o->target);
            return *reason ? ARGH_E_INVALID_VALUE : ARGH_E_NONE;
        default:
            return ARGH_E_NONE;
        }
    }

    /* ------------------------------------------------------------------------
     * Parsing
     * ------------------------------------------------------------------------ */

    static int argh__fail(argh_parser *p, argh_err code, int index, const argh_opt *o,
                          const char *value, char short_name)
    {
        argh_error *e = &p->argh__error;
        e->code = code;
        e->argv_index = index;
        e->opt = o;
        e->value = value;
        e->short_name = short_name;
        return ARGH__S_ERROR;
    }

    static bool argh__auto_help(const argh_parser *p)
    {
        return !(p->argh__flags & ARGH_NO_AUTO_HELP);
    }

    static int argh__config_error(argh_parser *p, const char *detail, const argh_opt *o)
    {
        p->argh__error.detail = detail;
        return argh__fail(p, ARGH_E_CONFIG, -1, o, NULL, 0);
    }

    static int argh__apply(argh_parser *p, const argh_opt *o, int index, const char *v,
                           bool negated, int argi, char short_name)
    {
        argh_err e;
        const char *reason = NULL;
        if ((o->flags & ARGH_ONCE) && argh__seen(p, index))
            return argh__fail(p, ARGH_E_REPEATED, argi, o, NULL, short_name);
        e = argh__store(o, v, negated, &reason);
        if (e != ARGH_E_NONE)
        {
            p->argh__error.detail = reason;
            return argh__fail(p, e, argi, o, v, short_name);
        }
        argh__mark(p, index);
        return ARGH__S_OK;
    }

    /* Moves argv[i] to argv[*w], shifting the options in between right.
     * Keeps argv a permutation, with positionals in their original order. */
    static void argh__move_positional(char **argv, int *w, int i)
    {
        char *arg = argv[i];
        int k;
        for (k = i; k > *w; k--)
            argv[k] = argv[k - 1];
        argv[*w] = arg;
        (*w)++;
    }

    static bool argh__next_is_value(int i, int argc, char **argv)
    {
        return i + 1 < argc && argv[i + 1] && strcmp(argv[i + 1], "--") != 0;
    }

    /* One pass over argv. With apply == false nothing is written: the pass
     * only finds out whether help or version was requested, so that help can
     * show the defaults before any option changes them. */
#ifndef ARGH_NO_COMMANDS
    /* `tool help remote add`: selects the named commands, then asks for help */
    static int argh__help_command(argh_parser *p, int argc, char **argv, int i)
    {
        for (; i < argc && argv[i]; i++)
        {
            const argh_cmd *level = argh__level_commands(p);
            const argh_cmd *c = argh__find_command(level, argv[i]);
            if (!level)
                return argh__fail(p, ARGH_E_UNEXPECTED_ARGUMENT, i, NULL, argv[i], 0);
            if (!c)
                return argh__fail(p, ARGH_E_UNKNOWN_COMMAND, i, NULL, argv[i], 0);
            if (p->argh__depth >= ARGH_MAX_DEPTH)
                return argh__config_error(p, "commands nested deeper than ARGH_MAX_DEPTH", NULL);
            p->argh__path[p->argh__depth++] = c;
        }
        return ARGH__S_HELP;
    }
#endif

    static int argh__scan(argh_parser *p, int argc, char **argv, bool apply, int *positional_count)
    {
        int w = 1;
        bool only_positionals = false;
        int i;

#ifndef ARGH_NO_COMMANDS
        p->argh__depth = 0;
#endif
        for (i = 1; i < argc && argv[i]; i++)
        {
            char *arg = argv[i];
            int rc = ARGH__S_OK;

            if (only_positionals || arg[0] != '-' || arg[1] == '\0')
            {
#ifndef ARGH_NO_COMMANDS
                const argh_cmd *level = only_positionals ? NULL : argh__level_commands(p);
                if (level)
                {
                    /* At a level with subcommands, a positional names one */
                    const argh_cmd *c = argh__find_command(level, arg);
                    if (!c && argh__auto_help(p) && strcmp(arg, "help") == 0)
                        return argh__help_command(p, argc, argv, i + 1);
                    if (!c)
                        rc = argh__fail(p, ARGH_E_UNKNOWN_COMMAND, i, NULL, arg, 0);
                    else if (p->argh__depth >= ARGH_MAX_DEPTH)
                        rc = argh__config_error(p, "commands nested deeper than ARGH_MAX_DEPTH", NULL);
                    else
                        p->argh__path[p->argh__depth++] = c;
                    if (rc != ARGH__S_OK && apply)
                        return rc;
                    continue;
                }
#endif
                if (apply)
                    argh__move_positional(argv, &w, i);
                if (p->argh__flags & ARGH_POSIX)
                    only_positionals = true;
                continue;
            }

            if (arg[1] == '-' && arg[2] == '\0')
            {
                only_positionals = true;
                continue;
            }

            if (arg[1] == '-')
            {
                /* --name, --name=value, --no-name */
                const char *name = arg + 2;
                const char *eq = strchr(name, '=');
                size_t len = eq ? (size_t)(eq - name) : strlen(name);
                const argh_opt *o;
                const char *v = NULL;
                bool negated = false;
                int index = 0;

                if (argh__auto_help(p))
                {
                    if (len == 4 && strncmp(name, "help", 4) == 0)
                        return ARGH__S_HELP;
                    if (p->argh__version && len == 7 && strncmp(name, "version", 7) == 0)
                        return ARGH__S_VERSION;
                }

                o = argh__find_long(p, name, len, &index);
                if (!o && len > 3 && strncmp(name, "no-", 3) == 0)
                {
                    o = argh__find_long(p, name + 3, len - 3, &index);
                    if (o && o->kind == ARGH_K_FLAG && (o->flags & ARGH_NEGATABLE))
                        negated = true;
                    else
                        o = NULL;
                }
                if (!o)
                    rc = argh__fail(p, ARGH_E_UNKNOWN_OPTION, i, NULL, arg, 0);
                else if (argh__takes_value(o))
                {
                    if (eq)
                        v = eq + 1;
                    else if (argh__next_is_value(i, argc, argv))
                        v = argv[++i];
                    else
                        rc = argh__fail(p, ARGH_E_MISSING_VALUE, i, o, NULL, 0);
                }
                else if (eq && (o->kind != ARGH_K_FLAG || negated))
                    rc = argh__fail(p, ARGH_E_UNEXPECTED_VALUE, i, o, eq + 1, 0);
                else if (eq)
                    v = eq + 1;

                if (rc == ARGH__S_OK && apply)
                    rc = argh__apply(p, o, index, v, negated, i, 0);
            }
            else
            {
                /* -v, -abc, -ofile, -o file */
                const char *c;
                int argi = i;
                for (c = arg + 1; *c && rc == ARGH__S_OK; c++)
                {
                    const argh_opt *o;
                    int index = 0;

                    if (*c == '=')
                    {
                        rc = argh__fail(p, ARGH_E_SHORT_EQUALS, argi, NULL, arg, c[-1]);
                        break;
                    }
                    if (argh__auto_help(p))
                    {
                        if (*c == 'h')
                            return ARGH__S_HELP;
                        if (*c == 'V' && p->argh__version)
                            return ARGH__S_VERSION;
                    }

                    o = argh__find_short(p, *c, &index);
                    if (!o)
                    {
                        rc = argh__fail(p, ARGH_E_UNKNOWN_OPTION, argi, NULL, arg, *c);
                        break;
                    }
                    if (argh__takes_value(o))
                    {
                        const char *v = NULL;
                        if (c[1] == '=')
                            rc = argh__fail(p, ARGH_E_SHORT_EQUALS, argi, o, arg, *c);
                        else if (c[1])
                            v = c + 1;
                        else if (argh__next_is_value(i, argc, argv))
                            v = argv[++i];
                        else
                            rc = argh__fail(p, ARGH_E_MISSING_VALUE, argi, o, NULL, *c);
                        if (rc == ARGH__S_OK && apply)
                            rc = argh__apply(p, o, index, v, false, argi, *c);
                        break; /* the rest of the cluster was the value */
                    }
                    if (apply)
                        rc = argh__apply(p, o, index, NULL, false, argi, *c);
                }
            }

            /* The dry pass ignores errors: it only looks for help/version */
            if (rc != ARGH__S_OK && apply)
                return rc;
        }

        *positional_count = w - 1;
        return ARGH__S_OK;
    }

    /* Hands positionals to ARGH_K_POS / ARGH_K_REST, checks required ones */
    static int argh__assign_positionals(argh_parser *p, char **argv, int count)
    {
        int used = 0;
        ARGH__EACH(p, o, index)
        {
            if (o->kind == ARGH_K_POS)
            {
                if (used < count)
                {
                    *(const char **)o->target = argv[1 + used++];
                    argh__mark(p, index);
                }
                else if (!(o->flags & ARGH_OPTIONAL))
                {
                    return argh__fail(p, ARGH_E_MISSING_REQUIRED, -1, o, NULL, 0);
                }
            }
            else if (o->kind == ARGH_K_REST)
            {
                argh_values *rest = (argh_values *)o->target;
                rest->items = (const char **)(void *)(argv + 1 + used);
                rest->count = count - used;
                rest->capacity = rest->count;
                used = count;
                if (rest->count > 0)
                    argh__mark(p, index);
                else if (o->flags & ARGH_REQUIRED)
                    return argh__fail(p, ARGH_E_MISSING_REQUIRED, -1, o, NULL, 0);
            }
        }
        if (used < count)
            return argh__fail(p, ARGH_E_UNEXPECTED_ARGUMENT, 1 + used, NULL, argv[1 + used], 0);
        return ARGH__S_OK;
    }

    static int argh__check_required(argh_parser *p)
    {
        ARGH__EACH(p, o, index)
        {
            if (argh__is_option_kind(o->kind) && (o->flags & ARGH_REQUIRED) && !argh__seen(p, index))
                return argh__fail(p, ARGH_E_MISSING_REQUIRED, -1, o, NULL, 0);
        }
        return ARGH__S_OK;
    }

#ifndef ARGH_NO_COMMANDS
    static bool argh__has_positionals(const argh_opt *o)
    {
        for (; o && o->kind != ARGH_K_END; o++)
            if (o->kind == ARGH_K_POS || o->kind == ARGH_K_REST)
                return true;
        return false;
    }
#endif

#if !defined(NDEBUG) && !defined(ARGH_NO_COMMANDS)
    /* Walks the command tree: names, reserved "help", positionals next to
     * subcommands. Debug builds only, like the duplicate check. */
    static int argh__check_commands(argh_parser *p, const argh_cmd *cmds, int depth)
    {
        const argh_cmd *a, *b;
        if (depth > ARGH_MAX_DEPTH)
            return argh__config_error(p, "commands nested deeper than ARGH_MAX_DEPTH", NULL);
        for (a = cmds; a->name; a++)
        {
            int st;
            if (argh__auto_help(p) && strcmp(a->name, "help") == 0)
                return argh__config_error(p, "command name 'help' is reserved, see ARGH_NO_AUTO_HELP", NULL);
            for (b = a + 1; b->name; b++)
                if (strcmp(a->name, b->name) == 0)
                    return argh__config_error(p, "command defined twice", NULL);
            if (a->subs && argh__has_positionals(a->opts))
                return argh__config_error(p, "a command with subcommands cannot have positional arguments", NULL);
            if (a->subs && (st = argh__check_commands(p, a->subs, depth + 1)) != ARGH__S_OK)
                return st;
        }
        return ARGH__S_OK;
    }
#endif

#ifndef ARGH_NO_SUGGEST
    /* Longest name considered for suggestions */
#define ARGH__SUGGEST_MAX 64

    /* Optimal string alignment distance: edits (insert, delete, replace)
     * plus swaps of adjacent characters, so "verbsoe" is 1 from "verbose".
     * Three rolling rows on the stack; returns a large value past the cap. */
    static int argh__distance(const char *a, size_t alen, const char *b, size_t blen)
    {
        unsigned char rows[3][ARGH__SUGGEST_MAX + 1];
        unsigned char *prev2 = rows[0], *prev = rows[1], *cur = rows[2];
        size_t i, j;

        if (alen > ARGH__SUGGEST_MAX || blen > ARGH__SUGGEST_MAX)
            return 1000;
        for (j = 0; j <= blen; j++)
            prev[j] = (unsigned char)j;
        for (i = 1; i <= alen; i++)
        {
            unsigned char *t;
            cur[0] = (unsigned char)i;
            for (j = 1; j <= blen; j++)
            {
                int cost = a[i - 1] != b[j - 1];
                int best = prev[j] + 1;              /* delete */
                if (cur[j - 1] + 1 < best)
                    best = cur[j - 1] + 1;           /* insert */
                if (prev[j - 1] + cost < best)
                    best = prev[j - 1] + cost;       /* replace */
                if (i > 1 && j > 1 && a[i - 1] == b[j - 2] && a[i - 2] == b[j - 1] &&
                    prev2[j - 2] + 1 < best)
                    best = prev2[j - 2] + 1;         /* swap */
                cur[j] = (unsigned char)best;
            }
            t = prev2;
            prev2 = prev;
            prev = cur;
            cur = t;
        }
        return prev[blen];
    }

    typedef struct
    {
        const char *typed;
        size_t len;
        const char *best;
        int best_distance;
    } argh__suggester;

    /* Close enough: at most 2 edits, and at most a third of the longer word,
     * so short inputs don't get far-fetched matches ("ad" -> "add" yes,
     * "jb" -> "jobs" no) */
    static void argh__consider(argh__suggester *s, const char *candidate)
    {
        int d;
        size_t clen, longer;
        if (!candidate)
            return;
        clen = strlen(candidate);
        longer = clen > s->len ? clen : s->len;
        d = argh__distance(s->typed, s->len, candidate, clen);
        if (d <= 2 && (size_t)d * 3 <= longer && d < s->best_distance)
        {
            s->best = candidate;
            s->best_distance = d;
        }
    }

    /* Runs only after a failed parse, so it costs nothing on success */
    static void argh__suggest(argh_parser *p)
    {
        argh_error *e = &p->argh__error;
        argh__suggester s;
        s.best = NULL;
        s.best_distance = 1000;

        if (e->code == ARGH_E_UNKNOWN_OPTION && !e->short_name && e->value)
        {
            const char *eq;
            s.typed = e->value + 2; /* skip "--" */
            eq = strchr(s.typed, '=');
            s.len = eq ? (size_t)(eq - s.typed) : strlen(s.typed);
            ARGH__EACH(p, o, index)
            {
                (void)index;
                if (argh__is_option_kind(o->kind) && !(o->flags & ARGH_HIDDEN))
                    argh__consider(&s, o->long_name);
            }
            if (argh__auto_help(p))
            {
                argh__consider(&s, "help");
                if (p->argh__version)
                    argh__consider(&s, "version");
            }
        }
        else if (e->code == ARGH_E_UNKNOWN_COMMAND && e->value)
        {
            const argh_cmd *c;
            s.typed = e->value;
            s.len = strlen(s.typed);
            for (c = argh__level_commands(p); c && c->name; c++)
                argh__consider(&s, c->name);
            if (argh__auto_help(p))
                argh__consider(&s, "help");
        }
        e->suggestion = s.best;
    }
#endif

    /* Duplicate names on the active path: quadratic, so debug builds only */
    static int argh__check_duplicates(argh_parser *p)
    {
#ifndef NDEBUG
        ARGH__EACH_T(p, argh__ta, a, ia)
        {
            if (!argh__is_option_kind(a->kind))
                continue;
            ARGH__EACH_T(p, argh__tb, b, ib)
            {
                if (ib <= ia || !argh__is_option_kind(b->kind))
                    continue;
                if ((a->short_name && a->short_name == b->short_name) ||
                    (a->long_name && b->long_name && a->long_name[0] == b->long_name[0] &&
                     strcmp(a->long_name, b->long_name) == 0))
                    return argh__config_error(p, "option defined twice", b);
            }
        }
#else
        (void)p;
#endif
        return ARGH__S_OK;
    }

    /* Catches mistakes in the definitions before any argument is read */
    static int argh__check_config(argh_parser *p)
    {
        int total = 0;

        if (p->argh__config_problem)
            return argh__config_error(p, p->argh__config_problem, NULL);

        ARGH__EACH(p, o, index)
        {
            (void)index;
            total++;
            if (argh__auto_help(p) && argh__is_option_kind(o->kind))
            {
                bool clash = o->short_name == 'h' || (o->long_name && strcmp(o->long_name, "help") == 0);
                if (p->argh__version)
                    clash = clash || o->short_name == 'V' ||
                            (o->long_name && strcmp(o->long_name, "version") == 0);
                if (clash)
                    return argh__config_error(p, "name reserved for help/version, see ARGH_NO_AUTO_HELP", o);
            }
            if (!o->target && o->kind != ARGH_K_GROUP)
                return argh__config_error(p, "option has no target variable", o);
            if (o->kind == ARGH_K_CUSTOM && (!o->extra || !((const argh_type *)o->extra)->parse))
                return argh__config_error(p, "custom option has no argh_type with a parse function", o);
            if (o->kind == ARGH_K_ENUM && !o->extra)
                return argh__config_error(p, "enum option has no choices", o);
        }
        if (total > ARGH_MAX_OPTS)
            return argh__config_error(p, "more options than ARGH_MAX_OPTS", NULL);

#ifndef ARGH_NO_COMMANDS
        if (p->argh__commands)
        {
            for (int t = 0; t < p->argh__table_count; t++)
                if (argh__has_positionals(p->argh__tables[t]))
                    return argh__config_error(p, "positional arguments cannot be combined with commands", NULL);
#ifndef NDEBUG
            {
                int st = argh__check_commands(p, p->argh__commands, 1);
                if (st != ARGH__S_OK)
                    return st;
            }
#endif
        }
#endif
        return ARGH__S_OK;
    }

    /* Checks that need the selected command path */
    static int argh__check_path(argh_parser *p)
    {
        int total = 0;
        ARGH__EACH(p, o, index)
        {
            (void)o;
            total = index + 1;
        }
        if (total > ARGH_MAX_OPTS)
            return argh__config_error(p, "more options than ARGH_MAX_OPTS on this command path", NULL);
        if (argh__level_commands(p))
            return argh__fail(p, ARGH_E_MISSING_COMMAND, -1, NULL, NULL, 0);
        return argh__check_duplicates(p);
    }

    /* Cheap check whether a dry pass for help/version is worth running */
    static bool argh__may_want_help(const argh_parser *p, int argc, char **argv)
    {
        int i;
        if (!argh__auto_help(p))
            return false;
        for (i = 1; i < argc && argv[i]; i++)
        {
            const char *a = argv[i];
            if (a[0] != '-')
                continue;
            if (a[1] == '-')
            {
                if (a[2] == '\0')
                    return false;
                if (strncmp(a + 2, "help", 4) == 0 || strncmp(a + 2, "version", 7) == 0)
                    return true;
            }
            else if (strchr(a, 'h') || strchr(a, 'V'))
            {
                return true;
            }
        }
        return false;
    }

    /* "tool remote add": program name plus the selected commands */
    static void argh__out_path(const argh_parser *p, int err)
    {
        int d;
        argh__out(p, err, p->argh__name);
        for (d = 0; d < ARGH__DEPTH(p); d++)
        {
#ifndef ARGH_NO_COMMANDS
            argh__out(p, err, " ");
            argh__out(p, err, p->argh__path[d]->name);
#endif
        }
    }

#ifndef ARGH_NO_COMMANDS
    static void argh__print_commands(const argh_parser *p, int err, const argh_cmd *cmds)
    {
        const argh_cmd *c;
        int width = 0;
        for (c = cmds; c->name; c++)
            if ((int)strlen(c->name) > width)
                width = (int)strlen(c->name);
        argh__out(p, err, "Commands:\n");
        for (c = cmds; c->name; c++)
        {
            int pad = width - (int)strlen(c->name) + 2;
            argh__out(p, err, "  ");
            argh__out(p, err, c->name);
            while (pad-- > 0)
                argh__out(p, err, " ");
            argh__out(p, err, c->help);
            argh__out(p, err, "\n");
        }
    }
#endif

    static void argh__print_error(const argh_parser *p)
    {
        char buf[256];
        size_t n = argh_format_error(p, buf, sizeof(buf));
        argh__out(p, 1, p->argh__name);
        argh__out(p, 1, ": ");
        argh__outn(p, 1, buf, n < sizeof(buf) ? n : sizeof(buf) - 1);
        argh__out(p, 1, "\n");
#ifndef ARGH_NO_COMMANDS
        if (p->argh__error.code == ARGH_E_MISSING_COMMAND)
        {
            argh__out(p, 1, "\n");
            argh__print_commands(p, 1, argh__level_commands(p));
            argh__out(p, 1, "\n");
        }
#endif
        if (argh__auto_help(p) && p->argh__error.code != ARGH_E_CONFIG)
        {
            argh__out(p, 1, "Try '");
            argh__out_path(p, 1);
            argh__out(p, 1, " --help' for more information.\n");
        }
    }

    static const char *argh__basename(const char *path)
    {
        const char *base = path;
        for (; *path; path++)
            if (*path == '/' || *path == '\\')
                base = path + 1;
        return base;
    }

    /* ------------------------------------------------------------------------
     * Public API: setup and builder
     * ------------------------------------------------------------------------ */

    void argh_init(argh_parser *p, const char *name, const char *about)
    {
        memset(p, 0, sizeof(*p));
        p->argh__name = name;
        p->argh__about = about;
        p->argh__write = argh__stdio_write;
    }

    void argh_version(argh_parser *p, const char *version)
    {
        p->argh__version = version;
    }

    void argh_set_flags(argh_parser *p, unsigned flags)
    {
        p->argh__flags = flags;
    }

    void argh_set_writer(argh_parser *p, argh_write_fn write, void *ctx)
    {
        p->argh__write = write ? write : argh__stdio_write;
        p->argh__write_ctx = ctx;
    }

    void argh_table(argh_parser *p, const argh_opt *table)
    {
        if (p->argh__table_count >= ARGH_MAX_TABLES)
        {
            p->argh__config_problem = "more tables than ARGH_MAX_TABLES";
            return;
        }
        p->argh__tables[p->argh__table_count++] = table;
    }

#ifndef ARGH_NO_COMMANDS
    void argh_commands(argh_parser *p, const argh_cmd *commands)
    {
        p->argh__commands = commands;
    }
#endif

    static argh_opt *argh__add(argh_parser *p, char short_name, const char *long_name, int kind,
                               void *target, const void *extra, const char *help)
    {
        argh_opt *o;
        if (p->argh__builder_count >= ARGH_BUILDER_CAP)
        {
            p->argh__config_problem = "more builder options than ARGH_BUILDER_CAP";
            return NULL;
        }
        /* The builder joins the table list at its first use, so help order
         * follows the order of argh_table() and builder calls */
        if (p->argh__builder_count == 0)
        {
            argh_table(p, p->argh__builder);
            if (p->argh__config_problem)
                return NULL;
        }
        o = &p->argh__builder[p->argh__builder_count++];
        o->short_name = short_name;
        o->long_name = long_name;
        o->kind = (unsigned char)kind;
        o->flags = 0;
        o->target = target;
        o->extra = extra;
        o->help = help;
        o->metavar = NULL;
        return o;
    }

    argh_opt *argh_flag(argh_parser *p, char s, const char *l, bool *target, const char *help)
    {
        return argh__add(p, s, l, ARGH_K_FLAG, target, NULL, help);
    }

    argh_opt *argh_count(argh_parser *p, char s, const char *l, int *target, const char *help)
    {
        return argh__add(p, s, l, ARGH_K_COUNT, target, NULL, help);
    }

    argh_opt *argh_int(argh_parser *p, char s, const char *l, int *target, const char *help)
    {
        return argh__add(p, s, l, ARGH_K_INT, target, NULL, help);
    }

    argh_opt *argh_long(argh_parser *p, char s, const char *l, long *target, const char *help)
    {
        return argh__add(p, s, l, ARGH_K_LONG, target, NULL, help);
    }

    argh_opt *argh_double(argh_parser *p, char s, const char *l, double *target, const char *help)
    {
        return argh__add(p, s, l, ARGH_K_DOUBLE, target, NULL, help);
    }

    argh_opt *argh_string(argh_parser *p, char s, const char *l, const char **target, const char *help)
    {
        return argh__add(p, s, l, ARGH_K_STRING, (void *)target, NULL, help);
    }

    argh_opt *argh_enum(argh_parser *p, char s, const char *l, int *target,
                        const char *const *choices, const char *help)
    {
        return argh__add(p, s, l, ARGH_K_ENUM, target, choices, help);
    }

    argh_opt *argh_list(argh_parser *p, char s, const char *l, argh_values *target, const char *help)
    {
        return argh__add(p, s, l, ARGH_K_LIST, target, NULL, help);
    }

    argh_opt *argh_pos(argh_parser *p, const char *name, const char **target, const char *help)
    {
        return argh__add(p, 0, name, ARGH_K_POS, (void *)target, NULL, help);
    }

    argh_opt *argh_rest(argh_parser *p, const char *name, argh_values *target, const char *help)
    {
        return argh__add(p, 0, name, ARGH_K_REST, target, NULL, help);
    }

    argh_opt *argh_custom(argh_parser *p, char s, const char *l, void *target, const argh_type *type,
                          const char *help)
    {
        return argh__add(p, s, l, ARGH_K_CUSTOM, target, type, help);
    }

    argh_opt *argh_group(argh_parser *p, const char *title)
    {
        return argh__add(p, 0, NULL, ARGH_K_GROUP, NULL, NULL, title);
    }

    static argh_opt *argh__set_flag(argh_opt *opt, int flag)
    {
        if (opt)
            opt->flags = (unsigned char)(opt->flags | flag);
        return opt;
    }

    argh_opt *argh_required(argh_opt *opt) { return argh__set_flag(opt, ARGH_REQUIRED); }
    argh_opt *argh_optional(argh_opt *opt) { return argh__set_flag(opt, ARGH_OPTIONAL); }
    argh_opt *argh_hidden(argh_opt *opt) { return argh__set_flag(opt, ARGH_HIDDEN); }
    argh_opt *argh_negatable(argh_opt *opt) { return argh__set_flag(opt, ARGH_NEGATABLE); }
    argh_opt *argh_once(argh_opt *opt) { return argh__set_flag(opt, ARGH_ONCE); }

    argh_opt *argh_metavar(argh_opt *opt, const char *metavar)
    {
        if (opt)
            opt->metavar = metavar;
        return opt;
    }

    /* ------------------------------------------------------------------------
     * Public API: parsing and results
     * ------------------------------------------------------------------------ */

    bool argh_parse(argh_parser *p, int argc, char **argv)
    {
        int positional_count = 0;
        int st;

        memset(p->argh__seen, 0, sizeof(p->argh__seen));
        memset(&p->argh__error, 0, sizeof(p->argh__error));
        p->argh__error.argv_index = -1;
        if (!p->argh__name)
            p->argh__name = (argc > 0 && argv[0]) ? argh__basename(argv[0]) : "program";

#ifndef ARGH_NO_COMMANDS
        p->argh__depth = 0;
#endif
        st = argh__check_config(p);
        if (st == ARGH__S_OK && argh__may_want_help(p, argc, argv))
        {
            st = argh__scan(p, argc, argv, false, &positional_count);
            if (st == ARGH__S_OK)
            {
                /* The dry pass records errors it then ignores */
                memset(&p->argh__error, 0, sizeof(p->argh__error));
                p->argh__error.argv_index = -1;
            }
        }
        if (st == ARGH__S_OK)
            st = argh__scan(p, argc, argv, true, &positional_count);
        if (st == ARGH__S_OK)
            st = argh__check_path(p);
        if (st == ARGH__S_OK)
            st = argh__assign_positionals(p, argv, positional_count);
        if (st == ARGH__S_OK)
            st = argh__check_required(p);

#ifndef ARGH_NO_SUGGEST
        if (st == ARGH__S_ERROR)
            argh__suggest(p);
#endif
        p->argh__status = st;
        switch (st)
        {
        case ARGH__S_OK:
            return true;
        case ARGH__S_HELP:
            argh_print_help(p);
            return false;
        case ARGH__S_VERSION:
            argh__out(p, 0, p->argh__name);
            argh__out(p, 0, " ");
            argh__out(p, 0, p->argh__version);
            argh__out(p, 0, "\n");
            return false;
        default:
            argh__print_error(p);
            return false;
        }
    }

    int argh_exit_code(const argh_parser *p)
    {
        return p->argh__status == ARGH__S_ERROR ? 2 : 0;
    }

    bool argh_given(const argh_parser *p, const void *target)
    {
        ARGH__EACH(p, o, index)
        {
            if (o->target == target && o->kind != ARGH_K_GROUP)
                return argh__seen(p, index);
        }
        return false;
    }

    const argh_error *argh_last_error(const argh_parser *p)
    {
        return &p->argh__error;
    }

#ifndef ARGH_NO_COMMANDS
    const argh_cmd *argh_command(const argh_parser *p)
    {
        return ARGH__LEAF(p);
    }

    int argh_run(argh_parser *p, void *user)
    {
        const argh_cmd *c = argh_command(p);
        return (c && c->run) ? c->run(p, user) : 0;
    }
#endif

    /* "--name" if the option has a long name, "-x" otherwise, "<name>" for
     * positionals. used_short: the short name the user typed, if any. */
    static void argh__sb_opt_name(argh__sb *b, const argh_opt *o, char used_short)
    {
        if (o && (o->kind == ARGH_K_POS || o->kind == ARGH_K_REST))
        {
            argh__sb_char(b, '<');
            argh__sb_put(b, o->long_name);
            argh__sb_char(b, '>');
        }
        else if (used_short)
        {
            argh__sb_char(b, '-');
            argh__sb_char(b, used_short);
        }
        else if (o && o->long_name)
        {
            argh__sb_put(b, "--");
            argh__sb_put(b, o->long_name);
        }
        else if (o)
        {
            argh__sb_char(b, '-');
            argh__sb_char(b, o->short_name);
        }
    }

    static void argh__sb_expected(argh__sb *b, const argh_opt *o)
    {
        switch (o->kind)
        {
        case ARGH_K_INT:
        case ARGH_K_LONG:
            argh__sb_put(b, "expected an integer");
            break;
        case ARGH_K_DOUBLE:
            argh__sb_put(b, "expected a number");
            break;
        case ARGH_K_FLAG:
            argh__sb_put(b, "expected true or false");
            break;
        case ARGH_K_ENUM:
        {
            const char *const *choices = (const char *const *)o->extra;
            int i;
            argh__sb_put(b, "expected one of: ");
            for (i = 0; choices && choices[i]; i++)
            {
                if (i)
                    argh__sb_put(b, ", ");
                argh__sb_put(b, choices[i]);
            }
            break;
        }
        default:
            argh__sb_put(b, "invalid value");
            break;
        }
    }

    size_t argh_format_error(const argh_parser *p, char *buf, size_t size)
    {
        const argh_error *e = &p->argh__error;
        argh__sb b;
        b.buf = buf;
        b.size = size;
        b.len = 0;
        if (size)
            buf[0] = '\0';

        switch (e->code)
        {
        case ARGH_E_NONE:
            break;
        case ARGH_E_UNKNOWN_OPTION:
            argh__sb_put(&b, "unknown option '");
            if (e->short_name)
            {
                argh__sb_char(&b, '-');
                argh__sb_char(&b, e->short_name);
            }
            else
            {
                const char *eq = strchr(e->value, '=');
                argh__sb_putn(&b, e->value, eq ? (size_t)(eq - e->value) : strlen(e->value));
            }
            argh__sb_char(&b, '\'');
            if (e->suggestion)
            {
                argh__sb_put(&b, " (did you mean '--");
                argh__sb_put(&b, e->suggestion);
                argh__sb_put(&b, "'?)");
            }
            break;
        case ARGH_E_MISSING_VALUE:
            argh__sb_put(&b, "option '");
            argh__sb_opt_name(&b, e->opt, e->short_name);
            argh__sb_put(&b, "' requires a value");
            break;
        case ARGH_E_INVALID_VALUE:
            argh__sb_put(&b, "invalid value '");
            argh__sb_put(&b, e->value);
            argh__sb_put(&b, "' for '");
            argh__sb_opt_name(&b, e->opt, e->short_name);
            argh__sb_put(&b, "': ");
            if (e->opt->kind == ARGH_K_CUSTOM && e->detail)
                argh__sb_put(&b, e->detail);
            else
                argh__sb_expected(&b, e->opt);
            break;
        case ARGH_E_OUT_OF_RANGE:
            argh__sb_put(&b, "value '");
            argh__sb_put(&b, e->value);
            argh__sb_put(&b, "' for '");
            argh__sb_opt_name(&b, e->opt, e->short_name);
            argh__sb_put(&b, "' is out of range");
            break;
        case ARGH_E_UNEXPECTED_VALUE:
            argh__sb_put(&b, "option '");
            argh__sb_opt_name(&b, e->opt, e->short_name);
            argh__sb_put(&b, "' does not take a value");
            break;
        case ARGH_E_SHORT_EQUALS:
            argh__sb_put(&b, "short option '-");
            argh__sb_char(&b, e->short_name);
            argh__sb_put(&b, "' does not accept '=': write '-");
            argh__sb_char(&b, e->short_name);
            argh__sb_put(&b, " VALUE'");
            if (e->opt && e->opt->long_name)
            {
                argh__sb_put(&b, " or '--");
                argh__sb_put(&b, e->opt->long_name);
                argh__sb_put(&b, "=VALUE'");
            }
            break;
        case ARGH_E_UNEXPECTED_ARGUMENT:
            argh__sb_put(&b, "unexpected argument '");
            argh__sb_put(&b, e->value);
            argh__sb_char(&b, '\'');
            break;
        case ARGH_E_MISSING_REQUIRED:
            argh__sb_put(&b, e->opt && (e->opt->kind == ARGH_K_POS || e->opt->kind == ARGH_K_REST)
                                 ? "missing required argument '"
                                 : "missing required option '");
            argh__sb_opt_name(&b, e->opt, 0);
            argh__sb_char(&b, '\'');
            break;
        case ARGH_E_REPEATED:
            argh__sb_put(&b, "option '");
            argh__sb_opt_name(&b, e->opt, e->short_name);
            argh__sb_put(&b, "' can only be given once");
            break;
        case ARGH_E_TOO_MANY_VALUES:
        {
            char num[24];
            snprintf(num, sizeof(num), "%d", ((const argh_values *)e->opt->target)->capacity);
            argh__sb_put(&b, "too many values for '");
            argh__sb_opt_name(&b, e->opt, e->short_name);
            argh__sb_put(&b, "' (at most ");
            argh__sb_put(&b, num);
            argh__sb_char(&b, ')');
            break;
        }
        case ARGH_E_UNKNOWN_COMMAND:
            argh__sb_put(&b, "unknown command '");
            argh__sb_put(&b, e->value);
            argh__sb_char(&b, '\'');
            if (e->suggestion)
            {
                argh__sb_put(&b, " (did you mean '");
                argh__sb_put(&b, e->suggestion);
                argh__sb_put(&b, "'?)");
            }
            break;
        case ARGH_E_MISSING_COMMAND:
            if (ARGH__LEAF(p))
            {
                argh__sb_put(&b, "'");
                argh__sb_put(&b, ARGH__LEAF(p)->name);
                argh__sb_put(&b, "' needs a command");
            }
            else
            {
                argh__sb_put(&b, "missing command");
            }
            break;
        case ARGH_E_CONFIG:
            argh__sb_put(&b, "configuration error: ");
            argh__sb_put(&b, e->detail);
            if (e->opt)
            {
                argh__sb_put(&b, " (");
                argh__sb_opt_name(&b, e->opt, 0);
                argh__sb_char(&b, ')');
            }
            break;
        }
        return b.len;
    }

    /* ------------------------------------------------------------------------
     * Help
     * ------------------------------------------------------------------------ */

    static void argh__sb_metavar(argh__sb *b, const argh_opt *o)
    {
        if (o->metavar)
        {
            argh__sb_put(b, o->metavar);
            return;
        }
        switch (o->kind)
        {
        case ARGH_K_INT:
        case ARGH_K_LONG:
            argh__sb_put(b, "<n>");
            break;
        case ARGH_K_DOUBLE:
            argh__sb_put(b, "<x>");
            break;
        case ARGH_K_CUSTOM:
        {
            const argh_type *t = (const argh_type *)o->extra;
            argh__sb_put(b, (t && t->metavar) ? t->metavar : "<value>");
            break;
        }
        case ARGH_K_ENUM:
        {
            const char *const *choices = (const char *const *)o->extra;
            int i;
            argh__sb_char(b, '<');
            for (i = 0; choices && choices[i]; i++)
            {
                if (i)
                    argh__sb_char(b, '|');
                argh__sb_put(b, choices[i]);
            }
            argh__sb_char(b, '>');
            break;
        }
        default:
            argh__sb_put(b, "<value>");
            break;
        }
    }

    /* Left help column for one entry, without indentation */
    static size_t argh__left_column(const argh_opt *o, char *buf, size_t size)
    {
        argh__sb b;
        b.buf = buf;
        b.size = size;
        b.len = 0;
        buf[0] = '\0';

        if (o->kind == ARGH_K_POS)
        {
            argh__sb_char(&b, (o->flags & ARGH_OPTIONAL) ? '[' : '<');
            argh__sb_put(&b, o->long_name);
            argh__sb_char(&b, (o->flags & ARGH_OPTIONAL) ? ']' : '>');
            return b.len;
        }
        if (o->kind == ARGH_K_REST)
        {
            argh__sb_char(&b, (o->flags & ARGH_REQUIRED) ? '<' : '[');
            argh__sb_put(&b, o->long_name);
            argh__sb_put(&b, (o->flags & ARGH_REQUIRED) ? ">..." : "...]");
            return b.len;
        }

        if (o->short_name)
        {
            argh__sb_char(&b, '-');
            argh__sb_char(&b, o->short_name);
            if (o->long_name)
                argh__sb_put(&b, ", ");
        }
        else
        {
            argh__sb_put(&b, "    ");
        }
        if (o->long_name)
        {
            argh__sb_put(&b, (o->kind == ARGH_K_FLAG && (o->flags & ARGH_NEGATABLE)) ? "--[no-]" : "--");
            argh__sb_put(&b, o->long_name);
        }
        if (argh__takes_value(o))
        {
            argh__sb_char(&b, ' ');
            argh__sb_metavar(&b, o);
        }
        return b.len;
    }

    /* " (default: ...)" from the variable's current value */
    static void argh__help_default(const argh_parser *p, const argh_opt *o)
    {
        char num[64];
        const char *text = NULL;

        if (o->flags & ARGH_REQUIRED)
        {
            argh__out(p, 0, " (required)");
            return;
        }
        switch (o->kind)
        {
        case ARGH_K_INT:
            snprintf(num, sizeof(num), "%d", *(const int *)o->target);
            text = num;
            break;
        case ARGH_K_LONG:
            snprintf(num, sizeof(num), "%ld", *(const long *)o->target);
            text = num;
            break;
        case ARGH_K_DOUBLE:
            snprintf(num, sizeof(num), "%g", *(const double *)o->target);
            text = num;
            break;
        case ARGH_K_STRING:
            text = *(const char *const *)o->target;
            break;
        case ARGH_K_CUSTOM:
        {
            const argh_type *t = (const argh_type *)o->extra;
            num[0] = '\0';
            if (t->format && t->format(o->target, num, sizeof(num)) && num[0])
            {
                num[sizeof(num) - 1] = '\0';
                text = num;
            }
            break;
        }
        case ARGH_K_ENUM:
        {
            const char *const *choices = (const char *const *)o->extra;
            int v = *(const int *)o->target;
            int i;
            for (i = 0; choices[i]; i++)
                if (i == v)
                    text = choices[i];
            break;
        }
        default:
            break;
        }
        if (text)
        {
            argh__out(p, 0, " (default: ");
            argh__out(p, 0, text);
            argh__out(p, 0, ")");
        }
    }

    static void argh__help_line(const argh_parser *p, const char *left, size_t left_len,
                                const char *help, int column, const argh_opt *o)
    {
        argh__spaces(p, 2);
        argh__outn(p, 0, left, left_len);
        if ((int)left_len > column)
        {
            argh__out(p, 0, "\n");
            argh__spaces(p, 2 + column + 2);
        }
        else
        {
            argh__spaces(p, column - (int)left_len + 2);
        }
        argh__out(p, 0, help);
        if (o)
            argh__help_default(p, o);
        argh__out(p, 0, "\n");
    }

    void argh_print_help(const argh_parser *p)
    {
        char left[128];
        int column = 0;
        bool has_positionals = false;
        bool has_globals = false;
        bool in_section = false;
        bool auto_help = argh__auto_help(p);
        const argh_cmd *cmds = argh__level_commands(p);
        const argh_cmd *c;
        /* Slots from own_slot on belong to the selected command (or to the
         * program without commands); earlier slots hold global options */
        int own_slot = ARGH__DEPTH(p) ? argh__slot_count(p) - 1 : 0;

        /* Column width: widest visible entry, capped */
        ARGH__EACH_T(p, slot, o, index)
        {
            size_t len;
            (void)index;
            if (o->kind == ARGH_K_GROUP || (o->flags & ARGH_HIDDEN))
                continue;
            if (slot < own_slot && !argh__is_option_kind(o->kind))
                continue;
            if (slot < own_slot)
                has_globals = true;
            if (o->kind == ARGH_K_POS || o->kind == ARGH_K_REST)
                has_positionals = true;
            len = argh__left_column(o, left, sizeof(left));
            if ((int)len > column && len <= ARGH__HELP_COLUMN_MAX)
                column = (int)len;
        }
        for (c = cmds; c && c->name; c++)
            if ((int)strlen(c->name) > column && strlen(c->name) <= ARGH__HELP_COLUMN_MAX)
                column = (int)strlen(c->name);
        if (auto_help && column < 13)
            column = 13; /* "-V, --version" */

        argh__out(p, 0, "Usage: ");
        argh__out_path(p, 0);
        argh__out(p, 0, " [OPTIONS]");
        if (cmds)
            argh__out(p, 0, " <command>");
        ARGH__EACH_T(p, slot, o, index)
        {
            (void)index;
            if (slot >= own_slot && (o->kind == ARGH_K_POS || o->kind == ARGH_K_REST))
            {
                size_t len = argh__left_column(o, left, sizeof(left));
                argh__out(p, 0, " ");
                argh__outn(p, 0, left, len);
            }
        }
        argh__out(p, 0, "\n");

        {
            const char *about = ARGH__LEAF(p) ? ARGH__LEAF(p)->help : p->argh__about;
            if (about)
            {
                argh__out(p, 0, "\n");
                argh__out(p, 0, about);
                argh__out(p, 0, "\n");
            }
        }

        if (has_positionals)
        {
            argh__out(p, 0, "\nArguments:\n");
            ARGH__EACH_T(p, slot, o, index)
            {
                (void)index;
                if (slot >= own_slot && (o->kind == ARGH_K_POS || o->kind == ARGH_K_REST) &&
                    !(o->flags & ARGH_HIDDEN))
                {
                    size_t len = argh__left_column(o, left, sizeof(left));
                    argh__help_line(p, left, len, o->help, column, NULL);
                }
            }
        }

        if (cmds)
        {
            argh__out(p, 0, "\nCommands:\n");
            for (c = cmds; c->name; c++)
                argh__help_line(p, c->name, strlen(c->name), c->help, column, NULL);
        }

        ARGH__EACH_T(p, slot, o, index)
        {
            size_t len;
            (void)index;
            if (slot < own_slot)
                continue;
            if (o->kind == ARGH_K_GROUP)
            {
                argh__out(p, 0, "\n");
                argh__out(p, 0, o->help);
                argh__out(p, 0, ":\n");
                in_section = true;
                continue;
            }
            if (!argh__is_option_kind(o->kind) || (o->flags & ARGH_HIDDEN))
                continue;
            if (!in_section)
            {
                argh__out(p, 0, "\nOptions:\n");
                in_section = true;
            }
            len = argh__left_column(o, left, sizeof(left));
            argh__help_line(p, left, len, o->help, column, o);
        }

        /* Options of the program and of parent commands, without group headings */
        if (has_globals)
        {
            argh__out(p, 0, "\nGlobal options:\n");
            ARGH__EACH_T(p, slot, o, index)
            {
                size_t len;
                (void)index;
                if (slot >= own_slot || !argh__is_option_kind(o->kind) || (o->flags & ARGH_HIDDEN))
                    continue;
                len = argh__left_column(o, left, sizeof(left));
                argh__help_line(p, left, len, o->help, column, o);
            }
            in_section = true;
        }

        if (auto_help)
        {
            argh__out(p, 0, in_section ? "\n" : "\nOptions:\n");
            argh__help_line(p, "-h, --help", 10, "Print help", column, NULL);
            if (p->argh__version)
                argh__help_line(p, "-V, --version", 13, "Print version", column, NULL);
        }
    }

#undef ARGH__EACH
#undef ARGH__EACH_T

#ifdef __cplusplus
}
#endif

#endif /* ARGH_IMPLEMENTATION_DONE */
#endif /* ARGH_IMPLEMENTATION */

/*
 * ----------------------------------------------------------------------------
 * LICENSE
 * ----------------------------------------------------------------------------
 * MIT License
 *
 * Copyright (c) 2026 Ilya Brin
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
