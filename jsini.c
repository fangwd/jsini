/*
 * Copyright (c) Weidong Fang
 */

#include "jsini.h"

#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

#define xmalloc malloc
#define xfree free

jsini_array_t *jsini_alloc_array() {
    jsini_array_t *array = (jsini_array_t *) xmalloc(sizeof(jsini_array_t));
    array->base.type = JSINI_TARRAY;
    jsa_init(&array->data);
    return array;
}

void jsini_free_array(jsini_array_t *array) {
    uint32_t i;
    for (i = 0; i < array->data.size; i++) {
        jsini_free((jsini_value_t*)array->data.item[i]);
    }
    jsa_clean(&array->data);
    xfree(array);
}

jsini_attr_t *jsini_alloc_attr(jsini_object_t *object, jsb_t *name) {
    jsini_attr_t *attr = (jsini_attr_t *) xmalloc(sizeof(jsini_attr_t));
    attr->name = name;
    attr->value = NULL;
#   if JSINI_KEEP_KEYS
    jsa_push(&object->data, attr);
#   endif
    jsh_put(object->map, name->data, attr);
    return attr;
}

void jsini_free_attr(jsini_attr_t *attr) {
    jsb_free(attr->name);
    if (attr->value) {
        jsini_free(attr->value);
    }
    xfree(attr);
}

jsini_value_t *jsini_alloc_undefined() {
    jsini_value_t *value = (jsini_value_t *) xmalloc(sizeof(jsini_value_t));
    value->type = JSINI_UNDEFINED;
    value->lang = 0;
    return value;
}

jsini_value_t *jsini_alloc_null() {
    jsini_value_t *value = (jsini_value_t *) xmalloc(sizeof(jsini_value_t));
    value->type = JSINI_TNULL;
	value->lang = 0;
    return value;
}

jsini_bool_t *jsini_alloc_bool(int value) {
    jsini_bool_t *js = (jsini_bool_t *) xmalloc(sizeof(jsini_bool_t));
    js->base.type = JSINI_TBOOL;
	js->base.lang = 0;
    js->data = value;
    return js;
}

jsini_integer_t *jsini_alloc_integer(int64_t value) {
    jsini_integer_t *js = (jsini_integer_t *) xmalloc(sizeof(jsini_integer_t));
    js->base.type = JSINI_TINTEGER;
	js->base.lang = 0;
    js->data = value;
    return js;
}

jsini_number_t *jsini_alloc_number(double value) {
    jsini_number_t *js = (jsini_number_t *) xmalloc(sizeof(jsini_number_t));
    js->base.type = JSINI_TNUMBER;
	js->base.lang = 0;
    js->data = value;
    return js;
}

jsini_object_t *jsini_alloc_object() {
    jsini_object_t *object = (jsini_object_t *) xmalloc(sizeof(jsini_object_t));
    object->base.type = JSINI_TOBJECT;
	object->base.lang = 0;
#   if JSINI_KEEP_KEYS
    jsa_init(&object->data);
#   endif
    object->map = jsh_create_simple(0,0);
    return object;
}

void jsini_free_object(jsini_object_t *object) {
    jsh_t *map = object->map;
    const jsh_iterator_t *it;
    for (it = jsh_first(map); it; it = jsh_next(map, it)) {
        jsini_free_attr((jsini_attr_t*)it->value);
    }
    jsh_destroy(object->map);
#   if JSINI_KEEP_KEYS
    jsa_clean(&object->data);
#   endif
    xfree(object);
}

jsini_string_t *jsini_alloc_string(const char *data, size_t length) {
    jsini_string_t *js = (jsini_string_t *) xmalloc(sizeof(jsini_string_t));
    js->base.type = JSINI_TSTRING;
	js->base.lang = 0;
    jsb_init(&js->data);
    if (data != NULL) {
    	jsb_append(&js->data, data, length);
    }
    return js;
}

void jsini_free_string(jsini_string_t *js) {
    jsb_clean(&js->data);
    xfree(js);
}

void jsini_free(jsini_value_t *js) {
    switch(js->type) {
    case JSINI_TARRAY:
        jsini_free_array((jsini_array_t*)js);
        break;
    case JSINI_TOBJECT:
        jsini_free_object((jsini_object_t*)js);
        break;
    case JSINI_TSTRING:
        jsini_free_string((jsini_string_t*)js);
        break;
    default:
        xfree(js);
        break;
    }
}

