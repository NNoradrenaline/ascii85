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
