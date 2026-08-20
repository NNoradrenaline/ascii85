/*
 * ascii85 - Ascii85 encode/decode data and print to standard output
 *
 * Original work Copyright (C) 2012-2016 Remy Oukaour
 * <http://www.remyoukaour.com>.
 * Fork enhancements Copyright (C) 2026 A85X contributors.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <ctype.h>
#include <errno.h>
#include <getopt.h>
#include <inttypes.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PROGRAM_NAME "ascii85"
#define PROGRAM_VERSION "2.0.0-a85x"
#define A85X_VERSION "A85X1"

static const char A85X_ALPHABET[] =
    "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz"
    "!#$%&()*+,-./=?@[]^_{}~";

static const char *USAGE_TEXT =
    "usage: " PROGRAM_NAME " [OPTION]... [FILE]\n"
    "Try '" PROGRAM_NAME " --help' for more information.\n";

static const char *HELP_TEXT =
    "NAME\n"
    "    " PROGRAM_NAME " - Ascii85/A85X encode or decode data\n\n"
    "SYNOPSIS\n"
    "    " PROGRAM_NAME " [OPTION]... [FILE]\n\n"
    "DESCRIPTION\n"
    "    Encode FILE (or stdin) as Ascii85 by default. Decode with -d.\n"
    "    Use -x/--a85x for the canonical A85X1 format.\n\n"
    "OPTIONS\n"
    "    -d, --decode\n"
    "        decode data instead of encoding\n\n"
    "    -x, --a85x\n"
    "        use A85X1 instead of classic Ascii85\n\n"
    "    -i, --ignore-garbage\n"
    "        Ascii85 decode only: skip invalid non-whitespace characters\n\n"
    "    -n, --no-delims\n"
    "        Ascii85 only: omit/ignore <~ and ~> delimiters\n\n"
    "    -w, --wrap=COLS\n"
    "        Ascii85 encode only: wrap after COLS characters (default 76);\n"
    "        use 0 to disable wrapping\n\n"
    "    -y, --y-abbr\n"
    "        Ascii85 only: abbreviate four spaces as 'y'\n\n"
    "        --version\n"
    "        display version information and exit\n\n"
    "    -h, --help\n"
    "        display this help and exit\n\n"
    "A85X1\n"
    "    Canonical envelope: A85X1:<payload>:<pad>:<CRC32>\n"
    "    The checksum detects accidental corruption; it is not cryptographic.\n";

static void die(const char *msg) {
    fprintf(stderr, PROGRAM_NAME ": %s\n", msg);
    exit(EXIT_FAILURE);
}

static void die_char(const char *prefix, int c) {
    if (isprint((unsigned char)c))
        fprintf(stderr, PROGRAM_NAME ": %s '%c'\n", prefix, c);
    else
        fprintf(stderr, PROGRAM_NAME ": %s 0x%02X\n", prefix, (unsigned)c & 0xFFU);
    exit(EXIT_FAILURE);
}

static int getc_nospace(FILE *f) {
    int c;
    do {
        c = getc(f);
    } while (c != EOF && isspace((unsigned char)c));
    return c;
}

static void putc_checked(int c) {
    if (putchar(c) == EOF)
        die("write error on standard output");
}

static void putc_wrap(int c, int wrap, int *len) {
    if (wrap > 0 && *len >= wrap) {
        putc_checked('\n');
        *len = 0;
    }
    putc_checked(c);
    (*len)++;
}

static void ascii85_encode_tuple(uint32_t tuple, int count, int wrap,
                                 int *plen, int y_abbr) {
    char out[5];
    int i;

    if (tuple == 0 && count == 4) {
        putc_wrap('z', wrap, plen);
        return;
    }
    if (tuple == UINT32_C(0x20202020) && count == 4 && y_abbr) {
        putc_wrap('y', wrap, plen);
        return;
    }

    for (i = 4; i >= 0; --i) {
        out[i] = (char)(tuple % 85U + '!');
        tuple /= 85U;
    }

    for (i = 0; i < count + 1; ++i)
        putc_wrap(out[i], wrap, plen);
}

static void ascii85_write_tuple(uint32_t tuple, int bytes) {
    int i;
    for (i = 3; i >= 4 - bytes; --i)
        putc_checked((int)((tuple >> (i * 8)) & 0xFFU));
}

static void ascii85_encode(FILE *fp, int delims, int wrap, int y_abbr) {
    int c;
    int count = 0;
    int line_len = 0;
    uint32_t tuple = 0;

    if (delims) {
        putc_wrap('<', wrap, &line_len);
        putc_wrap('~', wrap, &line_len);
    }

    while ((c = getc(fp)) != EOF) {
        tuple |= (uint32_t)(unsigned char)c << ((3 - count) * 8);
        ++count;
        if (count == 4) {
            ascii85_encode_tuple(tuple, 4, wrap, &line_len, y_abbr);
            tuple = 0;
            count = 0;
        }
    }

    if (ferror(fp))
        die("read error");

    if (count > 0)
        ascii85_encode_tuple(tuple, count, wrap, &line_len, y_abbr);

    if (delims) {
        putc_wrap('~', wrap, &line_len);
        putc_wrap('>', wrap, &line_len);
    }
}

static void ascii85_decode(FILE *fp, int delims, int ignore_garbage) {
    int c;
    int count = 0;
    int found_end = 0;
    uint64_t tuple = 0;

    if (delims) {
        for (;;) {
            c = getc_nospace(fp);
            if (c == EOF)
                die("missing <~ delimiter");
            if (c == '<') {
                int next = getc_nospace(fp);
                if (next == '~')
                    break;
                if (next != EOF && ungetc(next, fp) == EOF)
                    die("input pushback failed");
            }
        }
    }

    for (;;) {
        c = getc_nospace(fp);

        if (c == EOF) {
            if (delims && !found_end)
                die("missing ~> delimiter");
            break;
        }

        if (delims && c == '~') {
            int next = getc_nospace(fp);
            if (next != '>')
                die("'~' delimiter is not followed by '>'");
            found_end = 1;
            break;
        }

        if (c == 'z' || c == 'y') {
            if (count != 0)
                die("'z'/'y' abbreviation inside a partial tuple");
            if (c == 'z')
                ascii85_write_tuple(0, 4);
            else
                ascii85_write_tuple(UINT32_C(0x20202020), 4);
            continue;
        }

        if (c < '!' || c > 'u') {
            if (ignore_garbage)
                continue;
            die_char("invalid Ascii85 character", c);
        }

        tuple = tuple * 85U + (uint64_t)(c - '!');
        ++count;

        if (count == 5) {
            if (tuple > UINT32_MAX)
                die("Ascii85 tuple exceeds 32-bit range");
            ascii85_write_tuple((uint32_t)tuple, 4);
            tuple = 0;
            count = 0;
        }
    }

    if (ferror(fp))
        die("read error");

    if (count == 1)
        die("invalid final Ascii85 tuple length of 1");

    if (count >= 2) {
        int original_count = count;
        while (count < 5) {
            tuple = tuple * 85U + 84U; /* pad with 'u' */
            ++count;
        }
        if (tuple > UINT32_MAX)
            die("final Ascii85 tuple exceeds 32-bit range");
        ascii85_write_tuple((uint32_t)tuple, original_count - 1);
    }
}

