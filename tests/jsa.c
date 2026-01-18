#include "jsa.h"
#include <stdio.h>
#include <assert.h>
#include <string.h>

// Mock cleanup function for jsa_free_ex
static int cleanup_called = 0;
void mock_cleanup(uintptr_t item)
{
    cleanup_called++;
}

void test_jsa()
{
    printf("Testing JSA...\n");

    // 1. Basic Create & Push
    jsa_t *a = jsa_create();
    assert(a != NULL);
    assert(jsa_size(a) == 0);

    jsa_push(a, (void *)(uintptr_t)10);
    jsa_push(a, (void *)(uintptr_t)20);
    jsa_push(a, (void *)(uintptr_t)30);

    assert(jsa_size(a) == 3);
    assert(jsa_get(a, 0) == 10);
    assert(jsa_get(a, 1) == 20);
    assert(jsa_get(a, 2) == 30);
    assert(jsa_first(a) == 10);
    assert(jsa_last(a) == 30);

    // 2. Set & Insert
    // jsa_set extending size
    jsa_set(a, 4, 50); // Index 3 becomes 0 (implicit), Index 4 becomes 50
    assert(jsa_size(a) == 5);
    assert(jsa_get(a, 3) == 0);
    assert(jsa_get(a, 4) == 50);

    // jsa_insert in middle
    jsa_insert(a, 1, 15); // [10, 15, 20, 30, 0, 50]
    assert(jsa_size(a) == 6);
    assert(jsa_get(a, 0) == 10);
    assert(jsa_get(a, 1) == 15);
    assert(jsa_get(a, 2) == 20);

    // jsa_insert at end (extends)
    jsa_insert(a, 6, 60);
    assert(jsa_size(a) == 7);
    assert(jsa_get(a, 6) == 60);

    // jsa_insert beyond end (extends)
    jsa_insert(a, 8, 80); // index 7=0, index 8=80
    assert(jsa_size(a) == 9);
    assert(jsa_get(a, 7) == 0);
    assert(jsa_get(a, 8) == 80);

    // 3. Pop & Shift
    uintptr_t val = jsa_pop(a);
    assert(val == 80);
    assert(jsa_size(a) == 8);

    val = jsa_shift(a); // removes 10
    assert(val == 10);
    assert(jsa_size(a) == 7);
    assert(jsa_get(a, 0) == 15);

    // 4. Remove & Remove First
    // Current: [15, 20, 30, 0, 50, 60, 0]
    jsa_remove(a, 2); // Removes 30
    assert(jsa_size(a) == 6);
    assert(jsa_get(a, 2) == 0);

    // Add duplicates for remove_first
    jsa_push(a, 100);
    jsa_push(a, 100);
    // Current: [15, 20, 0, 50, 60, 0, 100, 100]
    jsa_remove_first(a, 100);
    assert(jsa_size(a) == 7);
    assert(jsa_get(a, 6) == 100); // One 100 remains

    // 5. Index Of
    assert(jsa_index_of(a, 50) == 3);
    assert(jsa_index_of(a, 999) == -1);

    // 6. Dedup
    jsa_clean(a); // Reset
    assert(jsa_size(a) == 0);

    jsa_push(a, 1);
    jsa_push(a, 2);
    jsa_push(a, 2);
    jsa_push(a, 3);
    jsa_push(a, 3);
    jsa_push(a, 3);

    jsa_dedup(a);
    assert(jsa_size(a) == 3);
    assert(jsa_get(a, 0) == 1);
    assert(jsa_get(a, 1) == 2);
    assert(jsa_get(a, 2) == 3);

    jsa_free(a);

    // 7. Test Free Ex
    a = jsa_create();
    jsa_push(a, 1);
    jsa_push(a, 2);
    cleanup_called = 0;
    jsa_free_ex(a, mock_cleanup);
    assert(cleanup_called == 2);

    printf("JSA Tests Passed.\n");
}
