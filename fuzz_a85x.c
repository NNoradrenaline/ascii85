#include <stddef.h>
#include <stdint.h>

#define ASCII85_NO_MAIN 1
#include "ascii85.c"

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    a85x_decoder dec;
    size_t i;

    a85x_decoder_init(&dec, NULL);
    for (i = 0; i < size; ++i) {
        if (!a85x_decoder_feed(&dec, data[i]))
            return 0;
    }
    (void)a85x_decoder_finish(&dec);
    return 0;
}