void jsl_init(jsl_t *lex, const char *s, size_t len, int options) {
    lex->input     = s;
    lex->input_end = s + len;
    lex->lineno    = 0;
    lex->error     = JSINI_OK;
}

int jsl_is_break(int c) {
    return !isalnum(c) && c != '_';
}

jsini_number_t *jsl_read_number(jsl_t *lex) {
    char  *tok_end;
    double data = strtod(lex->input, &tok_end);

    if (tok_end != lex->input && tok_end <= lex->input_end && errno != ERANGE) {
        jsini_number_t * result;
        int is_float = 0;
        while (lex->input != tok_end) {
            if (*lex->input++ == '.') {
                is_float = 1;
                break;
            }
        }
        result = is_float ? jsini_alloc_number(data) :
                (jsini_number_t *) jsini_alloc_integer((int64_t)data);
        lex->input = tok_end;
        return result;
    }

    return NULL ;
}

jsini_value_t *jsl_read_primitive(jsl_t *lex) {
    jsini_value_t *value = NULL;

    assert(lex->input < lex->input_end);

    switch (*lex->input) {
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
    case '-':
    case '.':
        value = (jsini_value_t*) jsl_read_number(lex);
        break;
    case 't':
        if (jsl_skip_keyword(lex, "true", NULL)) {
            value = (jsini_value_t*) jsini_alloc_bool(1);
        }
        break;
    case 'f':
        if (jsl_skip_keyword(lex, "false", NULL)) {
            value = (jsini_value_t*) jsini_alloc_bool(0);
        }
        break;
    case 'n':
        if (jsl_skip_keyword(lex, "null", NULL)) {
            value = jsini_alloc_null();
        }
        break;
    default:
        break;
    }

    return value;
}

void jsini_write_error(jsl_t *lex, FILE *stream) {
    fprintf(stream, "ERROR: ");

    switch(lex->error) {
    case JSINI_ERROR_EOF:
        fprintf(stream, "Unexpected EOF");
        break;
    case JSINI_ERROR_ESCAPE:
        fprintf(stream, "Bad escape sequence");
        break;
    case JSINI_ERROR_NOT_CLOSED:
        fprintf(stream, "'%c' not closed", lex->error_char);
        break;
    case JSINI_ERROR_NAME:
        fprintf(stream, "Bad name");
        break;
    case JSINI_ERROR_SEPARATOR:
        fprintf(stream, "'%c' expected", lex->error_char);
        break;
    default:
        assert(0);
        break;
    }

    fprintf(stream, " (line %zu)\n", lex->lineno + 1);
}

int jsl_skip_keyword(jsl_t *lex, const char *keyword, int (*is_break)(int)) {
    const char *next = lex->input;

    while (*keyword && (next != lex->input_end)) {
        if (*next != *keyword) {
            return 0;
        }
        next++;
        keyword++;
    }

    if (!*keyword && (next == lex->input_end || ((is_break &&
            is_break(*next)) || jsl_is_break(*next)))) {
        lex->input = next;
        return 1;
    }

    return 0;
}

int jsl_skip_line(jsl_t *lex) {
    while (lex->input != lex->input_end) {
        if (*lex->input == '\n' ||
           (*lex->input == '\r' && lex->input[1] != '\n'))
        {
            lex->input++;
            lex->lineno++;
            break;
        }
        lex->input++;
    }
    return JSINI_OK;
}

void jsl_skip_space(jsl_t *lex, const char *seps) {
    while (lex->input < lex->input_end) {
        char c = *lex->input;
        if (c == ' ' || c == '\t') {
            lex->input++;
        }
        else if (c == '\r') {
            lex->input++;
            if (lex->input[1] != '\n') {
                lex->lineno++;
            }
        }
        else if (c == '\n') {
            lex->input++;
            lex->lineno++;
        }
        else if (seps && strchr(seps, c)) {
            lex->input++;
        }
        else {
            break;
        }
    }
}

