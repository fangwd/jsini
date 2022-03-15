/*
 * Copyright (c) Weidong Fang
 */

#ifndef _JSH_H_
#define _JSH_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

typedef struct jsh_t jsh_t;

jsh_t *jsh_create_simple(uint32_t size, int caseless);
jsh_t *jsh_create(uint32_t size, uint32_t (*hasher) (const void *),
        int (*tester) (const void *, const void *), float max_full);
jsh_t *jsh_create2(uint32_t size, uint32_t (*hasher) (const void *, void*),
        int (*tester) (const void *, const void *, void*), void*, float);
void jsh_clear(jsh_t *);
void jsh_destroy(jsh_t *);
typedef void (*jsh_free_entry)(void *, void *);
void jsh_free_ex(jsh_t *, jsh_free_entry);

int jsh_put(jsh_t *, const void *key, const void *value);
void *jsh_get(jsh_t *, const void *key);
int jsh_exists(jsh_t *, const void *key);
int jsh_remove(jsh_t *, const void *key);

uint32_t jsh_count(jsh_t *);

typedef struct {
    const void  *key;
    const void  *value;
} jsh_iterator_t;

const jsh_iterator_t *jsh_find(jsh_t*, const void *key);

const jsh_iterator_t *jsh_first(jsh_t*);
const jsh_iterator_t *jsh_next(jsh_t*, const jsh_iterator_t*);

static inline uint32_t jsh_hash_pointer(const void *p) { return (uint32_t)((uintptr_t)p * 2654435761); }
static inline int jsh_test_pointer(const void *p1, const void *p2) { return p1 == p2; }
#define jsh_create_pointer_map(n) jsh_create((n), jsh_hash_pointer, jsh_test_pointer, 0.7);

#define jsh_get_int(ht,key) ((int)(intptr_t)jsh_get(ht,key))
#define jsh_put_int(ht,key,val) jsh_put(ht,key,(void*)(intptr_t)(val))
#define jsh_free jsh_destroy

#ifdef __cplusplus
}
#endif

#endif // _JSH_H_
