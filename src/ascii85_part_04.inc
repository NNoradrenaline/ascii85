                    dec->group_value = 0;
                    dec->group_count = 0;
                }
                return 1;
            }

        case A85X_STATE_PAD:
            if (c < '0' || c > '3')
                return a85x_decoder_fail(dec, "A85X pad must be exactly 0, 1, 2, or 3");
            dec->pad = (unsigned)(c - '0');
            dec->state = A85X_STATE_PAD_COLON;
            return 1;

        case A85X_STATE_PAD_COLON:
            if (c != ':')
                return a85x_decoder_fail(dec, "A85X pad must be followed by ':'");
            if (dec->group_total == 0 && dec->pad != 0)
                return a85x_decoder_fail(dec, "empty A85X payload cannot have padding");
            dec->state = A85X_STATE_CRC;
            return 1;

        case A85X_STATE_CRC: {
            unsigned digit;
            if (c >= '0' && c <= '9')
                digit = (unsigned)(c - '0');
            else if (c >= 'A' && c <= 'F')
                digit = (unsigned)(c - 'A' + 10);
            else
                return a85x_decoder_fail(dec, "A85X checksum must be eight uppercase hexadecimal digits");
            if (dec->crc_digits >= 8)
                return a85x_decoder_fail(dec, "trailing data after A85X checksum");
            dec->expected_crc = (dec->expected_crc << 4) | digit;
            ++dec->crc_digits;
            if (dec->crc_digits == 8)
                dec->state = A85X_STATE_DONE;
            return 1;
        }

        case A85X_STATE_DONE:
        case A85X_STATE_ERROR:
        default:
            return a85x_decoder_fail(dec, "internal A85X decoder state error");
    }
}

static int a85x_decoder_finish(a85x_decoder *dec) {
    size_t bytes_to_write;
    unsigned i;
    uint32_t actual_crc;

    if (dec->state == A85X_STATE_ERROR)
        return 0;
    if (dec->state != A85X_STATE_DONE || dec->crc_digits != 8)
        return a85x_decoder_fail(dec, "truncated A85X1 envelope");

    if (dec->group_total == 0) {
        if (dec->pad != 0)
            return a85x_decoder_fail(dec, "empty A85X payload cannot have padding");
    } else {
        if (!dec->have_pending)
            return a85x_decoder_fail(dec, "internal A85X pending block error");
        for (i = 4U - dec->pad; i < 4U; ++i) {
            if (dec->pending[i] != 0)
                return a85x_decoder_fail(dec, "A85X declared padding bytes are non-zero");
        }
        bytes_to_write = (size_t)(4U - dec->pad);
        if (!a85x_decoder_flush_pending(dec, bytes_to_write))
            return 0;
    }

    actual_crc = crc32_finish(dec->crc);
    if (actual_crc != dec->expected_crc)
        return a85x_decoder_fail(dec, "CRC-32 mismatch");
    return 1;
}

static void copy_stream(FILE *src, FILE *dst) {
    unsigned char buf[16384];
    size_t n;
    while ((n = fread(buf, 1, sizeof buf, src)) > 0) {
        if (fwrite(buf, 1, n, dst) != n)
            die("write error on standard output");
    }
    if (ferror(src))
        die("temporary stream read error");
}

static void a85x_decode(FILE *fp) {
    a85x_decoder dec;
    FILE *tmp = tmpfile();
    int c;

    if (tmp == NULL)
        die("could not create temporary verification stream");

    a85x_decoder_init(&dec, tmp);
    while ((c = getc(fp)) != EOF) {
        if (!a85x_decoder_feed(&dec, (unsigned char)c)) {
            const char *error = dec.error;
            fclose(tmp);
            die(error != NULL ? error : "invalid A85X1 envelope");
        }
    }
    if (ferror(fp)) {
        fclose(tmp);
        die("read error");
    }
    if (!a85x_decoder_finish(&dec)) {
        const char *error = dec.error;
        fclose(tmp);
        die(error != NULL ? error : "invalid A85X1 envelope");
    }

    if (fflush(tmp) == EOF || fseek(tmp, 0, SEEK_SET) != 0) {
        fclose(tmp);
        die("temporary verification stream seek error");
    }
    copy_stream(tmp, stdout);
    if (fclose(tmp) != 0)
        die("temporary verification stream close error");
