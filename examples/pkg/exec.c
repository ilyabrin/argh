/*
 * pkg exec -- <program> [args...]: run a program with the project's packages.
 *
 * Everything after "--" is passed on untouched, including options like
 * --version that pkg itself would otherwise take.
 */

#include "pkg.h"

const argh_opt exec_opts[] = {
    ARGH_REST("command", &opt.exec.command, "Program and its arguments, after --", ARGH_REQUIRED),
    ARGH_END
};

int cmd_exec(argh_parser *p, void *user)
{
    pkg_context *ctx = (pkg_context *)user;
    (void)p;
    fprintf(ctx->out, "Would run:");
    for (int i = 0; i < opt.exec.command.count; i++)
        fprintf(ctx->out, " %s", opt.exec.command.items[i]);
    fprintf(ctx->out, "\n");
    return 0;
}
