/* Code-size probe: minimal program using getopt_long */
#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>

int main(int argc, char **argv)
{
    static const struct option longopts[] = {
        {"verbose", no_argument, 0, 'v'},
        {"count", required_argument, 0, 'n'},
        {"output", required_argument, 0, 'o'},
        {0, 0, 0, 0},
    };
    int verbose = 0, count = 10, c;
    const char *output = "out.txt";
    while ((c = getopt_long(argc, argv, "vn:o:", longopts, NULL)) != -1)
    {
        switch (c)
        {
        case 'v': verbose = 1; break;
        case 'n': count = atoi(optarg); break;
        case 'o': output = optarg; break;
        default:
            fprintf(stderr, "usage: %s [-v] [-n count] [-o output]\n", argv[0]);
            return 1;
        }
    }
    printf("%d %d %s\n", verbose, count, output);
    return 0;
}
