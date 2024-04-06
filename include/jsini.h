/*
 * Copyright (c) Weidong Fang
 */

#ifndef _JSINI_H_
#define _JSINI_H_

#include "jsa.h"
#include "jsb.h"
#include "jsh.h"

#ifdef __cplusplus
extern "C" {
#endif

#define JSINI_VERSION           "0.2.1"

// Data Types
#define JSINI_TNULL             0
#define JSINI_TBOOL             1
#define JSINI_TINTEGER          2
#define JSINI_TNUMBER           3
#define JSINI_TSTRING           4
#define JSINI_TARRAY            5
#define JSINI_TOBJECT           6

#define JSINI_UNDEFINED         127

// Errors
#define JSINI_OK                0
#define JSINI_ERROR            -1
#define JSINI_ERROR_EOF        -2
#define JSINI_ERROR_ESCAPE     -3
#define JSINI_ERROR_NOT_CLOSED -4
#define JSINI_ERROR_NAME       -5
#define JSINI_ERROR_SEPARATOR  -6

// Parser options
#define JSINI_COMMENT           1

// Options
#define JSINI_PRETTY_PRINT      1
#define JSINI_SORT_KEYS         2
#define JSINI_ESCAPE_UNICODE    4
#define JSINI_PHP_EXPORT        8

#include <stdint.h>
#include <stdio.h>
#include <limits.h>

#define JSINI_VALUE_FIELDS      \
    uint8_t         type;       \
    uint8_t         lang;       \
    uint32_t        lineno;

typedef struct {
    JSINI_VALUE_FIELDS
} jsini_value_t;

typedef struct {
    JSINI_VALUE_FIELDS
    uint8_t         data;
} jsini_bool_t;

typedef struct {
    JSINI_VALUE_FIELDS
    int64_t         data;
} jsini_integer_t;

typedef struct {
    JSINI_VALUE_FIELDS
    double          data;
} jsini_number_t;

typedef struct {
    JSINI_VALUE_FIELDS
    jsb_t           data;
} jsini_string_t;

typedef struct {
    JSINI_VALUE_FIELDS
    jsa_t           data;
} jsini_array_t;

typedef struct {
    jsini_string_t *name;
    jsini_value_t  *value;
} jsini_attr_t;

typedef struct {
    JSINI_VALUE_FIELDS
    jsa_t          keys;
    jsh_t          *map;
} jsini_object_t;

// API
jsini_value_t   *jsini_alloc_undefined();
jsini_value_t   *jsini_alloc_null();
jsini_bool_t    *jsini_alloc_bool(int);
jsini_integer_t *jsini_alloc_integer(int64_t);
jsini_number_t  *jsini_alloc_number(double value);
jsini_array_t   *jsini_alloc_array();
jsini_attr_t    *jsini_alloc_attr(jsini_object_t *, jsini_string_t *);
jsini_object_t  *jsini_alloc_object();
jsini_string_t  *jsini_alloc_string(const char *data, size_t length);

void jsini_free(jsini_value_t *);
void jsini_free_array(jsini_array_t *array);
void jsini_free_object(jsini_object_t *object);
void jsini_free_string(jsini_string_t *js);

#define jsini_push(a,v) jsini_push_value(a,(jsini_value_t*)v)
#define jsini_array_size(a) ((a)->data.size)
#define jsini_array_resize(a,n) jsa_resize(&(a)->data, n)
#define jsini_aget(a,i) ((jsini_value_t*)jsa_get(&(a)->data,i))

#define jsini_aget_integer(a,i) ((jsini_integer_t*)jsini_aget(a,i))->data

void jsini_push_null(jsini_array_t *);
void jsini_push_bool(jsini_array_t*, int);
void jsini_push_integer(jsini_array_t*, int64_t);
void jsini_push_number(jsini_array_t*, double);
void jsini_push_string(jsini_array_t*, const char *, size_t);
void jsini_push_value(jsini_array_t*, jsini_value_t *);

void jsini_array_set(jsini_array_t*, uint32_t, jsini_value_t*);
void jsini_array_remove(jsini_array_t*, uint32_t);

#define jsini_object_size(o) (jsh_count((o)->map))
#define jsini_set(o,k,v) jsini_set_value(o,k,(jsini_value_t*)v)
jsini_attr_t *jsini_attr(jsini_object_t *object, const char *name);
void jsini_set_undefined(jsini_object_t *object, const char *key);
void jsini_set_null(jsini_object_t*, const char *);
void jsini_set_bool(jsini_object_t*, const char *, int);
void jsini_set_integer(jsini_object_t*, const char *, int64_t);
void jsini_set_number(jsini_object_t*, const char *, double);
jsini_object_t *jsini_set_buffer(jsini_object_t*, const char *, jsb_t *);
const char *jsini_set_string(jsini_object_t*, const char *, const char *);
jsini_value_t *jsini_set_value(jsini_object_t*, const char *, jsini_value_t*);
void jsini_remove(jsini_object_t *, const char *);

#define jsini_type(value) (((jsini_value_t*)(value))->type)
const char *jsini_type_name(uint8_t type);

jsini_value_t *jsini_get_value(jsini_object_t *object, const char *attr);
const char *jsini_get_string(jsini_object_t *object, const char *attr);
int jsini_get_integer(jsini_object_t *object, const char *attr);
jsini_object_t *jsini_get_object(jsini_object_t *object, const char *attr);
jsini_array_t *jsini_get_array(jsini_object_t *object, const char *attr);
int jsini_select_integer(const jsini_object_t *object, const char *path);
const char* jsini_select_string(const jsini_object_t *object, const char *path);

jsini_value_t *jsini_parse_string(const char *s, uint32_t len);
jsini_value_t *jsini_parse_file(const char *);
#define jsini_parse_object_file(s) ((jsini_object_t*)jsini_parse_file(s))
jsini_value_t *jsini_parse_file(const char *);
jsini_value_t *jsini_parse_file_ini(const char *);
void           jsini_print(FILE *, const jsini_value_t *, int options);
int            jsini_print_file(const char *, const jsini_value_t *, int);
jsini_value_t *jsini_select(const jsini_object_t *, const char *);
void jsini_stringify(const jsini_value_t *, jsb_t *, int options, int indent);
void jsini_write_string(jsb_t *sb, jsb_t *s, int options);

double jsini_cast_double(const jsini_value_t *js);
int jsini_cast_int(const jsini_value_t *js);

#define jsini_set_lang_bit(js,n)   (((jsini_value_t*)js)->lang |= 1 << n)
#define jsini_clear_lang_bit(js,n) (((jsini_value_t*)js)->lang &= ~(1 << n))
#define jsini_get_lang_bit(js,n)   (((jsini_value_t*)js)->lang & (1 << n))

#define jsini_iter_key(it) ((const char*)(it)->key)
#define jsini_iter_value(it) ((jsini_value_t*)((jsini_attr_t*) (it)->value)->value)
#define jsini_iter_double(it) ((jsini_number_t*)((jsini_attr_t*) (it)->value)->value)
#define jsini_iter_bool(it) ((jsini_bool_t*)((jsini_attr_t*) (it)->value)->value)
#define jsini_iter_int(it) ((jsini_integer_t*)((jsini_attr_t*) (it)->value)->value)
#define jsini_iter_string(it) ((jsini_string_t*)((jsini_attr_t*) (it)->value)->value)
#define jsini_iter_array(it) ((jsini_array_t*)((jsini_attr_t*) (it)->value)->value)
#define jsini_iter_object(it) ((jsini_object_t*)((jsini_attr_t*) (it)->value)->value)

// Internal
typedef struct {
    const char *input;
    const char *input_end;
    size_t      lineno;
    int         error;
    int         error_char;
    int         options;
} jsl_t;

void            jsl_init(jsl_t*, const char *, size_t, int);
jsini_string_t *jsl_read_attr_name(jsl_t *);
jsini_string_t *jsl_read_json_string(jsl_t *);
jsini_value_t  *jsl_read_primitive(jsl_t *lex);
void            jsl_skip_space(jsl_t *, const char *seps);
int             jsl_skip_keyword(jsl_t *, const char *, int (*is_break)(int));
int             jsl_skip_line(jsl_t *lex);
void            jsini_write_error(jsl_t *, FILE *);

// Utils
int32_t json_escape_unicode(int32_t ch, char *buffer);
int32_t json_unescape_unicode(const char *start, const char *end, int32_t *ch);
int32_t decode_utf8(const char *s, int32_t *ch);
int32_t encode_utf8(int32_t ch, char *buffer);

#ifdef __cplusplus
}
#endif

#endif
