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

static uint32_t crc32_table[256];
static int crc32_table_ready = 0;

static void crc32_init_table(void) {
    unsigned n;
    if (crc32_table_ready)
        return;
    for (n = 0; n < 256U; ++n) {
        uint32_t c = (uint32_t)n;
        int k;
        for (k = 0; k < 8; ++k)
            c = (c >> 1) ^ (UINT32_C(0xEDB88320) &
                            (uint32_t)-(int32_t)(c & 1U));
        crc32_table[n] = c;
    }
    crc32_table_ready = 1;
}

static uint32_t crc32_start(void) {
    crc32_init_table();
    return UINT32_C(0xFFFFFFFF);
}

static uint32_t crc32_update(uint32_t crc, unsigned char byte) {
    unsigned index = (unsigned)((crc ^ (uint32_t)byte) & UINT32_C(0xFF));
    return crc32_table[index] ^ (crc >> 8);
}

static uint32_t crc32_finish(uint32_t crc) {
    return crc ^ UINT32_C(0xFFFFFFFF);
}

static int a85x_decode_table[256];
static int a85x_decode_table_ready = 0;

static void a85x_init_decode_table(void) {
    size_t i;
    if (a85x_decode_table_ready)
        return;
    for (i = 0; i < 256; ++i)
        a85x_decode_table[i] = -1;
    for (i = 0; i < 85; ++i)
        a85x_decode_table[(unsigned char)A85X_ALPHABET[i]] = (int)i;
    a85x_decode_table_ready = 1;
}

static int a85x_digit(unsigned char c) {
    a85x_init_decode_table();
    return a85x_decode_table[c];
}

static void a85x_encode_u32(uint32_t value, char out[5]) {
    int i;
    for (i = 4; i >= 0; --i) {
        out[i] = A85X_ALPHABET[value % 85U];
        value /= 85U;
    }
}

static void a85x_encode(FILE *fp) {
    unsigned char block[4] = {0, 0, 0, 0};
    size_t count = 0;
    uint32_t crc = crc32_start();
    int c;

    if (fputs(A85X_VERSION ":", stdout) == EOF)
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
