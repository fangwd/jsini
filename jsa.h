#ifndef _JSA_H_
#define _JSA_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#ifndef JSA_TYPE
#define JSA_TYPE uintptr_t
#endif

typedef struct {
    JSA_TYPE *item;
    uint32_t   size;
    uint32_t   alloc_size;
} jsa_t;

jsa_t *jsa_create();
void jsa_init(jsa_t *);
void jsa_clean(jsa_t *);
void jsa_free(jsa_t *);
typedef void (*jsa_free_entry) (JSA_TYPE);
void jsa_free_ex(jsa_t * a, jsa_free_entry free_item);
void jsa_append(jsa_t *, JSA_TYPE m);
JSA_TYPE jsa_pop(jsa_t *) ;
JSA_TYPE jsa_shift(jsa_t *);
void jsa_insert(jsa_t *, uint32_t, JSA_TYPE);
int jsa_alloc(jsa_t * a, uint32_t size);
int jsa_resize(jsa_t * a, uint32_t size);
void jsa_dedup(jsa_t * a);
JSA_TYPE jsa_remove(jsa_t *a, uint32_t n);
void jsa_remove_first(jsa_t *a, JSA_TYPE);

#define jsa_get(a,i) ((a)->item[i])
#define jsa_set(a, i, v) ((a)->item[i]=(JSA_TYPE)v)
#define jsa_push(a,p) jsa_append(a,(JSA_TYPE)p)
#define jsa_first(a) ((a)->size ? (a)->item[0] : 0)
#define jsa_last(a) ((a)->size ? (a)->item[(a)->size - 1] : 0)
#define jsa_size(a) ((a)->size)

#ifdef __cplusplus
}
#endif

#endif /* _JSA_H_ */