static uint32_t crc32_start(void) {
    return UINT32_C(0xFFFFFFFF);
}

static uint32_t crc32_update(uint32_t crc, unsigned char byte) {
    int i;
    crc ^= byte;
    for (i = 0; i < 8; ++i)
        crc = (crc >> 1) ^ (UINT32_C(0xEDB88320) & (uint32_t)-(int32_t)(crc & 1U));
    return crc;
}

static uint32_t crc32_finish(uint32_t crc) {
    return crc ^ UINT32_C(0xFFFFFFFF);
}

static void a85x_encode_u32(uint32_t value, char out[5]) {
    int i;
    for (i = 4; i >= 0; --i) {
        out[i] = A85X_ALPHABET[value % 85U];
        value /= 85U;
    }
}

static int a85x_digit(int c) {
    const char *p = strchr(A85X_ALPHABET, c);
    if (p == NULL)
        return -1;
    return (int)(p - A85X_ALPHABET);
}

static void a85x_encode(FILE *fp) {
    unsigned char block[4] = {0, 0, 0, 0};
    size_t count = 0;
    uint32_t crc = crc32_start();
    int c;

    fputs(A85X_VERSION ":", stdout);
    if (ferror(stdout))
        die("write error on standard output");

    while ((c = getc(fp)) != EOF) {
        char out[5];
        uint32_t value;
        unsigned char b = (unsigned char)c;
        crc = crc32_update(crc, b);
        block[count++] = b;
        if (count == 4) {
            value = ((uint32_t)block[0] << 24) |
                    ((uint32_t)block[1] << 16) |
                    ((uint32_t)block[2] << 8) |
                    (uint32_t)block[3];
            a85x_encode_u32(value, out);
            if (fwrite(out, 1, 5, stdout) != 5)
                die("write error on standard output");
            count = 0;
            memset(block, 0, sizeof block);
        }
    }

    if (ferror(fp))
        die("read error");

    if (count > 0) {
        char out[5];
        uint32_t value = ((uint32_t)block[0] << 24) |
                         ((uint32_t)block[1] << 16) |
                         ((uint32_t)block[2] << 8) |
                         (uint32_t)block[3];
        a85x_encode_u32(value, out);
        if (fwrite(out, 1, 5, stdout) != 5)
            die("write error on standard output");
    }

    printf(":%zu:%08" PRIX32, count == 0 ? 0U : 4U - count, crc32_finish(crc));
    if (ferror(stdout))
        die("write error on standard output");
}

