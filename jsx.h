#ifndef _JSX_H_
#define _JSX_H_

#include "jsb.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct jsx_s jsx_t;

jsx_t *jsx_create(jsb_t *sb, int standalone);
void jsx_free(jsx_t *);

void jsx_node_open(jsx_t *, const char *name, size_t size);
void jsx_print_attr(jsx_t *, const char *name, size_t name_size, const char *value, size_t value_size);
void jsx_print_attr_int(jsx_t *, const char *name, size_t name_size, int value);
void jsx_print_attr_double(jsx_t *, const char *name, size_t name_size, double value);
void jsx_print_text(jsx_t *, const char *text, size_t size);
void jsx_print_int(jsx_t *, int);
void jsx_print_double(jsx_t *, double);
void jsx_node_close(jsx_t *);

#ifdef __cplusplus
}
#endif

#endif /* _JSX_H_ */
