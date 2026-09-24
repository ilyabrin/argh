/*
 * Parse-speed benchmark: argh vs getopt_long.
 *
 * Both parsers get the same workload: 30 defined options and a 17-argument
 * command line mixing short flags, long options, values and positionals.
 * Each iteration does the full job a real program does once: set up the
 * parser, parse, read the values back.
 *
 * Build: make bench
 */

#define _POSIX_C_SOURCE 200809L /* getopt_long on glibc */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <getopt.h>

/* Count heap allocations made by argh. The macros only affect code below. */
static size_t g_allocs;
static void *counting_malloc(size_t n) { g_allocs++; return malloc(n); }
static void *counting_realloc(void *p, size_t n) { g_allocs++; return realloc(p, n); }
#define malloc counting_malloc
#define realloc counting_realloc

#define ARGH_IMPLEMENTATION
#include "../argh.h"

#undef malloc
#undef realloc

#define ITERATIONS 200000
#define RUNS 5
#define NUM_OPTS 30

static char *const workload[] = {
    "prog",
    "-v", "--quiet",
    "-o", "out.txt",
    "--jobs=8",
    "input1.txt",
    "-abc",
    "--level", "3",
    "--ratio=0.75",
    "input2.txt",
    "--name", "benchmark",
    "-t", "42",
    "--force",
    "input3.txt",
};
#define WORKLOAD_ARGC ((int)(sizeof(workload) / sizeof(workload[0])))

/* Filler options so lookups scan a realistic table */
static const char *const filler_long[] = {
    "opt01", "opt02", "opt03", "opt04", "opt05", "opt06", "opt07", "opt08",
    "opt09", "opt10", "opt11", "opt12", "opt13", "opt14", "opt15", "opt16",
    "opt17", "opt18",
};
#define NUM_FILLER ((int)(sizeof(filler_long) / sizeof(filler_long[0])))

static volatile long g_sink; /* keeps results alive under -O2 */

static double now_ns(void)
{
    struct timespec ts;
    timespec_get(&ts, TIME_UTC);
    return (double)ts.tv_sec * 1e9 + (double)ts.tv_nsec;
}

/* ------------------------------------------------------------------------ */

static void run_argh(char **argv)
{
    argh_Parser p;
    argh_init(&p, WORKLOAD_ARGC, argv);

    argh_add(&p, "v", "verbose", ARGH_BOOL, NULL, "");
    argh_add(&p, "q", "quiet", ARGH_BOOL, NULL, "");
    argh_add(&p, "o", "output", ARGH_STRING, NULL, "");
    argh_add(&p, "j", "jobs", ARGH_INT, "1", "");
    argh_add(&p, "a", "all", ARGH_BOOL, NULL, "");
    argh_add(&p, "b", "brief", ARGH_BOOL, NULL, "");
    argh_add(&p, "c", "color", ARGH_BOOL, NULL, "");
    argh_add(&p, "l", "level", ARGH_INT, "0", "");
    argh_add(&p, "r", "ratio", ARGH_DOUBLE, "1.0", "");
    argh_add(&p, "n", "name", ARGH_STRING, NULL, "");
    argh_add(&p, "t", "threads", ARGH_INT, "1", "");
    argh_add(&p, "f", "force", ARGH_BOOL, NULL, "");
    for (int i = 0; i < NUM_FILLER; i++)
        argh_add(&p, NULL, filler_long[i], ARGH_STRING, NULL, "");

    if (!argh_parse(&p))
    {
        fprintf(stderr, "argh: unexpected parse failure\n");
        exit(1);
    }

    long sum = argh_get_bool(&p, "verbose") + argh_get_bool(&p, "quiet") +
               argh_get_bool(&p, "all") + argh_get_bool(&p, "brief") +
               argh_get_bool(&p, "color") + argh_get_bool(&p, "force") +
               argh_get_int(&p, "jobs") + argh_get_int(&p, "level") +
               argh_get_int(&p, "threads") + (long)(argh_get_double(&p, "ratio") * 100) +
               (long)strlen(argh_get_string(&p, "output")) +
               (long)strlen(argh_get_string(&p, "name")) + (long)p.positional_count;
    g_sink += sum;

    argh_free(&p);
}

/* ------------------------------------------------------------------------ */

static void reset_getopt(void)
{
#if defined(__APPLE__) || defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__)
    extern int optreset;
    optreset = 1;
    optind = 1;
#else
    optind = 0; /* glibc and mingw-w64: full reinitialization */
#endif
}

