/*
 * Copyright (c) 2014 Weidong Fang
 */

#include "jsh.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAX_DUMP 4096

typedef struct TestCase {
    const char *key;
    const char *value;
    const char *dump;
} TestCase;

int jsh_resize(jsh_t *t, uint32_t size);
void jsh_dump(jsh_t *t, char *buf, int length);

static uint32_t test_hasher (const void *k);

static TestCase test_cases_1[] = {
  {
    .key   = "K-1",
    .value = "V-1",
    .dump  = "(1,1,-1)",
  },
  {
    .key   = "K-5",
    .value = "V-5",
    .dump  = "(1,1,3)(3,5,-1)",
  },
  {
    .key   = "K-17",
    .value = "V-17",
    .dump  = "(1,1,2)(2,17,3)(3,5,-1)",
  },
  {
    .key   = "K-2",
    .value = "V-2",
    .dump  = "(0,17,3)(1,1,0)(2,2,-1)(3,5,-1)",
  },
  {
    .key   = NULL,
    .value = NULL,
  },
};

/* After resizing, (0,17,3)(1,1,0)(2,2,-1)(3,5,-1) becomes
 *
 *    (1,17,-1)
 * => (1,17,7)(7,1,-1)
 * => (1,17,7)(2,2,-1)(7,1,-1)
 * => (1,17,7)(2,2,-1)(5,5,-1)(7,1,-1)
 *
 * Then we have a key with hash 9, which should be chained into slot 1, and
 * the result should then be
 *
 * (1,17,6)(2,2,-1)(5,5,-1)(6,9,7)(7,1,-1)
 */
static TestCase test_cases_2[] = {
  {
    .key   = "K-9",
    .value = "V-9",
    .dump  = "(1,17,6)(2,2,-1)(5,5,-1)(6,9,7)(7,1,-1)",
  },
  {
    .key   = "K-25",
    .value = "V-25",
    .dump  = "(1,17,4)(2,2,-1)(4,25,6)(5,5,-1)(6,9,7)(7,1,-1)",
  },
  {
    .key   = "K-33",
    .value = "V-33",
    .dump  = "(1,17,3)(2,2,-1)(3,33,4)(4,25,6)(5,5,-1)(6,9,7)(7,1,-1)",
  },
  {
    .key   = NULL,
    .value = NULL,
  },
};

/* removal */
static TestCase test_cases_3[] = {
  {
    .key   = "K-2",     /* single node chain */
    .dump  = "(1,17,3)(3,33,4)(4,25,6)(5,5,-1)(6,9,7)(7,1,-1)",
  },
  {
    .key   = "K-1",     /* a tail */
    .dump  = "(1,17,3)(3,33,4)(4,25,6)(5,5,-1)(6,9,-1)",
  },
  {
    .key   = "K-25",    /* an inner node */
    .dump  = "(1,17,3)(3,33,6)(5,5,-1)(6,9,-1)",
  },
  {
    .key   = "K-17",    /* a head */
    .dump  = "(1,33,6)(5,5,-1)(6,9,-1)",
  },
  {
    .key   = "K-9",    /* another tail */
    .dump  = "(1,33,-1)(5,5,-1)",
  },
  {
    .key   = "K-5",    /* remove the remaining ... */
    .dump  = "(1,33,-1)",
  },
  {
    .key   = "K-33",
    .dump  = "",
  },
  {
    .key   = NULL,
    .value = NULL,
  },
};

static TestCase test_cases_4[] = {
  {
    .key   = "K-1",
    .value = "V-1",
    .dump  = "(1,1,-1)",
  },
  {
    .key   = "K-5",
    .value = "V-5",
    .dump  = "(1,1,3)(3,5,-1)",
  },
  {
    .key   = "K-17",
    .value = "V-17",
    .dump  = "(1,1,2)(2,17,3)(3,5,-1)",
  },
  {
    .key   = NULL,
    .value = NULL,
  },
};

