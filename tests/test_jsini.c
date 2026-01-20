#include "jsini.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

void test_jsini_c() {
    printf("Testing JSINI C API...\n");

    // Test jsini_remove_value
    {
        jsini_object_t *obj = jsini_alloc_object();
        jsini_set_integer(obj, "foo", 123);
        jsini_set_string(obj, "bar", "hello");

        assert(jsini_object_size(obj) == 2);
        assert(jsini_get_integer(obj, "foo") == 123);

        // Remove existing key
        jsini_value_t *val = jsini_remove_value(obj, "foo");
        assert(val != NULL);
        assert(val->type == JSINI_TINTEGER);
        assert(((jsini_integer_t*)val)->data == 123);
        
        assert(jsini_object_size(obj) == 1);
        assert(jsini_get_value(obj, "foo") == NULL);

        jsini_free(val);

        // Remove non-existing key
        val = jsini_remove_value(obj, "baz");
        assert(val == NULL);
        assert(jsini_object_size(obj) == 1);

        jsini_free_object(obj);
    }

    printf("JSINI C API Tests Passed.\n");
}
