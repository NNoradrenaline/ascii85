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
    return CRC32_TABLE[(crc ^ byte) & 0xFFU] ^ (crc >> 8);
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

static const signed char *a85x_decode_table(void) {
    static signed char table[256];
    static int initialized = 0;
    size_t i;

    if (!initialized) {
        for (i = 0; i < sizeof table; ++i)
            table[i] = -1;
        for (i = 0; A85X_ALPHABET[i] != '\0'; ++i)
            table[(unsigned char)A85X_ALPHABET[i]] = (signed char)i;
        initialized = 1;
    }
    return table;
}

static int a85x_digit(int c) {
    const signed char *table = a85x_decode_table();
    if (c < 0 || c > UCHAR_MAX)
        return -1;
    return table[(unsigned char)c];
}

static void a85x_encode(FILE *fp) {
    unsigned char block[4] = {0, 0, 0, 0};
    size_t count = 0;
    uint32_t crc = crc32_start();
    int c;

    if (fputs(A85X_VERSION ":", stdout) == EOF)
        die("write error on standard output");

    while ((c = getc(fp)) != EOF) {
        unsigned char b = (unsigned char)c;
        crc = crc32_update(crc, b);
        block[count++] = b;

        if (count == 4) {
            char out[5];
            uint32_t value = ((uint32_t)block[0] << 24) |
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

    if (printf(":%zu:%08" PRIX32, count == 0 ? 0U : 4U - count,
               crc32_finish(crc)) < 0)
        die("write error on standard output");
}

static void a85x_group_to_bytes(const unsigned char group[5],
                                unsigned char bytes[4]) {
    uint64_t value = 0;
    int i;

    for (i = 0; i < 5; ++i) {
        int d = a85x_digit(group[i]);
        if (d < 0)
            die_char("invalid A85X payload character", group[i]);
        value = value * 85U + (unsigned)d;
    }

    if (value > UINT32_MAX)
        die("A85X payload group exceeds 32-bit range");

    bytes[0] = (unsigned char)(value >> 24);
    bytes[1] = (unsigned char)(value >> 16);
    bytes[2] = (unsigned char)(value >> 8);
    bytes[3] = (unsigned char)value;
}

static void crc_write(FILE *tmp, const unsigned char *buf, size_t len,
                      uint32_t *crc) {
    size_t i;
    for (i = 0; i < len; ++i)
        *crc = crc32_update(*crc, buf[i]);
    write_checked(tmp, buf, len, "temporary verification stream write error");
