#include "jsini.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static int csv_check_cb(jsini_value_t *val, void *user_data)
{
    jsini_array_t *arr = (jsini_array_t *)user_data;
    jsini_push_value(arr, val);
    return JSINI_OK;
}

void test_csv()
{
    printf("Testing CSV...\n");
    const char *filename = "test_data.csv";

    // Test 1: Simple CSV, no header
    {
        FILE *fp = fopen(filename, "w");
        fprintf(fp, "a,b,c\n1,2,3\n");
        fclose(fp);

        jsini_array_t *result = jsini_alloc_array();
        int res = jsini_parse_file_csv_ex(filename, 0, csv_check_cb, result);
        assert(res == JSINI_OK);
        assert(jsini_array_size(result) == 2);

        jsini_array_t *row1 = (jsini_array_t *)jsini_aget(result, 0);
        assert(jsini_array_size(row1) == 3);

        jsini_string_t *s = (jsini_string_t *)jsini_aget(row1, 0);
        assert(strcmp(s->data.data, "a") == 0);

        jsini_free_array(result);
    }

    // Test 2: With header
    {
        FILE *fp = fopen(filename, "w");
        fprintf(fp, "col1,col2\nval1,val2\nval3,val4");
        fclose(fp);

        jsini_array_t *result = jsini_alloc_array();
        int res = jsini_parse_file_csv_ex(filename, JSINI_CSV_HEADER, csv_check_cb, result);
        assert(res == JSINI_OK);
        assert(jsini_array_size(result) == 2);

        jsini_object_t *row1 = (jsini_object_t *)jsini_aget(result, 0);
        assert(row1->type == JSINI_TOBJECT);

        jsini_value_t *v1 = jsini_get_value(row1, "col1");
        assert(v1 != NULL);
        assert(strcmp(((jsini_string_t *)v1)->data.data, "val1") == 0);

        jsini_free_array(result);
    }

    // Test 3: Quoted fields
    {
        FILE *fp = fopen(filename, "w");
        fprintf(fp, "'a,b',\"c\nd\"\n");
        fclose(fp);

        jsini_array_t *result = jsini_alloc_array();
        int res = jsini_parse_file_csv_ex(filename, 0, csv_check_cb, result);
        assert(res == JSINI_OK);

        jsini_array_t *row1 = (jsini_array_t *)jsini_aget(result, 0);
        jsini_string_t *s1 = (jsini_string_t *)jsini_aget(row1, 0);
        assert(strcmp(s1->data.data, "a,b") == 0);

        jsini_string_t *s2 = (jsini_string_t *)jsini_aget(row1, 1);
        assert(strcmp(s2->data.data, "c\nd") == 0);

        jsini_free_array(result);
    }

    // Test 4: Printing Array of Arrays
    {
        jsini_array_t *rows = jsini_alloc_array();
        jsini_array_t *r1 = jsini_alloc_array();
        jsini_push_string(r1, "a", 1);
        jsini_push_string(r1, "b,c", 3);
        jsini_push_value(rows, (jsini_value_t *)r1);

        int res = jsini_print_file_csv("test_out.csv", (jsini_value_t *)rows, ',');
        assert(res == JSINI_OK);

        FILE *fp = fopen("test_out.csv", "r");
        char buf[100];
        fgets(buf, 100, fp);
        // Expect: a,"b,c"
        assert(strstr(buf, "a,\"b,c\"") != NULL);

        fclose(fp);
        jsini_free_array(rows);
        remove("test_out.csv");
    }

    // Test 5: Printing Array of Objects (with Header)
    {
        jsini_array_t *rows = jsini_alloc_array();
        jsini_object_t *obj = jsini_alloc_object();
        jsini_set_string(obj, "name", "John");
        jsini_set_integer(obj, "age", 30);
        jsini_push_value(rows, (jsini_value_t *)obj);

        int res = jsini_print_file_csv("test_out_obj.csv", (jsini_value_t *)rows, ',');
        assert(res == JSINI_OK);

        FILE *fp = fopen("test_out_obj.csv", "r");
        char buf[1024];
        // Header
        fgets(buf, 1024, fp);
        assert(strstr(buf, "name") != NULL);
        assert(strstr(buf, "age") != NULL);
        // Row
        fgets(buf, 1024, fp);
        assert(strstr(buf, "John") != NULL);
        assert(strstr(buf, "30") != NULL);

        fclose(fp);
        jsini_free_array(rows);
        remove("test_out_obj.csv");
    }

    // Test 6: jsini_parse_string_csv (With Default Headers)
    {
        const char *csv_data = "a,b,c\n1,2,3\n\"quoted line\",4,5";
        jsini_array_t *result = (jsini_array_t *)jsini_parse_string_csv(csv_data, strlen(csv_data));
        assert(result != NULL);
        assert(result->type == JSINI_TARRAY);

        // Since JSINI_CSV_DEFAULT now includes JSINI_CSV_HEADER, we expect 2 rows of objects
        assert(jsini_array_size(result) == 2);

        jsini_object_t *row1 = (jsini_object_t *)jsini_aget(result, 1);
        assert(row1->type == JSINI_TOBJECT);

        jsini_value_t *v = jsini_get_value(row1, "a");
        assert(v != NULL);
        assert(v->type == JSINI_TSTRING);
        assert(strcmp(((jsini_string_t *)v)->data.data, "quoted line") == 0);

        jsini_free_array(result);
    }

    // Test 7: multi-line quoted fields (With Default Headers)
    {
        const char *csv_data = "a,\"b\nc\",d\n1,2,3";
        jsini_array_t *result = (jsini_array_t *)jsini_parse_string_csv(csv_data, strlen(csv_data));
        assert(result != NULL);
        assert(jsini_array_size(result) == 1);

        jsini_object_t *row0 = (jsini_object_t *)jsini_aget(result, 0);
        assert(row0->type == JSINI_TOBJECT);

        // Header "b\nc" maps to value "2"
        jsini_value_t *v = jsini_get_value(row0, "b\nc");
        assert(v != NULL);
        assert(strcmp(((jsini_string_t *)v)->data.data, "2") == 0);

        jsini_free_array(result);
    }

    // Test 8: Strict double quotes
    {
        FILE *fp = fopen(filename, "w");
        // 'a' should NOT be treated as quoted with JSINI_CSV_DOUBLE_QUOTE, so it becomes "'a'".
        // "b" should be treated as quoted.
        fprintf(fp, "'a',\"b\"");
        fclose(fp);

        jsini_array_t *result = jsini_alloc_array();
        int res = jsini_parse_file_csv_ex(filename, JSINI_CSV_DOUBLE_QUOTE, csv_check_cb, result);
        assert(res == JSINI_OK);
        assert(jsini_array_size(result) == 1);

        jsini_array_t *row = (jsini_array_t *)jsini_aget(result, 0);
        assert(jsini_array_size(row) == 2);

        jsini_string_t *s1 = (jsini_string_t *)jsini_aget(row, 0);
        assert(strcmp(s1->data.data, "'a'") == 0); // Single quotes are part of the value

        jsini_string_t *s2 = (jsini_string_t *)jsini_aget(row, 1);
        assert(strcmp(s2->data.data, "b") == 0); // Double quotes are consumed

        jsini_free_array(result);
    }

    // Test 9: Tab Separated Values
    {
        FILE *fp = fopen(filename, "w");
        fprintf(fp, "a\tb\tc\n1\t2\t3\n");
        fclose(fp);

        jsini_array_t *result = jsini_alloc_array();
        int res = jsini_parse_file_csv_ex(filename, JSINI_CSV_TAB, csv_check_cb, result);
        assert(res == JSINI_OK);
        assert(jsini_array_size(result) == 2);

        jsini_array_t *row1 = (jsini_array_t *)jsini_aget(result, 0);
        assert(jsini_array_size(row1) == 3);

        jsini_string_t *s = (jsini_string_t *)jsini_aget(row1, 0);
        assert(strcmp(s->data.data, "a") == 0);

        jsini_free_array(result);
    }

    remove(filename);
}
