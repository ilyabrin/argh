/*
 * Prototype of the v0.2 option-table macros.
 *
 * Checks two things before the real implementation relies on them:
 *   1. An optional trailing "flags" argument via C99 variadic macros,
 *      on GCC, Clang, MSVC (traditional and conforming preprocessor) and C++.
 *   2. A compile-time check that the target pointer has the right type,
 *      usable inside a static const initializer.
 */

#ifndef ARGH_PROTO_MACROS_H
#define ARGH_PROTO_MACROS_H

#include <stdbool.h>
#include <stddef.h>

enum
{
    ARGH_K_FLAG = 1,
    ARGH_K_INT,
    ARGH_K_STRING
};

enum
{
    ARGH_REQUIRED = 1 << 0,
    ARGH_HIDDEN = 1 << 1,
    ARGH_NEGATABLE = 1 << 2
};

typedef struct argh_opt
{
    char short_name;
    const char *long_name;
    unsigned char kind;
    unsigned char flags;
    void *target;
    const void *extra;
    const char *help;
    const char *metavar;
} argh_opt;

/* The traditional MSVC preprocessor passes __VA_ARGS__ on as a single token.
 * One extra expansion pass splits it into separate arguments. */
#define ARGH__EXPAND(x) x

/* ARGH__FIRST(help)        -> help
 * ARGH__FIRST(help, flags) -> help */
#define ARGH__FIRST(...) ARGH__EXPAND(ARGH__FIRST_(__VA_ARGS__, ~))
#define ARGH__FIRST_(a, ...) a

/* ARGH__SECOND(help)        -> 0
 * ARGH__SECOND(help, flags) -> flags */
#define ARGH__SECOND(...) ARGH__EXPAND(ARGH__SECOND_(__VA_ARGS__, 0, ~))
#define ARGH__SECOND_(a, b, ...) b

/* Type check that stays a constant expression: both branches of ?: must have
 * compatible pointer types, so a wrong target makes the compiler complain. */
#define ARGH__TARGET(type, ptr) ((void *)(1 ? (ptr) : (type *)0))

#define ARGH__OPT(s, l, kind, type, target, ...)                            \
    {                                                                       \
        (s), (l), (kind), (unsigned char)(ARGH__SECOND(__VA_ARGS__)),       \
            ARGH__TARGET(type, target), NULL, ARGH__FIRST(__VA_ARGS__), NULL \
    }

#define ARGH_FLAG(s, l, target, ...) ARGH__OPT(s, l, ARGH_K_FLAG, bool, target, __VA_ARGS__)
#define ARGH_INT(s, l, target, ...) ARGH__OPT(s, l, ARGH_K_INT, int, target, __VA_ARGS__)
#define ARGH_STRING(s, l, target, ...) ARGH__OPT(s, l, ARGH_K_STRING, const char *, target, __VA_ARGS__)
#define ARGH_END {0, NULL, 0, 0, NULL, NULL, NULL, NULL}

#endif
