/*
 * argh.h - v0.1.0 - Single-header argument parsing library for C
 *
 * Status: early development. The API may change before v1.0.
 *
 * Features:
 *   - Single header, pure C99, no dependencies beyond the C standard library
 *   - Support for short (-v) and long (--verbose) options
 *   - Required and optional arguments
 *   - Positional arguments
 *   - Automatic help generation
 *   - Type-safe value parsing (int, float, string, bool)
 *   - Error handling with descriptive messages
 *
 * USAGE:
 *   #define ARGH_IMPLEMENTATION before including this header
 *
 * EXAMPLE:
 *   #define ARGH_IMPLEMENTATION
 *   #include "argh.h"
 *
 *   int main(int argc, char** argv) {
 *       argh_Parser parser;
 *       argh_init(&parser, argc, argv);
 *
 *       argh_add(&parser, "v", "verbose", ARGH_BOOL, NULL, "Enable verbose output");
 *       argh_add(&parser, "o", "output", ARGH_STRING, "out.txt", "Output file");
 *       argh_add(&parser, "n", "count", ARGH_INT, "10", "Number of iterations");
 *
 *       if (!argh_parse(&parser)) {
 *           argh_print_error(&parser);
 *           argh_print_help(&parser);
 *           return 1;
 *       }
 *
 *       bool verbose = argh_get_bool(&parser, "verbose");
 *       const char* output = argh_get_string(&parser, "output");
 *       int count = argh_get_int(&parser, "count");
 *
 *       // Get positional arguments
 *       for (size_t i = 0; i < parser.positional_count; i++) {
 *           printf("Positional: %s\n", parser.positional[i]);
 *       }
 *
 *       argh_free(&parser);
 *       return 0;
 *   }
 *
 * LICENSE: MIT (see end of file)
 */

#ifndef ARGH_H_INCLUDED
#define ARGH_H_INCLUDED

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

    /* ============================================================================
     * Configuration
     * ============================================================================ */

#ifndef ARGH_MAX_OPTIONS
#define ARGH_MAX_OPTIONS 64
#endif

#ifndef ARGH_MAX_POSITIONAL
#define ARGH_MAX_POSITIONAL 128
#endif

#ifndef ARGH_MAX_ERRORS
#define ARGH_MAX_ERRORS 8
#endif

    /* ============================================================================
     * Types
     * ============================================================================ */

    typedef enum
    {
        ARGH_BOOL,   /* --flag (no value needed) */
        ARGH_STRING, /* --name value */
        ARGH_INT,    /* --name 42 */
        ARGH_FLOAT,  /* --name 3.14 */
        ARGH_DOUBLE  /* --name 3.14159 */
    } argh_Type;

    typedef enum
    {
        ARGH_ERR_NONE = 0,
        ARGH_ERR_UNKNOWN_OPTION,
        ARGH_ERR_MISSING_VALUE,
        ARGH_ERR_INVALID_VALUE,
        ARGH_ERR_DUPLICATE_OPTION,
        ARGH_ERR_REQUIRED_MISSING,
        ARGH_ERR_TOO_MANY_POSITIONAL,
        ARGH_ERR_INTERNAL
    } argh_ErrorCode;

    typedef struct
    {
        argh_ErrorCode code;
        const char *option;
        const char *message;
    } argh_Error;

    typedef struct
    {
        const char *short_name; /* e.g., "v" for -v */
        const char *long_name;  /* e.g., "verbose" for --verbose */
        argh_Type type;
        const char *default_value;
        const char *description;
        bool required;
        bool present;
        const char *value; /* parsed value */
    } argh_Option;

    typedef struct
    {
        const char *program;
        int argc;
        char **argv;

        argh_Option options[ARGH_MAX_OPTIONS];
        size_t option_count;

        const char *positional[ARGH_MAX_POSITIONAL];
        size_t positional_count;

        argh_Error errors[ARGH_MAX_ERRORS];
        size_t error_count;

        bool parsed;
        bool help_requested;

        /* Internal state */
        char **_values; /* allocated values */
        size_t _values_count;
        size_t _values_capacity;
    } argh_Parser;

    /* ============================================================================
     * API
     * ============================================================================ */

    /* Initialize parser */
    void argh_init(argh_Parser *parser, int argc, char **argv);

    /* Free allocated resources */
    void argh_free(argh_Parser *parser);

    /* Add an option definition
     * short_name: can be NULL for long-only options
     * long_name: required
     * type: option type
     * default_value: can be NULL
     * description: for help text
     */
    void argh_add(argh_Parser *parser,
                  const char *short_name,
                  const char *long_name,
                  argh_Type type,
                  const char *default_value,
                  const char *description);

    /* Mark an option as required */
    void argh_require(argh_Parser *parser, const char *long_name);

    /* Parse command line arguments. Returns true on success */
    bool argh_parse(argh_Parser *parser);

    /* Get typed values */
    bool argh_get_bool(argh_Parser *parser, const char *name);
    int argh_get_int(argh_Parser *parser, const char *name);
    float argh_get_float(argh_Parser *parser, const char *name);
    double argh_get_double(argh_Parser *parser, const char *name);
    const char *argh_get_string(argh_Parser *parser, const char *name);

    /* Check if option was provided */
    bool argh_has(argh_Parser *parser, const char *name);

    /* Error handling */
    bool argh_has_errors(argh_Parser *parser);
    void argh_print_error(argh_Parser *parser);
    const char *argh_error_string(argh_ErrorCode code);

    /* Help generation */
    void argh_print_help(argh_Parser *parser);

