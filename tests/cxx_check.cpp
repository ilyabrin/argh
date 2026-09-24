// Compile-only check: argh.h works from C++ (tables, builder, implementation).
#define ARGH_IMPLEMENTATION
#include "../argh.h"

static bool verbose;
static int jobs = 4;
static const char *output = "out.txt";
static const char *const modes[] = {"fast", "safe", nullptr};
static int mode;

static const argh_opt opts[] = {
    ARGH_FLAG('v', "verbose", &verbose, "Verbose output"),
    ARGH_INT('j', "jobs", &jobs, "Parallel jobs"),
    ARGH_STRING('o', "output", &output, "Output file", ARGH_REQUIRED),
    ARGH_ENUM('m', "mode", &mode, modes, "Mode"),
    ARGH_END,
};

int main(int argc, char **argv)
{
    double ratio = 1.0;
    argh_parser p;
    argh_init(&p, "cxx", nullptr);
    argh_table(&p, opts);
    argh_required(argh_double(&p, 'r', "ratio", &ratio, "Ratio"));
    return argh_parse(&p, argc, argv) ? 0 : argh_exit_code(&p);
}
