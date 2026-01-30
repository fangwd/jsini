#include "jsini_clone.h"
#include <assert.h>
#include <string.h>

void test_clone() {
    jsini_object_t* obj = jsini_alloc_object();
    jsini_set_integer(obj, "int_val", 123);
    jsini_set_number(obj, "num_val", 45.67);
    jsini_set_string(obj, "str_val", "hello");
    jsini_set_bool(obj, "bool_val", 1);
    jsini_set_null(obj, "null_val");

    jsini_array_t* arr = jsini_alloc_array();
    jsini_push_integer(arr, 1);
    jsini_push_string(arr, "two", 3);
    jsini_set_value(obj, "arr_val", (jsini_value_t*)arr);

    jsini_object_t* nested_obj = jsini_alloc_object();
    jsini_set_string(nested_obj, "nested_key", "nested_value");
    jsini_set_value(obj, "obj_val", (jsini_value_t*)nested_obj);

    jsini_value_t* cloned_val = jsini_clone((jsini_value_t*)obj);
    assert(cloned_val != NULL);
    assert(cloned_val->type == JSINI_TOBJECT);

    jsini_object_t* cloned_obj = (jsini_object_t*)cloned_val;

    assert(jsini_get_integer(cloned_obj, "int_val") == 123);
    
    jsini_value_t* num_val = jsini_get_value(cloned_obj, "num_val");
    assert(num_val != NULL && num_val->type == JSINI_TNUMBER);
    assert(jsini_cast_int(num_val) == 45);

    assert(strcmp(jsini_get_string(cloned_obj, "str_val"), "hello") == 0);

    jsini_value_t* bool_val = jsini_get_value(cloned_obj, "bool_val");
    assert(bool_val != NULL && bool_val->type == JSINI_TBOOL);
    assert(((jsini_bool_t*)bool_val)->data == 1);

    assert(jsini_get_value(cloned_obj, "null_val")->type == JSINI_TNULL);
    
    jsini_array_t* cloned_arr = jsini_get_array(cloned_obj, "arr_val");
    assert(cloned_arr != NULL);
    assert(jsini_array_size(cloned_arr) == 2);
    assert(((jsini_integer_t*)jsini_aget(cloned_arr, 0))->data == 1);
    assert(strcmp(((jsini_string_t*)jsini_aget(cloned_arr, 1))->data.data, "two") == 0);

    jsini_object_t* cloned_nested_obj = jsini_get_object(cloned_obj, "obj_val");
    assert(cloned_nested_obj != NULL);
    assert(strcmp(jsini_get_string(cloned_nested_obj, "nested_key"), "nested_value") == 0);

    // Verify they are different pointers (deep copy)
    assert((void*)obj != (void*)cloned_obj);
    assert((void*)arr != (void*)cloned_arr);
    assert((void*)nested_obj != (void*)cloned_nested_obj);

    jsini_free((jsini_value_t*)obj);
    jsini_free(cloned_val);
    
    printf("test_clone passed.\n");
}