#ifdef __cplusplus
}
#endif

#endif /* ARGH_H_INCLUDED */

/* ============================================================================
 * Implementation
 * ============================================================================ */

#ifdef ARGH_IMPLEMENTATION

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <float.h>
#include <limits.h>

/* ============================================================================
 * Internal utilities
 * ============================================================================ */

static int argh__strcmp(const char *a, const char *b)
{
    if (!a && !b)
        return 0;
    if (!a)
        return -1;
    if (!b)
        return 1;
    return strcmp(a, b);
}

static char *argh__strdup(const char *s)
{
    if (!s)
        return NULL;
    size_t len = strlen(s) + 1;
    char *dup = (char *)malloc(len);
    if (dup)
        memcpy(dup, s, len);
    return dup;
}

/* ASCII case-insensitive comparison; strcasecmp is POSIX, not C99 */
static bool argh__strieq(const char *a, const char *b)
{
    while (*a && *b)
    {
        if (tolower((unsigned char)*a) != tolower((unsigned char)*b))
            return false;
        a++;
        b++;
    }
    return *a == *b;
}

static bool argh__is_short_option(const char *arg)
{
    return arg && arg[0] == '-' && arg[1] && arg[1] != '-' && !isdigit((unsigned char)arg[1]);
}

static bool argh__is_long(const char *arg)
{
    return arg && arg[0] == '-' && arg[1] == '-' && arg[2];
}

static bool argh__is_option(const char *arg)
{
    return argh__is_short_option(arg) || argh__is_long(arg);
}

/* True if arg can be consumed as the value of the preceding option */
static bool argh__is_value(const char *arg)
{
    return arg && !argh__is_option(arg) && strcmp(arg, "--") != 0;
}

static argh_Option *argh__find_option(argh_Parser *parser, const char *name)
{
    for (size_t i = 0; i < parser->option_count; i++)
    {
        argh_Option *opt = &parser->options[i];
        if (argh__strcmp(opt->short_name, name) == 0 ||
            argh__strcmp(opt->long_name, name) == 0)
        {
            return opt;
        }
    }
    return NULL;
}

static void argh__add_error(argh_Parser *parser, argh_ErrorCode code,
                            const char *option, const char *message)
{
    if (parser->error_count >= ARGH_MAX_ERRORS)
        return;

    argh_Error *err = &parser->errors[parser->error_count++];
    err->code = code;
    err->option = option;
    err->message = message;
}

