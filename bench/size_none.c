/* Code-size baseline: same output as the other size_* programs, no parser */
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv)
{
    int verbose = argc > 1;
    int count = argc > 2 ? atoi(argv[2]) : 10;
    const char *output = argc > 3 ? argv[3] : "out.txt";
    printf("%d %d %s\n", verbose, count, output);
    return 0;
}
