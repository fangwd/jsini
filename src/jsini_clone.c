#include "jsini_clone.h"
#include "jsh.h"
#include <string.h>

static jsini_value_t* clone_object(const jsini_object_t* obj);
static jsini_value_t* clone_array(const jsini_array_t* array);

jsini_value_t* jsini_clone(const jsini_value_t* value) {
    if (!value) {
        return NULL;
    }

    switch (value->type) {
        case JSINI_TNULL:
            return (jsini_value_t*)jsini_alloc_null();
        case JSINI_TBOOL:
            return (jsini_value_t*)jsini_alloc_bool(((jsini_bool_t*)value)->data);
        case JSINI_TINTEGER:
            return (jsini_value_t*)jsini_alloc_integer(((jsini_integer_t*)value)->data);
        case JSINI_TNUMBER:
            return (jsini_value_t*)jsini_alloc_number(((jsini_number_t*)value)->data);
        case JSINI_TSTRING: {
            jsini_string_t* str = (jsini_string_t*)value;
            return (jsini_value_t*)jsini_alloc_string(str->data.data, str->data.size);
        }
        case JSINI_TARRAY:
            return clone_array((const jsini_array_t*)value);
        case JSINI_TOBJECT:
            return clone_object((const jsini_object_t*)value);
        case JSINI_UNDEFINED:
            return jsini_alloc_undefined();
        default:
            return NULL;
    }
}

static jsini_value_t* clone_object(const jsini_object_t* obj) {
    jsini_object_t* new_obj = jsini_alloc_object();
    if (!new_obj) {
        return NULL;
    }

    const jsh_iterator_t *it;
    for (it = jsh_first(obj->map); it; it = jsh_next(obj->map, it)) {
        const char* key = (const char*)it->key;
        jsini_value_t* val = ((jsini_attr_t*)it->value)->value;
        jsini_value_t* new_val = jsini_clone(val);
        if (new_val) {
            jsini_set_value(new_obj, key, new_val);
        }
    }
    return (jsini_value_t*)new_obj;
}

static jsini_value_t* clone_array(const jsini_array_t* array) {
    jsini_array_t* new_array = jsini_alloc_array();
    if (!new_array) {
        return NULL;
    }

    for (uint32_t i = 0; i < jsini_array_size(array); ++i) {
        jsini_value_t* val = jsini_aget(array, i);
        jsini_value_t* new_val = jsini_clone(val);
        if (new_val) {
            jsini_push_value(new_array, new_val);
        }
    }
    return (jsini_value_t*)new_array;
}