static bool argh__add_value(argh_Parser *parser, const char *value)
{
    if (parser->_values_count >= parser->_values_capacity)
    {
        size_t new_cap = parser->_values_capacity ? parser->_values_capacity * 2 : 16;
        char **new_vals = (char **)realloc(parser->_values, new_cap * sizeof(char *));
        if (!new_vals)
            return false;
        parser->_values = new_vals;
        parser->_values_capacity = new_cap;
    }

    char *dup = argh__strdup(value);
    if (!dup)
        return false;

    parser->_values[parser->_values_count++] = dup;
    return true;
}

static bool argh__parse_bool(const char *str, bool *out)
{
    if (!str)
    {
        *out = true;
        return true;
    }

    const char *s = str;
    while (isspace((unsigned char)*s))
        s++;

    if (argh__strieq(s, "true") || argh__strieq(s, "1") ||
        argh__strieq(s, "yes") || argh__strieq(s, "on"))
    {
        *out = true;
        return true;
    }
    if (argh__strieq(s, "false") || argh__strieq(s, "0") ||
        argh__strieq(s, "no") || argh__strieq(s, "off"))
    {
        *out = false;
        return true;
    }
    return false;
}

static bool argh__parse_int(const char *str, int *out)
{
    if (!str)
        return false;

    char *endptr;
    errno = 0;
    long val = strtol(str, &endptr, 10);

    if (errno == ERANGE || val < INT_MIN || val > INT_MAX)
        return false;
    while (isspace((unsigned char)*endptr))
        endptr++;
    if (*endptr != '\0')
        return false;

    *out = (int)val;
    return true;
}

static bool argh__parse_float(const char *str, float *out)
{
    if (!str)
        return false;

    char *endptr;
    errno = 0;
    double val = strtod(str, &endptr);

    if (errno == ERANGE || val < -FLT_MAX || val > FLT_MAX)
        return false;
    while (isspace((unsigned char)*endptr))
        endptr++;
    if (*endptr != '\0')
        return false;

    *out = (float)val;
    return true;
}

static bool argh__parse_double(const char *str, double *out)
{
    if (!str)
        return false;

    char *endptr;
    errno = 0;
    double val = strtod(str, &endptr);

    if (errno == ERANGE)
        return false;
    while (isspace((unsigned char)*endptr))
        endptr++;
    if (*endptr != '\0')
        return false;

    *out = val;
    return true;
}

/* ============================================================================
 * Public API implementation
 * ============================================================================ */

void argh_init(argh_Parser *parser, int argc, char **argv)
{
    if (!parser)
        return;

    memset(parser, 0, sizeof(argh_Parser));
    parser->argc = argc;
    parser->argv = argv;
    parser->program = (argc > 0 && argv[0]) ? argv[0] : "program";

    /* Extract just the program name from path */
    const char *p = parser->program;
    const char *last_sep = NULL;
    for (const char *c = p; *c; c++)
    {
        if (*c == '/' || *c == '\\')
            last_sep = c;
    }
    if (last_sep)
        parser->program = last_sep + 1;
}

void argh_free(argh_Parser *parser)
{
    if (!parser)
        return;

    for (size_t i = 0; i < parser->_values_count; i++)
    {
        free(parser->_values[i]);
    }
    free(parser->_values);
    parser->_values = NULL;
    parser->_values_count = 0;
    parser->_values_capacity = 0;
}

void argh_add(argh_Parser *parser,
              const char *short_name,
              const char *long_name,
              argh_Type type,
              const char *default_value,
              const char *description)
{
    if (!parser || (!short_name && !long_name))
        return;
    if (parser->option_count >= ARGH_MAX_OPTIONS)
        return;

    argh_Option *opt = &parser->options[parser->option_count++];
    opt->short_name = short_name;
    opt->long_name = long_name;
    opt->type = type;
    opt->default_value = default_value;
    opt->description = description;
    opt->required = false;
    opt->present = false;
    opt->value = default_value;
}

void argh_require(argh_Parser *parser, const char *long_name)
{
    argh_Option *opt = argh__find_option(parser, long_name);
    if (opt)
        opt->required = true;
}