const char *jsini_get_string(jsini_object_t *object, const char *name) {
    jsini_attr_t *attr = (jsini_attr_t *) jsh_get(object->map, name);
	if (attr) {
		jsini_string_t *js = (jsini_string_t *) attr->value;
		if (jsini_type(js) == JSINI_TNULL) {
			return NULL;
		}
		assert(jsini_type(js) == JSINI_TSTRING);
		return js->data.data;
	}
	return NULL;
}

int jsini_get_integer(jsini_object_t *object, const char *name) {
    jsini_attr_t *attr = (jsini_attr_t *) jsh_get(object->map, name);
	if (attr) {
		jsini_integer_t *js = (jsini_integer_t *) attr->value;
        if (jsini_type(js) == JSINI_TNULL) {
            return 0;
        }
		assert(jsini_type(js) == JSINI_TINTEGER);
		return js->data;
	}
	return INT_MIN;
}

jsini_value_t *jsini_get_value(jsini_object_t *object, const char *name) {
    jsini_attr_t *attr = (jsini_attr_t *) jsh_get(object->map, name);
	return attr ? attr->value : NULL;
}

jsini_object_t *jsini_get_object(jsini_object_t *object, const char *name) {
    jsini_attr_t *attr = (jsini_attr_t *) jsh_get(object->map, name);
	if (attr) {
		assert(attr->value->type == JSINI_TOBJECT);
		return (jsini_object_t *) attr->value;
	}
	return NULL;
}

jsini_array_t *jsini_get_array(jsini_object_t *object, const char *name) {
    jsini_attr_t *attr = (jsini_attr_t *) jsh_get(object->map, name);
	if (attr) {
		assert(attr->value->type == JSINI_TARRAY);
		return (jsini_array_t *) attr->value;
	}
	return NULL;
}

jsini_value_t *jsini_select(const jsini_object_t *object, const char *name) {
    jsini_attr_t *attr = NULL;
    char *buffer = strdup(name);
    char *p = buffer, c = *name;

    while (c) {
        const char *key = p;
        jsh_t *map;
        if (jsini_type(object) != JSINI_TOBJECT) {
            attr = NULL;
            break;
        }
        while (*p && *p != '.' && *p != '/') {
            p++;
        }
        if ((c = *p)) {
            *p++ = '\0';
        }
        map = ((jsini_object_t*)object)->map;
        if (!(attr = (jsini_attr_t *) jsh_get(map, key))) {
            break;
        }
        object = (jsini_object_t*)attr->value;
    }

    xfree(buffer);

    return attr ? attr->value : NULL;
}

int jsini_select_integer(const jsini_object_t *object, const char *name) {
    jsini_integer_t *js = (jsini_integer_t*)jsini_select(object, name);
	if (js != NULL) {
		assert(jsini_type(js) == JSINI_TINTEGER);
		return js->data;
	}
	return INT_MIN; 
}

const char* jsini_select_string(const jsini_object_t *object, const char *name) {
    jsini_string_t *js = (jsini_string_t*)jsini_select(object, name);
	if (js != NULL) {
		assert(jsini_type(js) == JSINI_TSTRING);
		return js->data.data;
	}
	return NULL;
}

// API
void jsini_push_null(jsini_array_t *array) {
    jsa_push(&array->data, jsini_alloc_null());
}

void jsini_push_bool(jsini_array_t *array, int value) {
    jsa_push(&array->data, jsini_alloc_bool(value));
}

void jsini_push_integer(jsini_array_t *array, int64_t value) {
    jsa_push(&array->data, jsini_alloc_integer(value));
}

void jsini_push_number(jsini_array_t *array, double value) {
    jsa_push(&array->data, jsini_alloc_number(value));
}

void jsini_push_string(jsini_array_t *array, const char *data, size_t len) {
    jsini_string_t *s = jsini_alloc_string(data, len);
    jsa_push(&array->data, s);
}

jsini_attr_t *jsini_attr(jsini_object_t *object, const char *name) {
    jsini_attr_t *attr = (jsini_attr_t *) jsh_get(object->map, name);
    if (attr) {
        if (attr->value) jsini_free(attr->value);
        attr->value = NULL;
    }
    else {
        jsb_t *attr_name = jsb_create();
        jsb_append(attr_name, name, strlen(name));
        return jsini_alloc_attr(object, attr_name);
    }
    return attr;
}

