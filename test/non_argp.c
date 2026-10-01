#include "non_argp.h"

#include <string.h>

static const struct argp_option *find_key(const struct argp *argp, int key)
{
    const struct argp_option *option;
    for (option = argp->options; option && option->name; option++)
        if (option->key == key)
            return option;
    return NULL;
}

static const struct argp_option *find_name(const struct argp *argp,
                                           const char *name, size_t length)
{
    const struct argp_option *option;
    for (option = argp->options; option && option->name; option++)
        if (strlen(option->name) == length &&
            !strncmp(option->name, name, length))
            return option;
    return NULL;
}

static void help(const struct argp *argp, FILE *stream, unsigned flags,
                 const char *name)
{
    const struct argp_option *option;

    if (flags & ARGP_HELP_SHORT_USAGE)
        fprintf(stream, "Usage: %s [OPTION...]\n", name);
    else if (flags & ARGP_HELP_USAGE)
        fprintf(stream, "Usage: %s [OPTION...]\n", name);
    if ((flags & ARGP_HELP_DOC) && argp->doc)
        fputs(argp->doc, stream);
    if (flags & ARGP_HELP_LONG) {
        fputs("Options:\n", stream);
        for (option = argp->options; option && option->name; option++) {
            if (option->key < 0)
                continue;
            fprintf(stream, "  -%c, --%s", option->key, option->name);
            if (option->arg)
                fprintf(stream, "=%s", option->arg);
            if (option->doc)
                fprintf(stream, "\t%s", option->doc);
            fputc('\n', stream);
        }
    }
}

void argp_help(const struct argp *argp, FILE *stream, unsigned flags,
               const char *name)
{
    help(argp, stream, flags, name ? name : "program");
}

void argp_state_help(struct argp_state *state, FILE *stream, unsigned flags)
{
    help(state->argp, stream, flags, state->name);
}

error_t argp_parse(const struct argp *argp, int argc, char **argv,
                   unsigned flags, int *arg_index, void *input)
{
    struct argp_state state;
    int i;
    (void)flags;
    (void)input;
    state.out_stream = stderr;
    state.argp = argp;
    state.name = argc ? argv[0] : "program";

    for (i = 1; i < argc; i++) {
        const char *text = argv[i];
        const struct argp_option *option;
        char *value = NULL;
        int key;

        if (text[0] != '-' || !text[1])
            return ARGP_ERR_UNKNOWN;
        if (text[1] == '-') {
            const char *equal;
            size_t length;
            text += 2;
            equal = strchr(text, '=');
            length = equal ? (size_t)(equal - text) : strlen(text);
            option = find_name(argp, text, length);
            if (!option)
                return ARGP_ERR_UNKNOWN;
            key = option->key;
            if (equal)
                value = (char *)equal + 1;
        } else {
            key = (unsigned char)text[1];
            option = find_key(argp, key);
            if (!option)
                return ARGP_ERR_UNKNOWN;
            if (text[2])
                value = (char *)text + 2;
        }

        if (option->arg) {
            if (!value) {
                if (++i >= argc)
                    return ARGP_ERR_UNKNOWN;
                value = argv[i];
            }
        } else if (value) {
            return ARGP_ERR_UNKNOWN;
        }
        if (argp->parser(key, value, &state))
            return ARGP_ERR_UNKNOWN;
    }
    if (arg_index)
        *arg_index = argc;
    return 0;
}
