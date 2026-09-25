/*
 * pkg exec <program> [args...]: run a program with the project's packages.
 *
 * The command is registered with ARGH_POSIX (see main.c): options end at the
 * program name, so its own options, even --version or --help, are passed on
 * untouched. Options for exec itself go before the program name.
 */

#include "pkg.h"

const argh_opt exec_opts[] = {
    ARGH_REST("command", &opt.exec.command, "Program and its arguments", ARGH_REQUIRED),
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
