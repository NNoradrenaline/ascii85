        die("read error");

    if (count > 0) {
        char encoded[5];
        uint32_t value = ((uint32_t)block[0] << 24) |
                         ((uint32_t)block[1] << 16) |
                         ((uint32_t)block[2] << 8) |
                         (uint32_t)block[3];
        a85x_encode_u32(value, encoded);
        if (fwrite(encoded, 1, 5, out) != 5)
            die("output write error");
    }

    if (fprintf(out, ":%zu:%08" PRIX32,
                count == 0 ? (size_t)0 : (size_t)4 - count,
                crc32_finish(crc)) < 0)
        die("output write error");
}

typedef enum {
    A85X_STATE_VERSION,
    A85X_STATE_PAYLOAD,
    A85X_STATE_PAD,
    A85X_STATE_PAD_COLON,
    A85X_STATE_CRC,
    A85X_STATE_DONE,
    A85X_STATE_ERROR
} a85x_state;

typedef struct {
    a85x_state state;
    size_t version_pos;
    uint64_t group_value;
    unsigned group_count;
    uint64_t group_total;
    unsigned char pending[4];
    int have_pending;
    unsigned pad;
    unsigned crc_digits;
    uint32_t expected_crc;
    uint32_t crc;
    FILE *sink;
    const char *error;
} a85x_decoder;

static const char A85X_PREFIX[] = A85X_VERSION ":";

static int a85x_decoder_fail(a85x_decoder *dec, const char *error) {
    dec->state = A85X_STATE_ERROR;
    dec->error = error;
    return 0;
}

static int a85x_decoder_write(a85x_decoder *dec,
                              const unsigned char *bytes, size_t count) {
    size_t i;
    for (i = 0; i < count; ++i)
        dec->crc = crc32_update(dec->crc, bytes[i]);
    if (dec->sink != NULL && count > 0 &&
        fwrite(bytes, 1, count, dec->sink) != count)
        return a85x_decoder_fail(dec, "temporary verification stream write error");
    return 1;
}

static int a85x_decoder_flush_pending(a85x_decoder *dec, size_t count) {
    if (!dec->have_pending)
        return count == 0 ? 1 : a85x_decoder_fail(dec, "internal A85X decoder error");
    if (!a85x_decoder_write(dec, dec->pending, count))
        return 0;
    dec->have_pending = 0;
    return 1;
}

static void a85x_decoder_init(a85x_decoder *dec, FILE *sink) {
    memset(dec, 0, sizeof *dec);
    dec->state = A85X_STATE_VERSION;
    dec->crc = crc32_start();
    dec->sink = sink;
}

static int a85x_decoder_feed(a85x_decoder *dec, unsigned char c) {
    if (dec->state == A85X_STATE_ERROR)
        return 0;
    if (dec->state == A85X_STATE_DONE)
        return a85x_decoder_fail(dec, "trailing data after A85X checksum");

    switch (dec->state) {
        case A85X_STATE_VERSION:
            if (c != (unsigned char)A85X_PREFIX[dec->version_pos])
                return a85x_decoder_fail(dec, "invalid or unsupported A85X version");
            ++dec->version_pos;
            if (dec->version_pos == sizeof A85X_PREFIX - 1)
                dec->state = A85X_STATE_PAYLOAD;
            return 1;

        case A85X_STATE_PAYLOAD:
            if (c == ':') {
                if (dec->group_count != 0)
                    return a85x_decoder_fail(dec, "A85X payload length is not a multiple of 5");
                dec->state = A85X_STATE_PAD;
                return 1;
            } else {
                int d = a85x_digit(c);
                if (d < 0)
                    return a85x_decoder_fail(dec, "invalid A85X payload character");
                dec->group_value = dec->group_value * 85U + (uint64_t)(unsigned)d;
                ++dec->group_count;
                if (dec->group_count == 5) {
                    uint32_t value;
                    if (dec->group_value > UINT32_MAX)
                        return a85x_decoder_fail(dec, "A85X payload group exceeds 32-bit range");
                    if (dec->have_pending && !a85x_decoder_flush_pending(dec, 4))
                        return 0;
                    value = (uint32_t)dec->group_value;
                    dec->pending[0] = (unsigned char)(value >> 24);
                    dec->pending[1] = (unsigned char)(value >> 16);
                    dec->pending[2] = (unsigned char)(value >> 8);
                    dec->pending[3] = (unsigned char)value;
                    dec->have_pending = 1;
                    ++dec->group_total;