/* After resizing, (1,1,2)(2,17,3)(3,5,-1) becomes
 *    (1,1,-1)
 * => (1,1,7)(7,17,-1)
 * => (1,1,7)(5,5,-1)(7,17,-1)
 */
static TestCase test_cases_5[] = {
  {
    .key   = "K-9",
    .value = "V-9",
    .dump  = "(1,1,6)(5,5,-1)(6,9,7)(7,17,-1)",
  },
  {
    .key   = "K-21",
    .value = "V-21",
    .dump  = "(1,1,6)(4,21,-1)(5,5,4)(6,9,7)(7,17,-1)",
  },
  {
    .key   = NULL,
    .value = NULL,
  },
};

/* removal */
static TestCase test_cases_6[] = {
  {
    .key   = "K-5",     /* single node chain */
    .dump  = "(1,1,6)(5,21,-1)(6,9,7)(7,17,-1)",
  },
  {
    .key   = NULL,
    .value = NULL,
  },
};

/* resize */
const char *resize_6_dump = "(1,1,-1)(3,21,4)(4,9,-1)(5,17,-1)";

char buf[MAX_DUMP];

static void test__insert(jsh_t *tiny, TestCase *tc, const char *label) {
    while (tc->key) {
        jsh_put(tiny, (const void *) tc->key, (const void *) tc->value);
        jsh_dump(tiny, buf, MAX_DUMP);
        if (strcmp(buf, tc->dump)) {
            fprintf(stderr, "FAILED %s (%s):\nExpected: %s\n  Output: %s\n",
                    __func__, label, tc->dump, buf);
            exit(1);
        }
        tc++;
    }

    printf("PASSED: %s (%s)\n", __func__, label);
}

static void test__remove(jsh_t *tiny, TestCase *tc, const char *label) {
    while (tc->key) {
        jsh_remove(tiny, (const void *) tc->key);
        jsh_dump(tiny, buf, MAX_DUMP);
        if (strcmp(buf, tc->dump)) {
            fprintf(stderr, "FAILED %s (%s):\nExpected: %s\n  Output: %s\n",
                    __func__, label, tc->dump, buf);
            exit(1);
        }
        tc++;
    }

    printf("PASSED: %s (%s)\n", __func__, label);
}

static void test__get(jsh_t *tiny, TestCase *tc, const char *label) {
    while (tc->key) {
        void * value = jsh_get(tiny, (const void *) tc->key);
        if (strcmp((const char *) value, tc->value)) {
            fprintf(stderr, "FAILED %s (%s):\nExpected: %s\n  Output: %s\n",
                    __func__, label, tc->value, (const char *) value);
            exit(1);
        }
        tc++;
    }

    printf("PASSED: %s (%s)\n", __func__, label);
}

static void test__resize(jsh_t *tiny, uint32_t size,
        const char *dump, const char *label)
{
    uint32_t count = jsh_count(tiny);

    jsh_resize(tiny, size);
    jsh_dump(tiny, buf, MAX_DUMP);
    if (strcmp(buf, dump)) {
        fprintf(stderr, "FAILED %s (%s):\nExpected: %s\n  Output: %s\n",
                __func__, label, dump, buf);
        exit(1);
    }

    if (count != jsh_count(tiny)) {
        fprintf(stderr, "FAILED %s (%s): %u != %u\n",
                __func__, label, count, jsh_count(tiny));
        exit(1);
    }

    printf("PASSED: %s (%s)\n", __func__, label);
}

static void print_entries(jsh_t *tiny) {
    const jsh_iterator_t *it = jsh_first(tiny);
    printf("\nNumber of entries: %d\n", jsh_count(tiny));
    printf("----------------\n");
    while (it) {
        printf("%s %s\n", (const char *) it->key, (const char *) it->value);
        it = jsh_next(tiny, it);
    }
    printf("----------------\n");
}

