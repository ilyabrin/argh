/*
 * Example usage of argh.h
 * Compile: gcc -o example example.c
 */

#define ARGH_IMPLEMENTATION
#include "argh.h"

#include <stdio.h>

int main(int argc, char **argv)
{
    argh_Parser parser;
    argh_init(&parser, argc, argv);

    /* Define options */
    argh_add(&parser,
             "v", "verbose",
             ARGH_BOOL,
             NULL,
             "Enable verbose output");

    argh_add(&parser,
             "o", "output",
             ARGH_STRING,
             "output.txt",
             "Output file path");

    argh_add(&parser,
             "n", "count",
             ARGH_INT,
             "10",
             "Number of iterations");

    argh_add(&parser,
             "r", "ratio",
             ARGH_FLOAT,
             "1.0",
             "Processing ratio (0.0-1.0)");

    argh_add(&parser,
             NULL, "config",
             ARGH_STRING,
             NULL,
             "Configuration file (required)");

    argh_require(&parser, "config");

    argh_add(&parser,
             "h", "help",
             ARGH_BOOL,
             NULL,
             "Show this help message");

    /* Parse */
    if (!argh_parse(&parser))
    {
        argh_print_error(&parser);
        printf("\n");
        argh_print_help(&parser);
        argh_free(&parser);
        return 1;
    }

    /* Check for help */
    if (argh_has(&parser, "help"))
    {
        argh_print_help(&parser);
        argh_free(&parser);
        return 0;
    }

    /* Get values */
    printf("Configuration:\n");
    printf("  Verbose: %s\n", argh_get_bool(&parser, "verbose") ? "yes" : "no");
    printf("  Output: %s\n", argh_get_string(&parser, "output"));
    printf("  Count: %d\n", argh_get_int(&parser, "count"));
    printf("  Ratio: %.2f\n", argh_get_float(&parser, "ratio"));
    printf("  Config: %s\n", argh_get_string(&parser, "config"));

    /* Positional arguments */
    if (parser.positional_count > 0)
    {
        printf("\nPositional arguments (%d):\n", (int)parser.positional_count);
        for (size_t i = 0; i < parser.positional_count; i++)
        {
            printf("  [%d] %s\n", (int)i, parser.positional[i]);
        }
    }

    argh_free(&parser);
    return 0;
}