bool argh_parse(argh_Parser *parser)
{
    if (!parser || parser->parsed)
        return false;
    parser->parsed = true;

    for (int i = 1; i < parser->argc; i++)
    {
        const char *arg = parser->argv[i];

        /* Check for -- (end of options) */
        if (arg && strcmp(arg, "--") == 0)
        {
            /* Rest are positional */
            for (int j = i + 1; j < parser->argc; j++)
            {
                if (parser->positional_count >= ARGH_MAX_POSITIONAL)
                {
                    argh__add_error(parser, ARGH_ERR_TOO_MANY_POSITIONAL,
                                    NULL, "Too many positional arguments");
                    break;
                }
                parser->positional[parser->positional_count++] = parser->argv[j];
            }
            break;
        }

        /* Positional argument */
        if (!argh__is_option(arg))
        {
            if (parser->positional_count >= ARGH_MAX_POSITIONAL)
            {
                argh__add_error(parser, ARGH_ERR_TOO_MANY_POSITIONAL,
                                NULL, "Too many positional arguments");
                continue;
            }
            parser->positional[parser->positional_count++] = arg;
            continue;
        }

        /* Parse option */
        const char *name = NULL;
        const char *value = NULL;
        argh_Option *opt = NULL;

        if (argh__is_long(arg))
        {
            /* Long option: --name or --name=value */
            name = arg + 2;
            const char *eq = strchr(name, '=');
            if (eq)
            {
                /* --name=value form */
                if (!argh__add_value(parser, eq + 1))
                {
                    argh__add_error(parser, ARGH_ERR_INTERNAL, name, "Memory allocation failed");
                    continue;
                }
                value = parser->_values[parser->_values_count - 1];
                /* Temporarily null-terminate at '=' for lookup */
                size_t name_len = eq - name;
                char *temp_name = (char *)malloc(name_len + 1);
                if (!temp_name)
                {
                    argh__add_error(parser, ARGH_ERR_INTERNAL, name, "Memory allocation failed");
                    continue;
                }
                memcpy(temp_name, name, name_len);
                temp_name[name_len] = '\0';
                opt = argh__find_option(parser, temp_name);
                free(temp_name);
            }
            else
            {
                opt = argh__find_option(parser, name);
            }
        }
        else if (argh__is_short_option(arg))
        {
            /* Short option: -v or -abc or -o value */
            name = arg + 1;
            value = NULL; /* Reset value for short options */

            /* Handle combined short options: -abc */
            if (strlen(name) > 1)
            {
                bool all_bool = true;
                bool has_unknown = false;

                /* First pass: check if all are defined bool options */
                for (size_t c = 0; name[c]; c++)
                {
                    char ch[2] = {name[c], '\0'};
                    argh_Option *copt = argh__find_option(parser, ch);
                    if (!copt)
                    {
                        has_unknown = true;
                        break;
                    }
                    if (copt->type != ARGH_BOOL)
                    {
                        all_bool = false;
                        break;
                    }
                }

                if (all_bool && !has_unknown)
                {
                    /* All are bool options - mark them all as present */
                    for (size_t c = 0; name[c]; c++)
                    {
                        char ch[2] = {name[c], '\0'};
                        argh_Option *copt = argh__find_option(parser, ch);
                        copt->present = true;
                        copt->value = "true";
                    }
                    /* Skip further processing - all handled */
                    continue;
                }
                else if (has_unknown)
                {
                    argh__add_error(parser, ARGH_ERR_UNKNOWN_OPTION, name, "Unknown option");
                    continue;
                }
                else
                {
                    /* Mixed bool and non-bool - not supported in combined form */
                    argh__add_error(parser, ARGH_ERR_INVALID_VALUE, name,
                                    "Cannot combine non-bool options");
                    continue;
                }
            }

            opt = argh__find_option(parser, name);
        }

        /* Unknown option */
        if (!opt)
        {
            argh__add_error(parser, ARGH_ERR_UNKNOWN_OPTION, name, "Unknown option");
            continue;
        }

        /* Get value for non-bool options */
        if (opt->type != ARGH_BOOL)
        {
            if (!value)
            {
                /* Try next argument as value */
                if (i + 1 < parser->argc && argh__is_value(parser->argv[i + 1]))
                {
                    i++;
                    value = parser->argv[i];
                }
                else
                {
                    argh__add_error(parser, ARGH_ERR_MISSING_VALUE,
                                    opt->long_name, "Missing required value");
                    continue;
                }
            }

            /* Validate value */
            bool valid = true;
            if (!argh__add_value(parser, value))
            {
                argh__add_error(parser, ARGH_ERR_INTERNAL, opt->long_name, "Memory allocation failed");
                continue;
            }
            opt->value = parser->_values[parser->_values_count - 1];

            switch (opt->type)
            {
            case ARGH_INT:
            {
                int tmp;
                if (!argh__parse_int(opt->value, &tmp))
                    valid = false;
                break;
            }
            case ARGH_FLOAT:
            {
                float tmp;
                if (!argh__parse_float(opt->value, &tmp))
                    valid = false;
                break;
            }
            case ARGH_DOUBLE:
            {
                double tmp;
                if (!argh__parse_double(opt->value, &tmp))
                    valid = false;
                break;
            }
            default:
                break;
            }

            if (!valid)
            {
                argh__add_error(parser, ARGH_ERR_INVALID_VALUE,
                                opt->long_name, "Invalid value format");
            }
        }
        else
        {
            /* Bool option */
            opt->present = true;
            if (!value)
            {
                opt->value = "true";
            }
            else
            {
                bool bval = false;
                if (!argh__parse_bool(value, &bval))
                {
                    argh__add_error(parser, ARGH_ERR_INVALID_VALUE,
                                    opt->long_name, "Invalid boolean value");
                    continue;
                }
                opt->value = bval ? "true" : "false";
            }
        }

        opt->present = true;

        /* Check for help option */
        if (argh__strcmp(opt->long_name, "help") == 0 ||
            argh__strcmp(opt->short_name, "h") == 0)
        {
            parser->help_requested = true;
        }
    }

    /* Check required options */
    for (size_t i = 0; i < parser->option_count; i++)
    {
        argh_Option *opt = &parser->options[i];
        if (opt->required && !opt->present)
        {
            argh__add_error(parser, ARGH_ERR_REQUIRED_MISSING,
                            opt->long_name, "Required option missing");
        }
    }

    return !argh_has_errors(parser);
}

