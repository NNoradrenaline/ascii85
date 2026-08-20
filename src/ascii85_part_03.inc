}

static int hex_upper_digit(int c) {
    if (c >= '0' && c <= '9')
        return c - '0';
    if (c >= 'A' && c <= 'F')
        return c - 'A' + 10;
    return -1;
}

static uint32_t read_crc32_field(FILE *fp) {
    uint32_t value = 0;
    int i;

    for (i = 0; i < 8; ++i) {
        int c = getc(fp);
        int d;
        if (c == EOF)
            die("A85X checksum is truncated");
        d = hex_upper_digit(c);
        if (d < 0)
            die("A85X checksum must be eight uppercase hexadecimal digits");
        value = (value << 4) | (uint32_t)d;
    }
    return value;
}

static void copy_stream(FILE *src, FILE *dst) {
    unsigned char buf[8192];
    size_t n;

    while ((n = fread(buf, 1, sizeof buf, src)) > 0) {
        if (fwrite(buf, 1, n, dst) != n)
            die("write error on standard output");
    }
    if (ferror(src))
        die("temporary stream read error");
}

static void expect_literal(FILE *fp, const char *literal) {
    size_t i;
    for (i = 0; literal[i] != '\0'; ++i) {
        int c = getc(fp);
        if (c == EOF || c != (unsigned char)literal[i])
            die("invalid or unsupported A85X version marker");
    }
}

static void a85x_decode(FILE *fp) {
    unsigned char group[5];
    size_t group_len = 0;
    unsigned char pending[4] = {0, 0, 0, 0};
    int have_pending = 0;
    int c;
    int pad;
    uint32_t crc = crc32_start();
    uint32_t expected_crc;
    FILE *tmp = tmpfile();

    if (tmp == NULL)
        die("could not create temporary verification stream");

    expect_literal(fp, A85X_VERSION ":");

    for (;;) {
        c = getc(fp);
        if (c == EOF) {
            fclose(tmp);
            die("truncated A85X payload");
        }
        if (c == ':')
            break;

        if (a85x_digit(c) < 0) {
            fclose(tmp);
            die_char("invalid A85X payload character", c);
        }

        group[group_len++] = (unsigned char)c;
        if (group_len == 5) {
            unsigned char decoded[4];
            a85x_group_to_bytes(group, decoded);
            if (have_pending)
                crc_write(tmp, pending, sizeof pending, &crc);
            memcpy(pending, decoded, sizeof pending);
            have_pending = 1;
            group_len = 0;
        }
    }

    if (group_len != 0) {
        fclose(tmp);
        die("A85X payload length is not a multiple of 5");
    }

    c = getc(fp);
    if (c < '0' || c > '3') {
        fclose(tmp);
        die("A85X pad must be exactly 0, 1, 2, or 3");
    }
    pad = c - '0';

    c = getc(fp);
    if (c != ':') {
        fclose(tmp);
        die("A85X pad must be followed by ':'");
    }

    expected_crc = read_crc32_field(fp);

    c = getc(fp);
    if (c != EOF) {
        fclose(tmp);
        die("trailing bytes after A85X checksum");
    }
    if (ferror(fp)) {
        fclose(tmp);
        die("read error");
    }

    if (!have_pending) {
        if (pad != 0) {
            fclose(tmp);
            die("empty A85X payload cannot have padding");
        }
    } else {
        size_t data_len = (size_t)(4 - pad);
        int i;
        if (pad > 0) {
            for (i = 4 - pad; i < 4; ++i) {
                if (pending[i] != 0) {
                    fclose(tmp);
                    die("A85X declared padding bytes are non-zero");
                }
            }
        }
        crc_write(tmp, pending, data_len, &crc);
    }

    crc = crc32_finish(crc);
    if (crc != expected_crc) {
        fclose(tmp);
        fprintf(stderr,
                PROGRAM_NAME ": CRC-32 mismatch: encoded=%08" PRIX32
                ", computed=%08" PRIX32 "\n",
                expected_crc, crc);
        exit(EXIT_FAILURE);
    }

    if (fflush(tmp) == EOF || fseek(tmp, 0, SEEK_SET) != 0) {
