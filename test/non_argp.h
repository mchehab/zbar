/* Minimal argp interface for platforms without GNU argp. */
#ifndef ZBAR_TEST_NON_ARGP_H
#define ZBAR_TEST_NON_ARGP_H

#include <stdio.h>

typedef int error_t;

struct argp_option {
    const char *name;
    int key;
    const char *arg;
    int flags;
    const char *doc;
    int group;
};

struct argp_state;
struct argp {
    const struct argp_option *options;
    error_t (*parser)(int key, char *arg, struct argp_state *state);
    const char *args_doc;
    const char *doc;
};

struct argp_state {
    FILE *out_stream;
    const struct argp *argp;
    const char *name;
};

#define ARGP_ERR_UNKNOWN 16
#define ARGP_NO_HELP 1
#define ARGP_NO_EXIT 2
#define ARGP_HELP_SHORT_USAGE 1
#define ARGP_HELP_LONG 2
#define ARGP_HELP_DOC 4
#define ARGP_HELP_USAGE 8

error_t argp_parse(const struct argp *argp, int argc, char **argv,
                   unsigned flags, int *arg_index, void *input);
void argp_help(const struct argp *argp, FILE *stream, unsigned flags,
               const char *name);
void argp_state_help(struct argp_state *state, FILE *stream, unsigned flags);

#endif
