/* Code-size probe: minimal program using argh */
#include <stdio.h>

#define ARGH_IMPLEMENTATION
#include "../argh.h"

int main(int argc, char **argv)
{
    argh_Parser p;
    argh_init(&p, argc, argv);
    argh_add(&p, "v", "verbose", ARGH_BOOL, NULL, "Verbose output");
    argh_add(&p, "n", "count", ARGH_INT, "10", "Iterations");
    argh_add(&p, "o", "output", ARGH_STRING, "out.txt", "Output file");
    if (!argh_parse(&p))
    {
        argh_print_error(&p);
        argh_print_help(&p);
        return 1;
    }
    printf("%d %d %s\n", argh_get_bool(&p, "verbose"), argh_get_int(&p, "count"),
           argh_get_string(&p, "output"));
    argh_free(&p);
    return 0;
}