void jsini_push_value(jsini_array_t *array, jsini_value_t *value) {
    jsa_push(&array->data, value);
}

void jsini_array_set(jsini_array_t *array, uint32_t key, jsini_value_t *value) {
    jsini_value_t *old_value = (jsini_value_t*)jsa_get(&array->data, key);
    if (old_value) {
        jsini_free(old_value);
    }
    jsa_set(&array->data, key, (JSA_TYPE) value);
}

void jsini_array_remove(jsini_array_t*array, uint32_t key) {
    jsini_value_t *value = (jsini_value_t*) jsa_get(&array->data, key);
    if (value) {
        jsini_free(value);
    }
    jsa_remove(&array->data, key);
}

void jsini_set_undefined(jsini_object_t *object, const char *key) {
    jsini_attr(object, key)->value = (jsini_value_t*) jsini_alloc_undefined();
}

void jsini_set_null(jsini_object_t *object, const char *key) {
    jsini_attr(object, key)->value = (jsini_value_t*) jsini_alloc_null();
}

void jsini_set_bool(jsini_object_t *object, const char *key, int value) {
    jsini_attr(object, key)->value = (jsini_value_t*) jsini_alloc_bool(value);
}

void jsini_set_integer(jsini_object_t *object, const char *key, int64_t value) {
    jsini_attr(object, key)->value = (jsini_value_t*) jsini_alloc_integer(value);
}

void jsini_set_number(jsini_object_t *object, const char *key, double value) {
    jsini_attr(object, key)->value = (jsini_value_t*) jsini_alloc_number(value);
}

jsini_object_t *jsini_set_buffer(jsini_object_t *object, const char *key, jsb_t *sb) {
    jsini_attr_t *attr = jsini_attr(object, key);
    if (attr->value) {
        jsini_free(attr->value);
    }
    if (sb != NULL) {
        jsini_string_t *js = jsini_alloc_string(sb->data, sb->size);
        attr->value = (jsini_value_t*) js;
    }
    else {
        jsini_set_null(object, key);
    }
    return object;
}

const char *jsini_set_string(jsini_object_t *object, const char *key,
        const char *s) {
    jsini_attr_t *attr = jsini_attr(object, key);
    if (attr->value) {
        jsini_free(attr->value);
    }

    if (s != NULL) {
        jsini_string_t *js = jsini_alloc_string(s, strlen(s));
        attr->value = (jsini_value_t*) js;
        return js->data.data;
    }
    else {
        // warning?
        jsini_set_null(object, key);
        return NULL;
    }
}

jsini_value_t *jsini_set_value(jsini_object_t *object, const char *key,
        jsini_value_t *value) {
    jsini_attr(object, key)->value = value;
    return value;
}

void jsini_remove(jsini_object_t *object, const char *key) {
    jsini_attr_t *attr = (jsini_attr_t *) jsh_get(object->map, key);
    if (attr) {
        jsh_remove(object->map, key);
#       if JSINI_KEEP_KEYS
        jsa_remove_first(&object->data, (JSA_TYPE) attr);
#       endif
        jsini_free_attr(attr);
    }
}

double jsini_cast_double(const jsini_value_t *js) {
    switch (js->type) {
    case JSINI_TINTEGER:
        return ((jsini_integer_t*) js)->data;
    case JSINI_TNUMBER:
        return ((jsini_number_t*) js)->data;
    default:
        return 0;
    }
}

int jsini_cast_int(const jsini_value_t *js) {
    switch (js->type) {
    case JSINI_TINTEGER:
        return ((jsini_integer_t*) js)->data;
    case JSINI_TNUMBER:
        return ((jsini_number_t*) js)->data;
    default:
        return 0;
    }
}

const char *jsini_type_name(uint8_t type) {
    switch (type) {
    case JSINI_TNULL:
        return "null";
    case JSINI_TBOOL:
        return "bool";
    case JSINI_TINTEGER:
        return "integer";
    case JSINI_TNUMBER:
        return "number";
    case JSINI_TSTRING:
        return "string";
    case JSINI_TARRAY:
        return "array";
    case JSINI_TOBJECT:
        return "object";
    case JSINI_UNDEFINED:
        return "undefined";
    default:
        return NULL;
    }
}