bool argh_get_bool(argh_Parser *parser, const char *name)
{
    argh_Option *opt = argh__find_option(parser, name);
    if (!opt)
        return false;

    bool result;
    if (argh__parse_bool(opt->value, &result))
        return result;
    return opt->present;
}

int argh_get_int(argh_Parser *parser, const char *name)
{
    argh_Option *opt = argh__find_option(parser, name);
    if (!opt || !opt->value)
        return 0;

    int result;
    if (argh__parse_int(opt->value, &result))
        return result;
    return 0;
}

float argh_get_float(argh_Parser *parser, const char *name)
{
    argh_Option *opt = argh__find_option(parser, name);
    if (!opt || !opt->value)
        return 0.0f;

    float result;
    if (argh__parse_float(opt->value, &result))
        return result;
    return 0.0f;
}

double argh_get_double(argh_Parser *parser, const char *name)
{
    argh_Option *opt = argh__find_option(parser, name);
    if (!opt || !opt->value)
        return 0.0;

    double result;
    if (argh__parse_double(opt->value, &result))
        return result;
    return 0.0;
}

const char *argh_get_string(argh_Parser *parser, const char *name)
{
    argh_Option *opt = argh__find_option(parser, name);
    return opt ? opt->value : NULL;
}

bool argh_has(argh_Parser *parser, const char *name)
{
    argh_Option *opt = argh__find_option(parser, name);
    return opt && opt->present;
}

bool argh_has_errors(argh_Parser *parser)
{
    return parser && parser->error_count > 0;
}

