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
void jsh_clear(jsh_t *);
void jsh_destroy(jsh_t *);
void jsh_free_ex(jsh_t *, void (*free_item)(void *, void *));

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

#define jsh_get_int(ht,key) ((int)(intptr_t)jsh_get(ht,key))
#define jsh_put_int(ht,key,val) jsh_put(ht,key,(void*)(intptr_t)(val))
#define jsh_free jsh_destroy

#ifdef __cplusplus
}
#endif

#endif // _JSH_H_