static void test__iterate() {
    jsh_t *tiny = jsh_create(4, test_hasher, NULL, 0);

    jsh_put(tiny, (const void *) "k-1", "v-1");
    jsh_put(tiny, (const void *) "k-2", "v-2");
    jsh_put(tiny, (const void *) "k-3", "v-3");
    jsh_put(tiny, (const void *) "k-4", "v-4");

    print_entries(tiny);

    jsh_clear(tiny);

    jsh_put(tiny, (const void *) "k-1", "v-1");
    jsh_put(tiny, (const void *) "k-2", "v-2");
    print_entries(tiny);

    jsh_clear(tiny);

    jsh_put(tiny, (const void *) "k-3", "v-3");
    jsh_put(tiny, (const void *) "k-4", "v-4");
    jsh_put(tiny, (const void *) "k-5", "v-5");
    jsh_put(tiny, (const void *) "k-6", "v-6");
    jsh_put(tiny, (const void *) "k-7", "v-7");
    jsh_put(tiny, (const void *) "K-7", "value-7");
    jsh_put(tiny, (const void *) "X-7", "value-7");
    jsh_put(tiny, (const void *) "k-8", "v-8");
    print_entries(tiny);

    jsh_free(tiny);
}

static void test__create2();

void test_jsh() {
    jsh_t *tiny = jsh_create(4, test_hasher, NULL, 0);
    test__insert(tiny, test_cases_1, "test_cases_1");
    test__insert(tiny, test_cases_2, "test_cases_2");
    test__get(tiny, test_cases_1, "test_cases_1");
    test__get(tiny, test_cases_2, "test_cases_2");
    assert(jsh_get(tiny, "K-100") == NULL);
    assert(jsh_get(tiny, "K-41") == NULL);
    test__remove(tiny, test_cases_3, "test_cases_3");
    jsh_destroy(tiny);

    tiny = jsh_create(4, test_hasher, NULL, 0.75);
    test__insert(tiny, test_cases_4, "test_cases_4");
    test__insert(tiny, test_cases_5, "test_cases_5");
    test__remove(tiny, test_cases_6, "test_cases_6");
    test__resize(tiny, 6, resize_6_dump, "resize_6_dump");
    assert(jsh_remove(tiny, "K-41") != 0);
    assert(jsh_remove(tiny, "X-21") != 0);
    assert(jsh_remove(tiny, "K-21") == 0);
    assert(jsh_count(tiny) == 3);
    jsh_destroy(tiny);

    test__iterate();

    test__create2();
}

/**
 * Returns the first unsigned integer in K
 */
static uint32_t test_hasher (const void *k) {
    const char *s = (const char *) k;
    uint32_t n;

    while (*s && !isdigit(*s)) {
        s++;
    }

    if (!isdigit(*s)) return (uint32_t) -1;

    for (n = 0; *s && isdigit(*s); s++) {
        n = n * 10 + (*s - '0');
    }

    return n;
}

static uint32_t hash_array_elems (const void *key, void *arg) {
  const int* arr = (int*) key;
  const int* idx = (int*) arg;
  return (uint32_t) arr[idx[0]] + arr[idx[1]];
}

static int test_array_elems(const void *k1, const void *k2, void *arg) {
  const int* arr1 = (int*) k1;
  const int* arr2 = (int*) k2;
  const int* idx = (int*) arg;
  return arr1[idx[0]] == arr2[idx[0]] && arr1[idx[1]] == arr2[idx[1]];
}

static void test__create2() {
  static int elems[2] = { 0, 2 };
  jsh_t *ht = jsh_create2(10, hash_array_elems, test_array_elems, elems, .75);
  static int data[5][3] = {
    {1, 2, 3},
    {0, 1, 4},
    {2, 3, 0},
    {1, 8, 1},
    {0, 9, 2}
  };
  static int key[3] = {1, 0, 1};
  for (int i = 0; i < 5; i++) {
    jsh_put(ht, data[i], data[i]);
  }
  int *value = jsh_get(ht, key);
  assert(value[1] == 8);
  jsh_destroy(ht);
}

