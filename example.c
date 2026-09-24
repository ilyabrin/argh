/*
 * Example usage of argh.h
 *
 * Build:  make example   (or: cc -std=c99 -o example example.c)
 * Try:    ./example --help
 *         ./example -vv --jobs 8 --mode safe -I src -I include input.txt a b c
 */

#define ARGH_IMPLEMENTATION
#include "argh.h"

#include <stdio.h>

static const char *const modes[] = {"fast", "safe", "debug", NULL};

int main(int argc, char **argv)
{
    /* Defaults are plain initializers */
    int verbosity = 0;
    int jobs = 4;
    double ratio = 1.0;
    const char *output = "out.txt";
    int mode = 0;
    bool color = true;
    const char *include_buf[8];
    argh_values includes = ARGH_VALUES(include_buf);
    const char *input = NULL;
    argh_values files = {0};

    argh_parser p;
    argh_init(&p, "example", "Shows what argh.h can parse");
    argh_version(&p, "1.0.0");

    argh_count(&p, 'v', "verbose", &verbosity, "More output, repeat for even more");
    argh_int(&p, 'j', "jobs", &jobs, "Parallel jobs");
    argh_double(&p, 'r', "ratio", &ratio, "Processing ratio");
    argh_string(&p, 'o', "output", &output, "Output file");
    argh_enum(&p, 'm', "mode", &mode, modes, "Execution mode");
    argh_list(&p, 'I', "include", &includes, "Add an include directory");

    argh_group(&p, "Display");
    argh_negatable(argh_flag(&p, 0, "color", &color, "Colored output"));

    argh_pos(&p, "input", &input, "Input file");
    argh_rest(&p, "files", &files, "Extra files");

    if (!argh_parse(&p, argc, argv))
        return argh_exit_code(&p);

    printf("verbosity: %d\n", verbosity);
    printf("jobs:      %d%s\n", jobs, argh_given(&p, &jobs) ? "" : " (default)");
    printf("ratio:     %g\n", ratio);
    printf("output:    %s\n", output);
    printf("mode:      %s\n", modes[mode]);
    printf("color:     %s\n", color ? "yes" : "no");
    for (int i = 0; i < includes.count; i++)
        printf("include:   %s\n", includes.items[i]);
    printf("input:     %s\n", input);
    for (int i = 0; i < files.count; i++)
        printf("file:      %s\n", files.items[i]);
    return 0;
}
