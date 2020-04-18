/*
 * Copyright (c) Weidong Fang
 */

#ifndef _JSB_H_
#define _JSB_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdarg.h>

#define JSB_OK     0
#define JSB_ERROR -1

typedef struct {
    char *data;
    size_t size;
    size_t alloc_size;
} jsb_t;

jsb_t *jsb_create();
void   jsb_clean(jsb_t *);
void   jsb_free(jsb_t *);

int    jsb_alloc(jsb_t * sb, size_t size);
jsb_t *jsb_append(jsb_t *, const char *, size_t);
int    jsb_append_char(jsb_t *, const char);
jsb_t *jsb_clear(jsb_t *);
void   jsb_init(jsb_t *);
int    jsb_load(jsb_t *, const char*);
int    jsb_printf(jsb_t *, const char *, ...);
int    jsb_sql_quote(jsb_t *sb, const char *s, size_t len);
int    jsb_log_quote(jsb_t *sb, const char *s, size_t len);
int    jsb_resize(jsb_t *, size_t);
int    jsb_save(jsb_t *, const char*);
void   jsb_strip(jsb_t *);

int    jsb_equals(jsb_t*, jsb_t*);

#define jsb_space(sb) ((sb)->data + (sb)->size)
#define jsb_space_size(sb) ((sb)->alloc_size + (sb)->size)
#define jsb_last_char(sb) ((sb)->data[(sb)->size-1])
#define jsb_free_safe(sb) if (sb) jsb_free(sb)

#ifdef __cplusplus
}
#endif

#endif /* _JSB_H_ */