static char *read_all_text(FILE *fp, size_t *len_out) {
    size_t cap = 4096;
    size_t len = 0;
    char *buf = malloc(cap + 1);
    int c;

    if (buf == NULL)
        die("out of memory");

    while ((c = getc(fp)) != EOF) {
        if (len == cap) {
            size_t next = cap > SIZE_MAX / 2 ? SIZE_MAX : cap * 2;
            char *grown;
            if (next <= cap) {
                free(buf);
                die("input too large");
            }
            grown = realloc(buf, next + 1);
            if (grown == NULL) {
                free(buf);
                die("out of memory");
            }
            buf = grown;
            cap = next;
        }
        buf[len++] = (char)c;
    }

    if (ferror(fp)) {
        free(buf);
        die("read error");
    }

    buf[len] = '\0';
    *len_out = len;
    return buf;
}

static int parse_hex8(const char *s, uint32_t *out) {
    uint32_t value = 0;
    int i;
    for (i = 0; i < 8; ++i) {
        int c = (unsigned char)s[i];
        unsigned digit;
        if (c >= '0' && c <= '9')
            digit = (unsigned)(c - '0');
        else if (c >= 'A' && c <= 'F')
            digit = (unsigned)(c - 'A' + 10);
        else
            return 0;
        value = (value << 4) | digit;
    }
    if (s[8] != '\0')
        return 0;
    *out = value;
    return 1;
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

static void a85x_decode(FILE *fp) {
    size_t len;
    char *text = read_all_text(fp, &len);
    char *c1;
    char *c2;
    char *c3;
    char *payload;
    char *pad_text;
    char *crc_text;
    size_t payload_len;
    int pad;
    uint32_t expected_crc;
    uint32_t crc = crc32_start();
    FILE *tmp;
    size_t pos;

    (void)len;

    c1 = strchr(text, ':');
    if (c1 == NULL) {
        free(text);
        die("invalid A85X1 envelope");
    }
    *c1 = '\0';
    c2 = strchr(c1 + 1, ':');
    if (c2 == NULL) {
        free(text);
        die("invalid A85X1 envelope");
    }
    *c2 = '\0';
    c3 = strchr(c2 + 1, ':');
    if (c3 == NULL || strchr(c3 + 1, ':') != NULL) {
        free(text);
        die("A85X1 envelope must contain exactly four fields");
    }
    *c3 = '\0';

    if (strcmp(text, A85X_VERSION) != 0) {
        free(text);
        die("unsupported A85X version");
    }

    payload = c1 + 1;
    pad_text = c2 + 1;
    crc_text = c3 + 1;
    payload_len = strlen(payload);

    if (payload_len % 5 != 0) {
        free(text);
        die("A85X payload length is not a multiple of 5");
    }
    if (pad_text[0] < '0' || pad_text[0] > '3' || pad_text[1] != '\0') {
        free(text);
        die("A85X pad must be exactly 0, 1, 2, or 3");
    }
    pad = pad_text[0] - '0';
    if (payload_len == 0 && pad != 0) {
        free(text);
        die("empty A85X payload cannot have padding");
    }
    if (!parse_hex8(crc_text, &expected_crc)) {
        free(text);
        die("A85X checksum must be eight uppercase hexadecimal digits");
    }

    tmp = tmpfile();
    if (tmp == NULL) {
        free(text);
        die("could not create temporary verification stream");
    }

    for (pos = 0; pos < payload_len; pos += 5) {
        uint64_t value = 0;
        unsigned char bytes[4];
        int i;
        int bytes_to_write = 4;
        int is_last = pos + 5 == payload_len;

        for (i = 0; i < 5; ++i) {
            int d = a85x_digit((unsigned char)payload[pos + (size_t)i]);
            if (d < 0) {
                int bad = (unsigned char)payload[pos + (size_t)i];
                fclose(tmp);
                free(text);
                die_char("invalid A85X payload character", bad);
            }
            value = value * 85U + (unsigned)d;
        }
        if (value > UINT32_MAX) {
            fclose(tmp);
            free(text);
            die("A85X payload group exceeds 32-bit range");
        }

        bytes[0] = (unsigned char)(value >> 24);
        bytes[1] = (unsigned char)(value >> 16);
        bytes[2] = (unsigned char)(value >> 8);
        bytes[3] = (unsigned char)value;

        if (is_last && pad > 0) {
            for (i = 4 - pad; i < 4; ++i) {
                if (bytes[i] != 0) {
                    fclose(tmp);
                    free(text);
                    die("A85X declared padding bytes are non-zero");
                }
            }
            bytes_to_write -= pad;
        }

        for (i = 0; i < bytes_to_write; ++i)
            crc = crc32_update(crc, bytes[i]);

        if (fwrite(bytes, 1, (size_t)bytes_to_write, tmp) != (size_t)bytes_to_write) {
            fclose(tmp);
            free(text);
            die("temporary verification stream write error");
        }
    }

    crc = crc32_finish(crc);
    if (crc != expected_crc) {
        fclose(tmp);
        free(text);
        fprintf(stderr,
                PROGRAM_NAME ": CRC-32 mismatch: encoded=%08" PRIX32
                ", computed=%08" PRIX32 "\n",
                expected_crc, crc);
        exit(EXIT_FAILURE);
    }

    if (fflush(tmp) == EOF || fseek(tmp, 0, SEEK_SET) != 0) {
        fclose(tmp);
        free(text);
        die("temporary verification stream seek error");
    }
    copy_stream(tmp, stdout);
    fclose(tmp);
    free(text);
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

int main(int argc, char **argv) {
    int opt;
    int decode = 0;
    int ignore_garbage = 0;
    int delims = 1;
    int wrap = 76;
    int wrap_set = 0;
    int y_abbr = 0;
    int a85x = 0;
    FILE *fp = stdin;

    static const struct option long_opts[] = {
        {"decode", no_argument, NULL, 'd'},
        {"a85x", no_argument, NULL, 'x'},
        {"ignore-garbage", no_argument, NULL, 'i'},
        {"no-delims", no_argument, NULL, 'n'},
        {"wrap", required_argument, NULL, 'w'},
        {"y-abbr", no_argument, NULL, 'y'},
        {"version", no_argument, NULL, 1000},
        {"help", no_argument, NULL, 'h'},
        {NULL, 0, NULL, 0}
    };

    opterr = 0;
    while ((opt = getopt_long(argc, argv, "dxinw:yh", long_opts, NULL)) != -1) {
        switch (opt) {
            case 'd': decode = 1; break;
            case 'x': a85x = 1; break;
            case 'i': ignore_garbage = 1; break;
            case 'n': delims = 0; break;
            case 'w': wrap = parse_wrap(optarg); wrap_set = 1; break;
            case 'y': y_abbr = 1; break;
            case 'h': fputs(HELP_TEXT, stdout); return EXIT_SUCCESS;
            case 1000:
                printf(PROGRAM_NAME " %s\n", PROGRAM_VERSION);
                return EXIT_SUCCESS;
            case '?':
            default:
                fputs(USAGE_TEXT, stderr);
                return EXIT_FAILURE;
        }
    }

    if (argc - optind > 1) {
        fputs(PROGRAM_NAME ": too many operands\n", stderr);
        fputs(USAGE_TEXT, stderr);
        return EXIT_FAILURE;
    }

    if (a85x && (ignore_garbage || !delims || y_abbr || wrap_set))
        die("-i, -n, -w, and -y are Ascii85-only options and cannot be used with --a85x");

    if (optind < argc && strcmp(argv[optind], "-") != 0) {
        fp = fopen(argv[optind], "rb");
        if (fp == NULL) {
            fprintf(stderr, PROGRAM_NAME ": %s: %s\n", argv[optind], strerror(errno));
            return EXIT_FAILURE;
        }
    }

    if (a85x) {
        if (decode)
            a85x_decode(fp);
        else
            a85x_encode(fp);
    } else {
        if (decode)
            ascii85_decode(fp, delims, ignore_garbage);
        else
            ascii85_encode(fp, delims, wrap, y_abbr);
    }

    if (fp != stdin && fclose(fp) != 0)
        die("input close error");
    if (fflush(stdout) == EOF)
        die("write error on standard output");

    return EXIT_SUCCESS;
}
