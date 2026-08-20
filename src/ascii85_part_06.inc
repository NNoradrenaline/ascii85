
static void parse_cli(int argc, char **argv, cli_options *opts) {
    int i;
    int end_options = 0;

    cli_init(opts);
    for (i = 1; i < argc; ++i) {
        const char *arg = argv[i];
        if (!end_options && strcmp(arg, "--") == 0) {
            end_options = 1;
            continue;
        }
        if (!end_options && strncmp(arg, "--", 2) == 0 && arg[2] != '\0') {
            parse_long_option(argc, argv, &i, opts);
            continue;
        }
        if (!end_options && arg[0] == '-' && arg[1] != '\0') {
            parse_short_options(argc, argv, &i, opts);
            continue;
        }
        set_filename(opts, arg);
    }
}

#ifndef ASCII85_NO_MAIN
int main(int argc, char **argv) {
    cli_options opts;
    FILE *fp = stdin;

    parse_cli(argc, argv, &opts);

    if (opts.help) {
        fputs(HELP_TEXT, stdout);
        return EXIT_SUCCESS;
    }
    if (opts.version) {
        printf(PROGRAM_NAME " %s\n", PROGRAM_VERSION);
        return EXIT_SUCCESS;
    }

    if (opts.a85x &&
        (opts.ignore_garbage || !opts.delims || opts.y_abbr || opts.wrap_set))
        die("-i, -n, -w, and -y are Ascii85-only options and cannot be used with --a85x");

    set_binary_stdio();

    if (opts.filename != NULL && strcmp(opts.filename, "-") != 0) {
        fp = fopen(opts.filename, "rb");
        if (fp == NULL) {
            fprintf(stderr, PROGRAM_NAME ": %s: %s\n",
                    opts.filename, strerror(errno));
            return EXIT_FAILURE;
        }
    }

    if (opts.a85x) {
        if (opts.decode)
            a85x_decode(fp);
        else
            a85x_encode(fp);
    } else {
        if (opts.decode)
            ascii85_decode(fp, opts.delims, opts.ignore_garbage);
        else
            ascii85_encode(fp, opts.delims, opts.wrap, opts.y_abbr);
    }

    if (fp != stdin && fclose(fp) != 0)
        die("input close error");
    if (fflush(stdout) == EOF)
        die("write error on standard output");

    return EXIT_SUCCESS;
}
#endif
