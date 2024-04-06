#include "jsa.h"
#include <stdio.h>

void test_jsa() {
    jsa_t *a = jsa_create();
    int i, n;

    printf("Enter array elements (-1 to stop):\n");
    while (1) {
        scanf("%d", &n);
        if (n == -1) break;
        jsa_push(a, (void *)(uintptr_t)n);
    }

    for (i = 0; i < (int)jsa_size(a); i++) {
        printf("%lu ", (unsigned long)jsa_get(a, i));
    }

    printf("\n");

    jsa_dedup(a);

    for (i = 0; i < (int)jsa_size(a); i++) {
        printf("%lu ", (unsigned long)jsa_get(a, i));
    }

    printf("\n");

    jsa_free(a);
}