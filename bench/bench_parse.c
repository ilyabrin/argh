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

/* Variables the options write to, reset before every parse */
static struct
{
    bool verbose, quiet, all, brief, color, force;
    int jobs, level, threads;
    double ratio;
    const char *output, *name;
    const char *filler[NUM_FILLER];
    argh_values rest;
} v;

static void reset_values(void)
{
    memset(&v, 0, sizeof(v));
    v.jobs = 1;
    v.threads = 1;
    v.ratio = 1.0;
}

static long argh_checksum(void)
{
    return v.verbose + v.quiet + v.all + v.brief + v.color + v.force + v.jobs + v.level +
           v.threads + (long)(v.ratio * 100) + (long)strlen(v.output) + (long)strlen(v.name) +
           v.rest.count;
}

static void parse_or_die(argh_parser *p, char **argv)
{
    if (!argh_parse(p, WORKLOAD_ARGC, argv))
    {
        fprintf(stderr, "argh: unexpected parse failure\n");
        exit(1);
    }
}

/* Level 1: options added with builder calls on every run */
static void run_argh_builder(char **argv)
{
    argh_parser p;
    reset_values();
    argh_init(&p, "prog", NULL);
    argh_flag(&p, 'v', "verbose", &v.verbose, "");
    argh_flag(&p, 'q', "quiet", &v.quiet, "");
    argh_string(&p, 'o', "output", &v.output, "");
    argh_int(&p, 'j', "jobs", &v.jobs, "");
    argh_flag(&p, 'a', "all", &v.all, "");
    argh_flag(&p, 'b', "brief", &v.brief, "");
    argh_flag(&p, 'c', "color", &v.color, "");
    argh_int(&p, 'l', "level", &v.level, "");
    argh_double(&p, 'r', "ratio", &v.ratio, "");
    argh_string(&p, 'n', "name", &v.name, "");
    argh_int(&p, 't', "threads", &v.threads, "");
    argh_flag(&p, 'f', "force", &v.force, "");
    for (int i = 0; i < NUM_FILLER; i++)
        argh_string(&p, 0, filler_long[i], &v.filler[i], "");
    argh_rest(&p, "files", &v.rest, "");
    parse_or_die(&p, argv);
    g_sink += argh_checksum();
}

/* Level 2: one static const table, as an embedded program would use */
static const argh_opt table[] = {
    ARGH_FLAG('v', "verbose", &v.verbose, ""),
    ARGH_FLAG('q', "quiet", &v.quiet, ""),
    ARGH_STRING('o', "output", &v.output, ""),
    ARGH_INT('j', "jobs", &v.jobs, ""),
    ARGH_FLAG('a', "all", &v.all, ""),
    ARGH_FLAG('b', "brief", &v.brief, ""),
    ARGH_FLAG('c', "color", &v.color, ""),
    ARGH_INT('l', "level", &v.level, ""),
    ARGH_DOUBLE('r', "ratio", &v.ratio, ""),
    ARGH_STRING('n', "name", &v.name, ""),
    ARGH_INT('t', "threads", &v.threads, ""),
    ARGH_FLAG('f', "force", &v.force, ""),
    ARGH_STRING(0, "opt01", &v.filler[0], ""),
    ARGH_STRING(0, "opt02", &v.filler[1], ""),
    ARGH_STRING(0, "opt03", &v.filler[2], ""),
    ARGH_STRING(0, "opt04", &v.filler[3], ""),
    ARGH_STRING(0, "opt05", &v.filler[4], ""),
    ARGH_STRING(0, "opt06", &v.filler[5], ""),
    ARGH_STRING(0, "opt07", &v.filler[6], ""),
    ARGH_STRING(0, "opt08", &v.filler[7], ""),
    ARGH_STRING(0, "opt09", &v.filler[8], ""),
    ARGH_STRING(0, "opt10", &v.filler[9], ""),
    ARGH_STRING(0, "opt11", &v.filler[10], ""),
    ARGH_STRING(0, "opt12", &v.filler[11], ""),
    ARGH_STRING(0, "opt13", &v.filler[12], ""),
    ARGH_STRING(0, "opt14", &v.filler[13], ""),
    ARGH_STRING(0, "opt15", &v.filler[14], ""),
    ARGH_STRING(0, "opt16", &v.filler[15], ""),
    ARGH_STRING(0, "opt17", &v.filler[16], ""),
    ARGH_STRING(0, "opt18", &v.filler[17], ""),
    ARGH_REST("files", &v.rest, ""),
    ARGH_END,
};

static void run_argh_table(char **argv)
{
    argh_parser p;
    reset_values();
    argh_init(&p, "prog", NULL);
    argh_table(&p, table);
    parse_or_die(&p, argv);
    g_sink += argh_checksum();
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

/* Runs one parser on a fresh argv copy and returns its checksum */
static long checksum_of(void (*fn)(char **))
{
    char *argv[WORKLOAD_ARGC + 1];
    memcpy(argv, workload, sizeof(workload));
    argv[WORKLOAD_ARGC] = NULL;
    g_sink = 0;
    fn(argv);
    return g_sink;
}

int main(void)
{
    /* Sanity check: all parsers read the same values */
    long expected = checksum_of(run_getopt);
    long builder = checksum_of(run_argh_builder);
    long table = checksum_of(run_argh_table);
    if (builder != expected || table != expected)
    {
        fprintf(stderr, "Parsers disagree: getopt=%ld builder=%ld table=%ld\n", expected, builder, table);
        return 1;
    }

    /* argh makes no heap allocations, so the counters are normally unused.
     * They stay in place to catch any allocation that creeps back in. */
    (void)counting_malloc;
    (void)counting_realloc;
    g_allocs = 0;
    checksum_of(run_argh_builder);
    checksum_of(run_argh_table);
    size_t allocs = g_allocs;

    double t_builder = bench(run_argh_builder);
    double t_table = bench(run_argh_table);
    double t_getopt = bench(run_getopt);

    printf("Workload: %d options defined, %d arguments\n", NUM_OPTS, WORKLOAD_ARGC - 1);
    printf("Best of %d runs x %d iterations\n\n", RUNS, ITERATIONS);
    printf("%-16s %12s %14s\n", "parser", "ns/parse", "heap allocs");
    printf("%-16s %12.0f %14zu\n", "argh (builder)", t_builder, allocs);
    printf("%-16s %12.0f %14zu\n", "argh (table)", t_table, allocs);
    printf("%-16s %12.0f %14s\n", "getopt_long", t_getopt, "not counted");
    printf("\nsizeof(argh_parser) = %zu bytes (%zu without builder storage)\n", sizeof(argh_parser),
           sizeof(argh_parser) - sizeof(argh_opt) * (ARGH_BUILDER_CAP + 1));
    return 0;
}