const char *argh_error_string(argh_ErrorCode code)
{
    switch (code)
    {
    case ARGH_ERR_NONE:
        return "No error";
    case ARGH_ERR_UNKNOWN_OPTION:
        return "Unknown option";
    case ARGH_ERR_MISSING_VALUE:
        return "Missing value";
    case ARGH_ERR_INVALID_VALUE:
        return "Invalid value";
    case ARGH_ERR_DUPLICATE_OPTION:
        return "Duplicate option";
    case ARGH_ERR_REQUIRED_MISSING:
        return "Required option missing";
    case ARGH_ERR_TOO_MANY_POSITIONAL:
        return "Too many positional arguments";
    case ARGH_ERR_INTERNAL:
        return "Internal error";
    default:
        return "Unknown error";
    }
}

void argh_print_error(argh_Parser *parser)
{
    if (!parser || !parser->error_count)
        return;

    fprintf(stderr, "Error: ");
    for (size_t i = 0; i < parser->error_count; i++)
    {
        argh_Error *err = &parser->errors[i];
        if (err->option)
        {
            fprintf(stderr, "%s: %s", err->option, argh_error_string(err->code));
        }
        else
        {
            fprintf(stderr, "%s", argh_error_string(err->code));
        }
        if (i < parser->error_count - 1)
            fprintf(stderr, "; ");
    }
    fprintf(stderr, "\n");
}

void argh_print_help(argh_Parser *parser)
{
    if (!parser)
        return;

    printf("Usage: %s [OPTIONS] [POSITIONAL...]\n\n", parser->program);

    if (parser->option_count == 0)
    {
        printf("No options defined.\n");
        return;
    }

    printf("Options:\n");

    /* Calculate max width for alignment */
    size_t max_width = 0;
    for (size_t i = 0; i < parser->option_count; i++)
    {
        argh_Option *opt = &parser->options[i];
        size_t width = 2; /* -X */
        if (opt->short_name && opt->long_name)
        {
            width += strlen(opt->short_name) + 2 + strlen(opt->long_name) + 2; /* -x, --long */
        }
        else if (opt->short_name)
        {
            width += strlen(opt->short_name);
        }
        else if (opt->long_name)
        {
            width += 2 + strlen(opt->long_name);
        }
        if (opt->type != ARGH_BOOL)
        {
            width += 6; /* <VAL> */
        }
        if (width > max_width)
            max_width = width;
    }

    /* Print options */
    for (size_t i = 0; i < parser->option_count; i++)
    {
        argh_Option *opt = &parser->options[i];

        /* Build option string */
        char opt_str[128] = "  ";
        char *p = opt_str + 2;
        size_t remaining = sizeof(opt_str) - 2;

        if (opt->short_name)
        {
            int written = snprintf(p, remaining, "-%s", opt->short_name);
            if (written > 0)
            {
                p += written;
                remaining -= (size_t)written;
            }
            if (opt->long_name && remaining > 0)
            {
                written = snprintf(p, remaining, ", ");
                if (written > 0)
                {
                    p += written;
                    remaining -= (size_t)written;
                }
            }
        }
        if (opt->long_name && remaining > 0)
        {
            int written = snprintf(p, remaining, "--%s", opt->long_name);
            if (written > 0)
            {
                p += written;
                remaining -= (size_t)written;
            }
        }
        if (opt->type != ARGH_BOOL && remaining > 0)
        {
            int written = snprintf(p, remaining, " <VAL>");
            if (written > 0)
            {
                p += written;
                remaining -= (size_t)written;
            }
        }

        /* Print with padding */
        printf("%-*s", (int)(max_width + 2), opt_str);

        /* Print description */
        if (opt->description)
        {
            printf("%s", opt->description);
        }

        /* Print default value */
        if (opt->default_value && opt->type != ARGH_BOOL)
        {
            printf(" [default: %s]", opt->default_value);
        }

        /* Print required marker */
        if (opt->required)
        {
            printf(" [required]");
        }

        printf("\n");
    }

    printf("\n");
}

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
