#include "jsini.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

int encode_utf8(int32_t ch, char *buffer);

int main(int argc, char **argv) {
    char   *e, s[4];
    int32_t c, i;
    int32_t b, n;

    if (argc < 2) {
        printf("usage: %s number [base]\n", argv[0]);
        return 0;
    }

    b = argc > 2 ? atoi(argv[2]) : 16;

    c = strtol(argv[1], &e, b);

    if (*e) {
        fprintf(stderr, "Invalid base %d number: '%s'\n", b, argv[1]);
        return 1;
    }

    for (i = 0; i < 4; i++) s[i] = '\0';

    n = encode_utf8(c, s);

    if (n > 0) {
        //     { 0x24,     1, { 0x24, 0x00, 0x00, 0x00 } },

        printf("    { 0x%x, %d, { ", c, n);
        for (i = 0; i < 4; i++) {
            printf("0x%02x", (s[i] & 0xff));
            if (i < 3) printf(", ");
            else printf(" } },\n");
        }
    }
    else {
        printf("Invalid utf-8 code %d (%x)\n", c, c);
    }

    return 0;
}
