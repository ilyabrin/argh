/*
 * pkg: a package manager front end in the style of cargo, npm or git.
 *
 * Shared declarations. Each command lives in its own file with its own option
 * table and handler; main.c puts them together into a command tree.
 */

#ifndef PKG_H
#define PKG_H

#include <stdio.h>

#include "../../argh.h"

/* A version requirement: "1.2.3", "^1.2", "~1.4.0", ">=2.0.0", "*" */
typedef struct
{
    char op; /* '=', '^', '~', '>' (for >=) or '*' (any version) */
    int major, minor, patch;
} version_req;

extern const argh_type version_req_type;

/* Every option of every command ends up here. Commands read their part. */
typedef struct
{
    /* Global options */
    int verbosity;
    const char *dir;
    const char *registry;
    bool offline;
    int color; /* index into color_modes */

    struct
    {
        argh_values packages;
        version_req version;
        bool dev, optional;
        bool dry_run;
        bool trace; /* hidden, for debugging the resolver */
    } install;

    struct
    {
        argh_values packages;
        bool purge;
    } remove;

    struct
    {
        const char *name, *url;
        bool force;
    } remote_add;

    struct
    {
        const char *name;
    } remote_remove;

    struct
    {
        argh_values command;
    } exec;
} pkg_options;

extern pkg_options opt;

/* Decided once in main.c from the options, then handed to every handler */
typedef struct
{
    FILE *out;
    bool color;
    const char *registry;
} pkg_context;

/* Command modules */
extern const argh_opt install_opts[];
extern const argh_opt remove_opts[];
extern const argh_cmd remote_cmds[];
extern const argh_opt exec_opts[];

int cmd_install(argh_parser *p, void *ctx);
int cmd_remove(argh_parser *p, void *ctx);
int cmd_exec(argh_parser *p, void *ctx);

/* Prints "+ name ^1.2.0" style text for a requirement */
void print_version_req(FILE *out, const version_req *req);

#endif
