/*
 * ascii85 - Ascii85/A85X encode/decode data and print to standard output
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
#include <inttypes.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <fcntl.h>
#include <io.h>
#endif

#define PROGRAM_NAME "ascii85"
#define PROGRAM_VERSION "2.2.0-a85x"
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
    if (c >= 0 && c <= UCHAR_MAX && isprint((unsigned char)c))
        fprintf(stderr, PROGRAM_NAME ": %s '%c'\n", prefix, c);
    else
        fprintf(stderr, PROGRAM_NAME ": %s 0x%02X\n", prefix, (unsigned)c & 0xFFU);
    exit(EXIT_FAILURE);
}

static void set_binary_stdio(void) {
#ifdef _WIN32
    if (_setmode(_fileno(stdin), _O_BINARY) == -1)
        die("could not set stdin to binary mode");
    if (_setmode(_fileno(stdout), _O_BINARY) == -1)
        die("could not set stdout to binary mode");
#endif
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
