        fclose(tmp);
        die("temporary verification stream seek error");
    }

    copy_stream(tmp, stdout);
    if (fclose(tmp) != 0)
        die("temporary verification stream close error");
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

static int starts_with(const char *s, const char *prefix) {
    return strncmp(s, prefix, strlen(prefix)) == 0;
}

static void set_operand(Options *o, const char *arg) {
    if (o->input_path != NULL)
        die("too many operands");
    o->input_path = arg;
}

static void parse_short_options(Options *o, int argc, char **argv, int *index) {
    const char *arg = argv[*index];
    size_t j;

    for (j = 1; arg[j] != '\0'; ++j) {
        switch (arg[j]) {
            case 'd': o->decode = 1; break;
            case 'x': o->a85x = 1; break;
            case 'i': o->ignore_garbage = 1; break;
            case 'n': o->delims = 0; break;
            case 'y': o->y_abbr = 1; break;
            case 'h': o->help = 1; break;
            case 'w': {
                const char *value = NULL;
                if (arg[j + 1] != '\0') {
                    value = arg + j + 1;
                    if (*value == '=')
                        ++value;
                } else {
                    if (*index + 1 >= argc)
                        die("-w/--wrap requires an argument");
                    ++(*index);
                    value = argv[*index];
                }
                o->wrap = parse_wrap(value);
                o->wrap_set = 1;
                return;
            }
            default:
                die_option(arg);
        }
    }
}

static void parse_args(Options *o, int argc, char **argv) {
    int i;
    int end_options = 0;

    memset(o, 0, sizeof *o);
    o->delims = 1;
    o->wrap = 76;

    for (i = 1; i < argc; ++i) {
        const char *arg = argv[i];

        if (!end_options && strcmp(arg, "--") == 0) {
            end_options = 1;
            continue;
        }

        if (!end_options && starts_with(arg, "--")) {
            if (strcmp(arg, "--decode") == 0)
                o->decode = 1;
            else if (strcmp(arg, "--a85x") == 0)
                o->a85x = 1;
            else if (strcmp(arg, "--ignore-garbage") == 0)
                o->ignore_garbage = 1;
            else if (strcmp(arg, "--no-delims") == 0)
                o->delims = 0;
            else if (strcmp(arg, "--y-abbr") == 0)
                o->y_abbr = 1;
            else if (strcmp(arg, "--help") == 0)
                o->help = 1;
            else if (strcmp(arg, "--version") == 0)
                o->version = 1;
            else if (starts_with(arg, "--wrap=")) {
                o->wrap = parse_wrap(arg + strlen("--wrap="));
                o->wrap_set = 1;
            } else if (strcmp(arg, "--wrap") == 0) {
                if (i + 1 >= argc)
                    die("-w/--wrap requires an argument");
                ++i;
                o->wrap = parse_wrap(argv[i]);
                o->wrap_set = 1;
            } else {
                die_option(arg);
            }
            continue;
        }

        if (!end_options && arg[0] == '-' && arg[1] != '\0') {
            parse_short_options(o, argc, argv, &i);
            continue;
        }

        set_operand(o, arg);
    }
}

int main(int argc, char **argv) {
    Options o;
    FILE *fp = stdin;

    set_binary_stdio();
    parse_args(&o, argc, argv);

    if (o.help) {
        fputs(HELP_TEXT, stdout);
        return ferror(stdout) ? EXIT_FAILURE : EXIT_SUCCESS;
    }
    if (o.version) {
        printf(PROGRAM_NAME " %s\n", PROGRAM_VERSION);
        return ferror(stdout) ? EXIT_FAILURE : EXIT_SUCCESS;
    }

    if (o.a85x && (o.ignore_garbage || !o.delims || o.y_abbr || o.wrap_set))
        die("-i, -n, -w, and -y are Ascii85-only options and cannot be used with --a85x");

    if (o.input_path != NULL && strcmp(o.input_path, "-") != 0) {
        fp = fopen(o.input_path, "rb");
        if (fp == NULL) {
            fprintf(stderr, PROGRAM_NAME ": %s: %s\n", o.input_path, strerror(errno));
            return EXIT_FAILURE;
        }
    }

    if (o.a85x) {
        if (o.decode)
            a85x_decode(fp);
        else
