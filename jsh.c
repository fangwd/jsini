/*
 * Copyright (c) Weidong Fang
 */

#include "jsh.h"

#include <assert.h>
#include <ctype.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
typedef unsigned __int32 u_int32_t;
typedef unsigned __int32 uint32_t;
#define strcasecmp _stricmp
#endif

typedef uint32_t HASH;

typedef struct Slot {
    const void  *key;
    const void  *value;
    struct Slot *next;
    HASH         hash;
    uint8_t      taken;
} Slot;

typedef HASH    (*Hasher) (const void *);
typedef int     (*Tester) (const void *, const void *);

typedef HASH    (*Hasher2) (const void *, void*);
typedef int     (*Tester2) (const void *, const void *, void*);

struct jsh_t {
    Slot *slot;
    Slot *free_slot;

    uint32_t size;
    uint32_t count;

    float    max_full;
    uint32_t max_count;

    Hasher hasher;
    Tester tester;
    void  *arg;
};

#define TINY_OK         0
#define TINY_ERROR     -1
#define TINY_SIZE       64          /* default size */
#define TINY_MAX        1073741824  /* maximum size */
#define HOME_OF(t,h)    (&(t)->slot[(h) % (t)->size])

#define xmalloc         malloc
#define xfree           free

int jsh_resize(jsh_t *, uint32_t);

static Slot *jsh_locate(jsh_t *, HASH, const void *);
static int jsh_test(jsh_t *, Slot *, HASH, const void *);

HASH jsh_hash_cstr(const void *key);
int  jsh_cmp_cstr(const void *s1, const void *s2);

static HASH jsh_string_hasher_caseless(const void *key);
static int  jsh_string_tester_caseless(const void *s1, const void *s2);

uint32_t
jsh_count(jsh_t *t)
{
    return t->count;
}

jsh_t *
jsh_create(uint32_t size, Hasher hasher, Tester tester, float max_full)
{
    jsh_t *t;

    t = (jsh_t *) xmalloc (sizeof (jsh_t));

    if (t == NULL) {
        /* out of memory */
        return NULL;
    }

    memset(t, 0, sizeof(*t));

    t->max_full = max_full > 0.0 ? max_full : 1.0;

    if (jsh_resize(t, size) != TINY_OK) {
        xfree(t);
        return NULL;
    }

    t->hasher = hasher;
    t->tester = tester;
    t->arg = NULL;

    return t;
}

jsh_t *
jsh_create2(uint32_t n, Hasher2 h, Tester2 t, void* a, float f)
{
    jsh_t *ht = jsh_create(n, (Hasher) h, (Tester) t, f);
    ht->arg = a;
    return ht;
}

jsh_t *
jsh_create_simple(uint32_t size, int caseless)
{
    if (caseless) {
        return jsh_create(size, jsh_string_hasher_caseless,
                jsh_string_tester_caseless, 0.75);
    }
    else {
        return jsh_create(size, jsh_hash_cstr,
                jsh_cmp_cstr, 0.75);
    }
}

void
jsh_destroy(jsh_t *t)
{
    xfree(t->slot);
    xfree(t);
}

void
jsh_clear(jsh_t *t)
{
    if (t->count > 0) {
        memset(t->slot, 0, t->size * sizeof(Slot));
        t->free_slot = t->slot + t->size;
        t->count = 0;
    }
}

#define hash_key(t,k) ((t)->arg ? \
    (((Hasher2)(t)->hasher)(k, (t)->arg)) : (t)->hasher(k))

int
jsh_exists(jsh_t *t, const void *key)
{
    HASH  h = hash_key(t, key);
    Slot *n = jsh_locate(t, h, key);
    return n ? 1 : 0;
}

static Slot *
jsh_locate(jsh_t *t, HASH  h, const void *k)
{
    Slot *m = HOME_OF(t, h);

    while (m && m->taken) {
        if (jsh_test(t, m, h, k)) {
            return m;
        }
        else {
            m = m->next;
        }
    }

    return NULL;
}

void *jsh_get(jsh_t *t, const void *key) {
    HASH  h = hash_key(t, key);
    Slot *n = jsh_locate(t, h, key);
    return n ? (void *) n->value : NULL;
}

const jsh_iterator_t *jsh_find(jsh_t *t, const void *key) {
    HASH  h = hash_key(t, key);
    return (jsh_iterator_t*)jsh_locate(t, h, key);
}

static int
jsh_insert(jsh_t *t, HASH h, const void *key, const void *value)
{
    Slot *m;

    if (t->count >= t->max_count) {
        if (jsh_resize(t, 2 * t->size) != TINY_OK) {
            return TINY_ERROR;
        }
    }

    m = HOME_OF(t, h);

    if (m->taken) {
        Slot *n  = NULL;
        Slot *mm = HOME_OF(t, m->hash);

        for (n = NULL; t->free_slot > t->slot;) {
            n = --t->free_slot;
            if (!n->taken) {
                break;
            }
        }

        assert(n && !n->taken);

        if (mm == m) {
            n->next = m->next;
            m->next = n;
            n->taken = 1;
            m = n;
        } else {
            while (mm->next != m) {
                mm = mm->next;
            }
            mm->next = n;
            *n = *m;
            m->next = NULL;
        }
    }
    else {
        m->next  = NULL;
        m->taken = 1;
    }

    m->key   = key;
    m->value = value;
    m->hash  = h;

    t->count++;

    return TINY_OK;
}

