}

static int parse_wrap(const char *s) {
    char *end = NULL;
    long value;
    errno = 0;
    value = strtol(s, &end, 10);
    if (errno != 0 || end == s || *end != '\0' || value < 0 || value > INT_MAX)
        die("--wrap requires an integer from 0 to INT_MAX");
    return (int)value;
}

typedef struct {
    int decode;
    int ignore_garbage;
    int delims;
    int wrap;
    int wrap_set;
    int y_abbr;
    int a85x;
    int help;
    int version;
    const char *filename;
} cli_options;

static void cli_init(cli_options *opts) {
    memset(opts, 0, sizeof *opts);
    opts->delims = 1;
    opts->wrap = 76;
}

static int long_option_value(const char *arg, const char *name, const char **value) {
    size_t n = strlen(name);
    if (strncmp(arg, name, n) != 0)
        return 0;
    if (arg[n] == '=') {
        *value = arg + n + 1;
        return 1;
    }
    if (arg[n] == '\0') {
        *value = NULL;
        return 1;
    }
    return 0;
}

static void set_filename(cli_options *opts, const char *arg) {
    if (opts->filename != NULL)
        die("too many operands");
    opts->filename = arg;
}

static void parse_long_option(int argc, char **argv, int *index, cli_options *opts) {
    const char *arg = argv[*index];
    const char *value = NULL;

    if (strcmp(arg, "--decode") == 0)
        opts->decode = 1;
    else if (strcmp(arg, "--a85x") == 0)
        opts->a85x = 1;
    else if (strcmp(arg, "--ignore-garbage") == 0)
        opts->ignore_garbage = 1;
    else if (strcmp(arg, "--no-delims") == 0)
        opts->delims = 0;
    else if (strcmp(arg, "--y-abbr") == 0)
        opts->y_abbr = 1;
    else if (strcmp(arg, "--help") == 0)
        opts->help = 1;
    else if (strcmp(arg, "--version") == 0)
        opts->version = 1;
    else if (long_option_value(arg, "--wrap", &value)) {
        if (value == NULL) {
            ++(*index);
            if (*index >= argc)
                die("--wrap requires an argument");
            value = argv[*index];
        }
        opts->wrap = parse_wrap(value);
        opts->wrap_set = 1;
    } else {
        fprintf(stderr, PROGRAM_NAME ": unrecognized option '%s'\n", arg);
        fputs(USAGE_TEXT, stderr);
        exit(EXIT_FAILURE);
    }
}

static void parse_short_options(int argc, char **argv, int *index, cli_options *opts) {
    const char *arg = argv[*index];
    size_t pos;

    for (pos = 1; arg[pos] != '\0'; ++pos) {
        switch (arg[pos]) {
            case 'd': opts->decode = 1; break;
            case 'x': opts->a85x = 1; break;
            case 'i': opts->ignore_garbage = 1; break;
            case 'n': opts->delims = 0; break;
            case 'y': opts->y_abbr = 1; break;
            case 'h': opts->help = 1; break;
            case 'w': {
                const char *value;
                if (arg[pos + 1] != '\0') {
                    value = arg + pos + 1;
                    pos = strlen(arg) - 1;
                } else {
                    ++(*index);
                    if (*index >= argc)
                        die("-w requires an argument");
                    value = argv[*index];
                }
                opts->wrap = parse_wrap(value);
                opts->wrap_set = 1;
                return;
            }
            default:
                fprintf(stderr, PROGRAM_NAME ": invalid option -- '%c'\n", arg[pos]);
                fputs(USAGE_TEXT, stderr);
                exit(EXIT_FAILURE);
        }
    }
}
