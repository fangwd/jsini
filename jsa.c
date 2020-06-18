/*
 * Copyright (c) Weidong Fang
 */

#include "jsa.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#define xmalloc         malloc
#define xrealloc        realloc
#define xfree           free

#define JSA_MAX_SIZE    1048576
#define JSA_OK          0
#define JSA_ERROR      -1

jsa_t *jsa_create() {
    jsa_t *a = (jsa_t*)xmalloc(sizeof(jsa_t));
    jsa_init(a);
    return a;
}

void jsa_init(jsa_t *a) {
    a->size = 0;
    a->item = NULL;
    a->alloc_size = 0;
}

int jsa_alloc(jsa_t * a, uint32_t size) {
    if (a->alloc_size < size) {
        void *item;

        if ((size = ((size + 31) / 32) * 32) > JSA_MAX_SIZE) {
            return JSA_ERROR;
        }

        item = xrealloc(a->item, size * sizeof(JSA_TYPE));

        if (item == NULL ) {
            return JSA_ERROR;
        }

        a->item = (JSA_TYPE*) item;
        a->alloc_size = size;
    }

    return JSA_OK;
}

void jsa_clean(jsa_t *a) {
    if (a->alloc_size > 0) {
        xfree(a->item);
        a->item = NULL;
        a->alloc_size = 0;
        a->size = 0;
    }
}

void jsa_free(jsa_t * a) {
    jsa_clean(a);
    xfree(a);
}

void jsa_free_ex(jsa_t * a, void (*free_item)(JSA_TYPE)) {
	if (free_item) {
		uint32_t i;
		for (i = 0; i < a->size; i++) {
			(*free_item)(a->item[i]);
		}
	}
	jsa_clean(a);
    xfree(a);
}

void jsa_append(jsa_t *a, JSA_TYPE m) {
    if (a->size >= a->alloc_size) {
        jsa_alloc(a, a->size + 1);
    }
    assert (a->size < a->alloc_size);
    a->item[a->size++] = m;
}

JSA_TYPE jsa_pop(jsa_t *a) {
    assert(a->size > 0);
    return a->item[--a->size];
}

JSA_TYPE jsa_shift(jsa_t *a) {
    return jsa_remove(a, 0);
}

int jsa_resize(jsa_t *a, uint32_t size) {
    if (size > a->alloc_size) {
        if (jsa_alloc(a, size) != JSA_OK) {
        	return JSA_ERROR;
        }
        memset(a->item + a->size, 0, (size - a->size) * sizeof(a->item[0]));
    }
    else if (size < a->size) {
        memset(a->item + size, 0, (a->size - size) * sizeof(a->item[0]));
    }

    a->size = size;

    return JSA_OK;
}

void jsa_insert(jsa_t *a, uint32_t index, JSA_TYPE m) {
    if (index >= a->size) {
        jsa_resize(a, index + 1);
        a->item[index] = m;
    } else {
        memmove(&a->item[index + 1], &a->item[index],
                (a->size - index) * sizeof(a->item[0]));
        a->item[index] = m;
        a->size++;
    }
}

void jsa_set(jsa_t *a, uint32_t index, JSA_TYPE m) {
  if (index >= a->size) {
    jsa_resize(a, index + 1);
  }
  a->item[index] = m;
}

void jsa_dedup(jsa_t *a) {
    uint32_t i, j, k, n = a->size;

    for (i = 0; i < n; i++) {
        for (j = i + 1; j < n;) {
            if (a->item[j] == a->item[i]) {
                for (k = j; k < n; k++) {
                    a->item[k] = a->item[k + 1];
                }
                n--;
            } else {
                j++;
            }
        }
    }

    if (n != a->size) {
        jsa_resize(a, n);
    }
}

/**
 * Removes the element at index N.
 */
JSA_TYPE jsa_remove(jsa_t *a, uint32_t n) {
    uint32_t i;
    JSA_TYPE t;

    assert(n < a->size);

    t = a->item[n];

    for (i = n + 1; i < a->size; i++) {
        a->item[i - 1] = a->item[i];
    }

    a->size--;

    return t;
}

/**
 * Removes the first occurrence of VALUE.
 */
void jsa_remove_first(jsa_t *a, JSA_TYPE value) {
    uint32_t i;
    for (i = 0; i < a->size; i++) {
        if (a->item[i] == value) {
            uint32_t j;
            for (j = i + 1; j < a->size; j++) {
                a->item[j - 1] = a->item[j];
            }
            a->size--;
            break;
        }
    }
}

jsa_t *jsa_clear(jsa_t *a) {
    jsa_resize(a, 0);
    return a;
}

int jsa_index_of(jsa_t *a, JSA_TYPE p)
{
  uint32_t i;
  for (i = 0; i < a->size; i++) {
    if (a->item[i] == p) {
      return (int)i;
    }
  }
  return -1;
}

#if TEST_DEDUP

#include <stdio.h>

int main() {
    jsa_t *a = jsa_create();
    int i, n;

    printf("Enter array elements (-1 to stop):\n");
    while (1) {
        scanf("%d", &n);
        if (n == -1) break;
        jsa_push(a, (void*)(uintptr_t)n);
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

    return 0;
}
#endif