static void
jsh_reclaim(jsh_t *t, Slot *n)
{
    memset(n, 0, sizeof(*n));
    if (n >= t->free_slot) {
        t->free_slot = n + 1;
    }
}

int
jsh_remove(jsh_t *t, const void *k)
{
    HASH  h = hash_key(t, k);
    Slot *m = HOME_OF(t, h);    /* tortoise */
    Slot *n = m;                /* hare */

    do {                        /* jsh_locate */
        if (jsh_test(t, n, h, k)) {
            break;
        }
        else {
            m = n;
            n = n->next;
        }
    } while (n);

    if (!n) {
        return TINY_ERROR;
    }

    if (m == n) {
        if (m->next) {
            Slot *p = m->next;
            *m = *m->next;
            jsh_reclaim(t, p);
        }
        else {
            jsh_reclaim(t, n);
        }
    }
    else {
        m->next = n->next;
        jsh_reclaim(t, n);
    }

    assert(t->count > 0);

    t->count--;

    return TINY_OK;
}

int
jsh_resize(jsh_t *t, uint32_t size)
{
    uint32_t old_size;
    Slot    *old_slot;
    uint32_t i;

    if (size > TINY_MAX) {
        /* too big */
        return TINY_ERROR;
    }

    if (size == 0) {
        size = TINY_SIZE;
    }

    old_size = t->size;
    old_slot = t->slot;

    t->slot = (Slot *) xmalloc(size * sizeof(Slot));

    if (t->slot == NULL) {
        /* out of memory */
        t->slot = old_slot;
        return -1;
    }

    memset(t->slot, 0, size * sizeof(Slot));

    t->size = size;
    t->free_slot = t->slot + size;
    t->count = 0;
    t->max_count = size * t->max_full;

    for (i = 0; i < old_size; i++) {
        Slot *slot = old_slot + i;
        if (slot->taken) {
            jsh_insert(t, slot->hash, slot->key, slot->value);
        }
    }

    xfree(old_slot);

    return TINY_OK;
}

int
jsh_put(jsh_t *t, const void *key, const void *value) {
    HASH  h = hash_key(t, key);
    Slot *n = jsh_locate(t, h, key);

    if (n) {
        n->value = value;
        return TINY_OK;
    }

    return jsh_insert(t, h, key, value);
}

static int
jsh_test(jsh_t *t, Slot *slot, HASH hash, const void *key)
{
    if (slot->hash != hash) {
        return 0;
    }
    return t->tester
        ? t->arg
            ? ((Tester2)t->tester)(slot->key, key, t->arg)
            : t->tester(slot->key, key)
        : slot->key == key;
}

int jsh_cmp_cstr(const void *s1, const void *s2) {
    return !strcmp((const char *) s1, (const char *) s2);
}

static int jsh_string_tester_caseless(const void *s1, const void *s2) {
    return !strcasecmp((const char *) s1, (const char *) s2);
}

/**
 * 32-bit FNV1-1a algorithm
 */
HASH jsh_hash_cstr(const void *key) {
    unsigned char *s = (unsigned char *) key;
    HASH h = 2166136261;
    while (*s) {
        h ^= (u_int32_t) *s++;
        h *= (HASH) 0x01000193;
    }
    return h;
}

/**
 * 32-bit FNV1-1a algorithm (caseless)
 */
static HASH jsh_string_hasher_caseless(const void *key) {
    unsigned char *s = (unsigned char *) key;
    HASH h = 2166136261;
    while (*s) {
        char c;
        c = tolower(*s++);
        h ^= (u_int32_t) c;
        h *= (HASH) 0x01000193;
    }
    return h;
}

const jsh_iterator_t *jsh_first(jsh_t *t) {
    Slot *slot = t->slot;
    while (slot < t->slot + t->size && !slot->taken) {
        slot++;
    }
    if (slot < t->slot + t->size) {
        return (const jsh_iterator_t *) slot;
    }
    return NULL;
}

const jsh_iterator_t *jsh_next(jsh_t *t, const jsh_iterator_t *p) {
    Slot *slot = (Slot *) p;

    ++slot;

    while (slot < t->slot + t->size && !slot->taken) {
        slot++;
    }

    if (slot < t->slot + t->size) {
        return (const jsh_iterator_t *) slot;
    }

    return NULL;
}

void jsh_free_ex(jsh_t *t, void (*free_item)(void *, void *)) {
	if (free_item) {
		Slot *slot = t->slot;
		while (slot < t->slot + t->size) {
			if (slot->taken) {
				free_item((void*)slot->key, (void*)slot->value);
			}
			slot++;
		}
	}
	jsh_destroy(t);
}

#ifdef TEST_JSH
#include <stdio.h>
void jsh_dump(jsh_t *t, char *buf, int length) {
    uint32_t i, so;
    *buf = '\0';
    for (i = 0, so = 0; i < t->size; i++) {
        Slot *slot = &t->slot[i];
        if (slot->taken) {
            so += snprintf(buf + so, length - so, "(%d,%d,%d)", i,
                    slot->hash, slot->next ? (int)(slot->next - t->slot) : -1);
        }
    }
}
#endif