static void run_getopt(char **argv)
{
    static struct option longopts[NUM_OPTS + 1];
    static int initialized;
    if (!initialized)
    {
        static const struct option base[] = {
            {"verbose", no_argument, 0, 'v'},
            {"quiet", no_argument, 0, 'q'},
            {"output", required_argument, 0, 'o'},
            {"jobs", required_argument, 0, 'j'},
            {"all", no_argument, 0, 'a'},
            {"brief", no_argument, 0, 'b'},
            {"color", no_argument, 0, 'c'},
            {"level", required_argument, 0, 'l'},
            {"ratio", required_argument, 0, 'r'},
            {"name", required_argument, 0, 'n'},
            {"threads", required_argument, 0, 't'},
            {"force", no_argument, 0, 'f'},
        };
        int n = (int)(sizeof(base) / sizeof(base[0]));
        memcpy(longopts, base, sizeof(base));
        for (int i = 0; i < NUM_FILLER; i++)
        {
            longopts[n + i].name = filler_long[i];
            longopts[n + i].has_arg = required_argument;
            longopts[n + i].flag = 0;
            longopts[n + i].val = 1000 + i;
        }
        initialized = 1;
    }

    /* Same work as argh: typed values, validation of numbers */
    int verbose = 0, quiet = 0, all = 0, brief = 0, color = 0, force = 0;
    int jobs = 1, level = 0, threads = 1;
    double ratio = 1.0;
    const char *output = NULL, *name = NULL;

    reset_getopt();
    int c;
    while ((c = getopt_long(WORKLOAD_ARGC, argv, "vqo:j:abcl:r:n:t:f", longopts, NULL)) != -1)
    {
        char *end;
        switch (c)
        {
        case 'v': verbose = 1; break;
        case 'q': quiet = 1; break;
        case 'a': all = 1; break;
        case 'b': brief = 1; break;
        case 'c': color = 1; break;
        case 'f': force = 1; break;
        case 'o': output = optarg; break;
        case 'n': name = optarg; break;
        case 'j': jobs = (int)strtol(optarg, &end, 10); break;
        case 'l': level = (int)strtol(optarg, &end, 10); break;
        case 't': threads = (int)strtol(optarg, &end, 10); break;
        case 'r': ratio = strtod(optarg, &end); break;
        default:
            fprintf(stderr, "getopt: unexpected option\n");
            exit(1);
        }
    }

    long sum = verbose + quiet + all + brief + color + force + jobs + level + threads +
               (long)(ratio * 100) + (long)strlen(output) + (long)strlen(name) +
               (WORKLOAD_ARGC - optind);
    g_sink += sum;
}

/* ------------------------------------------------------------------------ */

/* getopt permutes argv, so both parsers get a fresh copy every iteration */
static double bench(void (*fn)(char **))
{
    char *argv[WORKLOAD_ARGC + 1];
    double best = 1e30;
    for (int run = 0; run < RUNS; run++)
    {
        double start = now_ns();
        for (int i = 0; i < ITERATIONS; i++)
        {
            memcpy(argv, workload, sizeof(workload));
            argv[WORKLOAD_ARGC] = NULL;
            fn(argv);
        }
        double per_iter = (now_ns() - start) / ITERATIONS;
        if (per_iter < best)
            best = per_iter;
    }
    return best;
}

int main(void)
{
    char *argv[WORKLOAD_ARGC + 1];

    /* Sanity check both parsers read the same values */
    memcpy(argv, workload, sizeof(workload));
    g_sink = 0;
    run_argh(argv);
    long argh_result = g_sink;
    memcpy(argv, workload, sizeof(workload));
    g_sink = 0;
    run_getopt(argv);
    if (argh_result != g_sink)
    {
        fprintf(stderr, "Parsers disagree: argh=%ld getopt=%ld\n", argh_result, (long)g_sink);
        return 1;
    }

    g_allocs = 0;
    memcpy(argv, workload, sizeof(workload));
    run_argh(argv);
    size_t allocs = g_allocs;

    double t_argh = bench(run_argh);
    double t_getopt = bench(run_getopt);

    printf("Workload: %d options defined, %d arguments\n", NUM_OPTS, WORKLOAD_ARGC - 1);
    printf("Best of %d runs x %d iterations\n\n", RUNS, ITERATIONS);
    printf("%-14s %12s %14s\n", "parser", "ns/parse", "heap allocs");
    printf("%-14s %12.0f %14zu\n", "argh v0.1", t_argh, allocs);
    printf("%-14s %12.0f %14s\n", "getopt_long", t_getopt, "not counted");
    printf("\nsizeof(argh_Parser) = %zu bytes\n", sizeof(argh_Parser));
    return 0;
}
