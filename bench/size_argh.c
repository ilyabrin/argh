/* Code-size probe: minimal program using argh */
#include <stdio.h>

#define ARGH_IMPLEMENTATION
#include "../argh.h"

int main(int argc, char **argv)
{
    bool verbose = false;
    int count = 10;
    const char *output = "out.txt";

    argh_parser p;
    argh_init(&p, NULL, NULL);
    argh_flag(&p, 'v', "verbose", &verbose, "Verbose output");
    argh_int(&p, 'n', "count", &count, "Iterations");
    argh_string(&p, 'o', "output", &output, "Output file");
    if (!argh_parse(&p, argc, argv))
        return argh_exit_code(&p);

    printf("%d %d %s\n", verbose, count, output);
    return 0;
}
