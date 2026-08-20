
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

static FILE *text_input_stream(const char *text) {
    FILE *tmp = tmpfile();
    size_t len = strlen(text);

    if (tmp == NULL)
        die("could not create temporary text input stream");
    if (len > 0 && fwrite(text, 1, len, tmp) != len) {
        fclose(tmp);
        die("temporary text input write error");
    }
    if (fflush(tmp) == EOF || fseek(tmp, 0, SEEK_SET) != 0) {
        fclose(tmp);
        die("temporary text input seek error");
    }
    return tmp;
}

static FILE *staged_output_stream(void) {
    FILE *tmp = tmpfile();
    if (tmp == NULL)
        die("could not create temporary output stream");
    return tmp;
}

static void commit_staged_output(FILE *stage, const char *filename) {
    FILE *dst;

    if (fflush(stage) == EOF || fseek(stage, 0, SEEK_SET) != 0) {
        fclose(stage);
        die("temporary output seek error");
    }

    dst = fopen(filename, "wb");
    if (dst == NULL) {
        fprintf(stderr, PROGRAM_NAME ": %s: %s\n", filename, strerror(errno));
        fclose(stage);
        exit(EXIT_FAILURE);
    }

    copy_stream(stage, dst);
    if (fclose(dst) != 0) {
        fclose(stage);
        die("output close error");
    }
    if (fclose(stage) != 0)
        die("temporary output close error");
}

#ifndef ASCII85_NO_MAIN
int main(int argc, char **argv) {
    cli_options opts;
    FILE *fp = stdin;
    FILE *out = stdout;
    FILE *staged_out = NULL;

    parse_cli(argc, argv, &opts);

#ifdef _WIN32
    if (argc == 1 && _isatty(_fileno(stdin))) {
        fputs(HELP_TEXT, stdout);
        return EXIT_SUCCESS;
    }
#endif

    if (opts.help) {
        fputs(HELP_TEXT, stdout);
        return EXIT_SUCCESS;
    }
    if (opts.version) {
        printf(PROGRAM_NAME " %s\n", PROGRAM_VERSION);
        return EXIT_SUCCESS;
    }

    if (opts.text != NULL && opts.filename != NULL)
        die("--text cannot be combined with a FILE operand");
    if (opts.text != NULL && opts.decode)
        die("--text is encoding-only and cannot be used with --decode");

    if (opts.a85x &&
        (opts.ignore_garbage || !opts.delims || opts.y_abbr || opts.wrap_set))
        die("-i, -n, -w, and -y are Ascii85-only options and cannot be used with --a85x");

    set_binary_stdio();

    if (opts.text != NULL) {
        fp = text_input_stream(opts.text);
    } else if (opts.filename != NULL && strcmp(opts.filename, "-") != 0) {
        fp = fopen(opts.filename, "rb");
        if (fp == NULL) {
            fprintf(stderr, PROGRAM_NAME ": %s: %s\n",
                    opts.filename, strerror(errno));
            return EXIT_FAILURE;
        }
    }

    if (opts.output_filename != NULL && strcmp(opts.output_filename, "-") != 0) {
        staged_out = staged_output_stream();
        out = staged_out;
    }

    if (opts.a85x) {
        if (opts.decode)
            a85x_decode(fp, out);
        else
            a85x_encode(fp, out);
    } else {
        if (opts.decode)
            ascii85_decode(fp, out, opts.delims, opts.ignore_garbage);
        else
            ascii85_encode(fp, out, opts.delims, opts.wrap, opts.y_abbr);
    }

    if (fp != stdin && fclose(fp) != 0)
        die("input close error");

    if (staged_out != NULL)
        commit_staged_output(staged_out, opts.output_filename);
    else if (fflush(stdout) == EOF)
        die("output write error");

    return EXIT_SUCCESS;
}
#endif
