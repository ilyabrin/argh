/* Prototype check: optional flags argument and target type checks. */

#include <stdio.h>
#include <string.h>

#include "macros.h"

static bool verbose;
static int jobs = 4;
static const char *output = "out.txt";

/* static const: must be a constant initializer, i.e. could live in flash */
static const argh_opt opts[] = {
    ARGH_FLAG('v', "verbose", &verbose, "Verbose output"),
    ARGH_INT('j', "jobs", &jobs, "Parallel jobs"),
    ARGH_STRING('o', "output", &output, "Output file", ARGH_REQUIRED),
    ARGH_FLAG(0, "color", &verbose, "Colored output", ARGH_NEGATABLE | ARGH_HIDDEN),
    ARGH_END,
};

static int failures;

#define CHECK(cond)                                              \
    do                                                           \
    {                                                            \
        if (!(cond))                                             \
        {                                                        \
            printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); \
            failures++;                                          \
        }                                                        \
    } while (0)

int main(void)
{
    CHECK(opts[0].flags == 0);
    CHECK(strcmp(opts[0].help, "Verbose output") == 0);
    CHECK(opts[0].target == (void *)&verbose);
    CHECK(opts[0].kind == ARGH_K_FLAG);

    CHECK(opts[1].flags == 0);
    CHECK(opts[1].target == (void *)&jobs);

    CHECK(opts[2].flags == ARGH_REQUIRED);
    CHECK(strcmp(opts[2].help, "Output file") == 0);
    CHECK(opts[2].target == (void *)&output);

    CHECK(opts[3].flags == (ARGH_NEGATABLE | ARGH_HIDDEN));
    CHECK(opts[3].short_name == 0);
    CHECK(strcmp(opts[3].help, "Colored output") == 0);

    CHECK(opts[4].long_name == NULL);

    /* Tables built at runtime from locals must work too */
    int local = 1;
    argh_opt dyn[] = {ARGH_INT('n', "num", &local, "Local", ARGH_REQUIRED), ARGH_END};
    CHECK(dyn[0].target == (void *)&local);
    CHECK(dyn[0].flags == ARGH_REQUIRED);

    printf("%s (%d failures)\n", failures ? "FAILED" : "OK", failures);
    return failures ? 1 : 0;
}
