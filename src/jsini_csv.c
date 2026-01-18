/*
 * Copyright (c) Weidong Fang
 */

#include "jsini.h"
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#define JSINI_CSV_INCOMPLETE -99

static int is_quote(char c)
{
    return c == '"' || c == '\'';
}

/*
 * Parses a single CSV record from the buffer.
 * Returns JSINI_OK on success and populates out_row.
 * Returns JSINI_CSV_INCOMPLETE if the record has an open quote at the end.
 */
static int try_parse_csv_record(const char *buffer, size_t len, int flags, jsini_array_t **out_row)
{
    jsini_array_t *row = jsini_alloc_array();
    const char *p = buffer;
    const char *end = buffer + len;
    char delimiter = (flags & JSINI_CSV_TAB) ? '\t' : ',';

    while (p < end)
    {
        jsb_t *sb = jsb_create();
        char quote_char = 0;

        if (flags & JSINI_CSV_DOUBLE_QUOTE)
        {
            if (*p == '"')
            {
                quote_char = *p++;
            }
        }
        else
        {
            if (is_quote(*p))
            {
                quote_char = *p++;
            }
        }

        if (quote_char)
        {
            int closed = 0;
            while (p < end)
            {
                if (*p == quote_char)
                {
                    if (p + 1 < end && *(p + 1) == quote_char)
                    {
                        jsb_append_char(sb, quote_char);
                        p += 2;
                    }
                    else
                    {
                        p++; // Closing quote
                        closed = 1;
                        break;
                    }
                }
                else
                {
                    jsb_append_char(sb, *p++);
                }
            }

            if (!closed)
            {
                jsb_free(sb);
                jsini_free_array(row);
                return JSINI_CSV_INCOMPLETE;
            }

            // Consume trailing chars until delimiter or newline
            while (p < end && *p != delimiter && *p != '\n' && *p != '\r')
                p++;
        }
        else
        {
            // Unquoted
            while (p < end && *p != delimiter && *p != '\n' && *p != '\r')
            {
                jsb_append_char(sb, *p++);
            }
        }

        jsini_push_string(row, sb->data, sb->size);
        jsb_free(sb);

        if (p < end)
        {
            if (*p == delimiter)
            {
                p++;
                // If delimiter is last char or followed by newline, add empty field
                if (p == end || *p == '\n' || *p == '\r')
                {
                    jsini_push_string(row, "", 0);
                }
            }

            // Check for record terminator
            if (p < end && (*p == '\n' || *p == '\r'))
            {
                // Found end of record
                break;
            }
        }
    }

    *out_row = row;
    return JSINI_OK;
}

int jsini_parse_file_csv_ex(const char *file, int flags, jsini_jsonl_cb cb, void *user_data)
{
    FILE *fp = fopen(file, "r");
    if (!fp)
        return JSINI_ERROR;

    jsb_t sb; // Accumulator
    jsb_init(&sb);

    jsb_t line_buf;
    jsb_init(&line_buf);

    jsini_array_t *headers = NULL;
    int res = JSINI_OK;
    int has_header = (flags & JSINI_CSV_HEADER) ? 1 : 0;

    while (jsb_getline(&line_buf, fp) == JSB_OK)
    {
        jsb_append(&sb, line_buf.data, line_buf.size);

        jsini_array_t *row = NULL;
        int parse_res = try_parse_csv_record(sb.data, sb.size, flags, &row);

        if (parse_res == JSINI_CSV_INCOMPLETE)
        {
            // Continue reading
            continue;
        }
        else if (parse_res == JSINI_OK)
        {
            // Success
            jsb_clear(&sb); // Reset accumulator

            if (jsini_array_size(row) == 0)
            {
                jsini_free_array(row);
                continue;
            }

            if (has_header && !headers)
            {
                headers = row;
                continue;
            }

            if (has_header)
            {
                jsini_object_t *obj = jsini_alloc_object();
                uint32_t i;
                for (i = 0; i < jsini_array_size(row); i++)
                {
                    if (i < jsini_array_size(headers))
                    {
                        jsini_value_t *hval = jsini_aget(headers, i);
                        if (hval->type == JSINI_TSTRING)
                        {
                            jsini_string_t *hstr = (jsini_string_t *)hval;
                            jsini_value_t *val = jsini_aget(row, i);

                            if (val->type == JSINI_TSTRING)
                            {
                                jsini_string_t *vstr = (jsini_string_t *)val;
                                jsini_set_string(obj, hstr->data.data, vstr->data.data);
                            }
                            else
                            {
                                jsini_set_null(obj, hstr->data.data);
                            }
                        }
                    }
                }
                res = cb((jsini_value_t *)obj, user_data);
                jsini_free_array(row);
            }
            else
            {
                res = cb((jsini_value_t *)row, user_data);
            }

            if (res != JSINI_OK)
                break;
        }
        else
        {
            // Error
            res = JSINI_ERROR;
            break;
        }
    }

    if (sb.size > 0 && res == JSINI_OK)
    {
        // Trailing data?
        jsini_array_t *row = NULL;
        int parse_res = try_parse_csv_record(sb.data, sb.size, flags, &row);
        if (parse_res == JSINI_OK && jsini_array_size(row) > 0)
        {
            // Handle last row
            if (has_header && headers)
            {
                jsini_object_t *obj = jsini_alloc_object();
                uint32_t i;
                for (i = 0; i < jsini_array_size(row); i++)
                {
                    if (i < jsini_array_size(headers))
                    {
                        jsini_value_t *hval = jsini_aget(headers, i);
                        if (hval->type == JSINI_TSTRING)
                        {
                            jsini_string_t *hstr = (jsini_string_t *)hval;
                            jsini_value_t *val = jsini_aget(row, i);
                            if (val->type == JSINI_TSTRING)
                            {
                                jsini_string_t *vstr = (jsini_string_t *)val;
                                jsini_set_string(obj, hstr->data.data, vstr->data.data);
                            }
                            else
                            {
                                jsini_set_null(obj, hstr->data.data);
                            }
                        }
                    }
                }
                cb((jsini_value_t *)obj, user_data);
                jsini_free_array(row);
            }
            else if (!has_header)
            {
                cb((jsini_value_t *)row, user_data);
            }
            else
            {
                jsini_free_array(row);
            }
        }
        else if (row)
        {
            jsini_free_array(row);
        }
    }

    if (headers)
        jsini_free_array(headers);
    jsb_clean(&sb);
    jsb_clean(&line_buf);
    fclose(fp);
    return res;
}

static void csv_append_string(jsb_t *sb, const char *s, char delimiter)
{
    int needs_quote = 0;
    const char *p = s;
    if (!p)
        return;

    while (*p)
    {
        if (*p == delimiter || *p == '"' || *p == '\n' || *p == '\r')
        {
            needs_quote = 1;
            break;
        }
        p++;
    }

    if (needs_quote)
    {
        jsb_append_char(sb, '"');
        p = s;
        while (*p)
        {
            if (*p == '"')
            {
                jsb_append_char(sb, '"');
            }
            jsb_append_char(sb, *p);
            p++;
        }
        jsb_append_char(sb, '"');
    }
    else
    {
        jsb_append(sb, s, 0);
    }
}

int jsini_print_file_csv(const char *file, const jsini_value_t *value, char delimiter)
{
    if (value->type != JSINI_TARRAY)
        return JSINI_ERROR;

    FILE *fp = fopen(file, "w");
    if (!fp)
        return JSINI_ERROR;

    jsini_array_t *arr = (jsini_array_t *)value;
    if (jsini_array_size(arr) == 0)
    {
        fclose(fp);
        return JSINI_OK;
    }

    jsini_value_t *first = jsini_aget(arr, 0);
    jsini_array_t *headers = NULL;

    if (first->type == JSINI_TOBJECT)
    {
        headers = jsini_alloc_array();
        jsini_object_t *obj = (jsini_object_t *)first;
        uint32_t i;
        for (i = 0; i < obj->keys.size; i++)
        {
            jsini_attr_t *attr = (jsini_attr_t *)obj->keys.item[i];
            jsini_push_string(headers, attr->name->data.data, attr->name->data.size);
        }

        jsb_t sb;
        jsb_init(&sb);
        for (i = 0; i < jsini_array_size(headers); i++)
        {
            if (i > 0)
                jsb_append_char(&sb, delimiter);
            jsini_string_t *h = (jsini_string_t *)jsini_aget(headers, i);
            csv_append_string(&sb, h->data.data, delimiter);
        }
        jsb_append_char(&sb, '\n');
        fwrite(sb.data, 1, sb.size, fp);
        jsb_clean(&sb);
    }

    uint32_t i;
    for (i = 0; i < jsini_array_size(arr); i++)
    {
        jsini_value_t *row_val = jsini_aget(arr, i);
        jsb_t sb;
        jsb_init(&sb);

        if (headers && row_val->type == JSINI_TOBJECT)
        {
            jsini_object_t *obj = (jsini_object_t *)row_val;
            uint32_t k;
            for (k = 0; k < jsini_array_size(headers); k++)
            {
                if (k > 0)
                    jsb_append_char(&sb, delimiter);
                jsini_string_t *key = (jsini_string_t *)jsini_aget(headers, k);
                jsini_value_t *v = jsini_get_value(obj, key->data.data);
                if (v)
                {
                    if (v->type == JSINI_TSTRING)
                    {
                        csv_append_string(&sb, ((jsini_string_t *)v)->data.data, delimiter);
                    }
                    else if (v->type == JSINI_TINTEGER)
                    {
                        jsb_printf(&sb, "%ld", ((jsini_integer_t *)v)->data);
                    }
                    else if (v->type == JSINI_TNUMBER)
                    {
                        jsb_printf(&sb, "%g", ((jsini_number_t *)v)->data);
                    }
                    else if (v->type == JSINI_TBOOL)
                    {
                        jsb_printf(&sb, "%s", ((jsini_bool_t *)v)->data ? "true" : "false");
                    }
                }
            }
        }
        else if (!headers && row_val->type == JSINI_TARRAY)
        {
            jsini_array_t *row_arr = (jsini_array_t *)row_val;
            uint32_t k;
            for (k = 0; k < jsini_array_size(row_arr); k++)
            {
                if (k > 0)
                    jsb_append_char(&sb, delimiter);
                jsini_value_t *v = jsini_aget(row_arr, k);
                if (v->type == JSINI_TSTRING)
                {
                    csv_append_string(&sb, ((jsini_string_t *)v)->data.data, delimiter);
                }
                else if (v->type == JSINI_TINTEGER)
                {
                    jsb_printf(&sb, "%ld", ((jsini_integer_t *)v)->data);
                }
                else if (v->type == JSINI_TNUMBER)
                {
                    jsb_printf(&sb, "%g", ((jsini_number_t *)v)->data);
                }
                else if (v->type == JSINI_TBOOL)
                {
                    jsb_printf(&sb, "%s", ((jsini_bool_t *)v)->data ? "true" : "false");
                }
            }
        }

        jsb_append_char(&sb, '\n');
        fwrite(sb.data, 1, sb.size, fp);
        jsb_clean(&sb);
    }

    if (headers)
        jsini_free_array(headers);
    fclose(fp);
    return JSINI_OK;
}

jsini_value_t *jsini_parse_string_csv(const char *s, uint32_t len)
{
    jsini_array_t *root = jsini_alloc_array();
    const char *p = s;
    const char *end = s + len;
    jsb_t sb;
    jsb_init(&sb);

    int flags = JSINI_CSV_DEFAULT;
    int has_header = (flags & JSINI_CSV_HEADER) ? 1 : 0;
    jsini_array_t *headers = NULL;

    while (p < end)
    {
        const char *line_start = p;
        while (p < end && *p != '\n' && *p != '\r')
        {
            p++;
        }

        if (p < end && *p == '\r')
        {
            p++;
            if (p < end && *p == '\n')
            {
                p++;
            }
        }
        else if (p < end && *p == '\n')
        {
            p++;
        }

        jsb_append(&sb, line_start, p - line_start);

        jsini_array_t *row = NULL;
        int parse_res = try_parse_csv_record(sb.data, sb.size, flags, &row);

        if (parse_res == JSINI_CSV_INCOMPLETE)
        {
            continue;
        }
        else if (parse_res == JSINI_OK)
        {
            jsb_clear(&sb);
            if (jsini_array_size(row) > 0)
            {
                if (has_header && !headers)
                {
                    headers = row;
                }
                else if (has_header)
                {
                    jsini_object_t *obj = jsini_alloc_object();
                    uint32_t i;
                    for (i = 0; i < jsini_array_size(row); i++)
                    {
                        if (i < jsini_array_size(headers))
                        {
                            jsini_value_t *hval = jsini_aget(headers, i);
                            if (hval->type == JSINI_TSTRING)
                            {
                                jsini_string_t *hstr = (jsini_string_t *)hval;
                                jsini_value_t *val = jsini_aget(row, i);

                                if (val->type == JSINI_TSTRING)
                                {
                                    jsini_string_t *vstr = (jsini_string_t *)val;
                                    jsini_set_string(obj, hstr->data.data, vstr->data.data);
                                }
                                else
                                {
                                    jsini_set_null(obj, hstr->data.data);
                                }
                            }
                        }
                    }
                    jsini_push_value(root, (jsini_value_t *)obj);
                    jsini_free_array(row);
                }
                else
                {
                    jsini_push_value(root, (jsini_value_t *)row);
                }
            }
            else
            {
                jsini_free_array(row);
            }
        }
        else
        {
            if (row)
                jsini_free_array(row);
            jsini_free_array(root);
            if (headers) jsini_free_array(headers);
            jsb_clean(&sb);
            return NULL;
        }
    }

    if (sb.size > 0)
    {
        jsini_array_t *row = NULL;
        int parse_res = try_parse_csv_record(sb.data, sb.size, 0, &row);
        if (parse_res == JSINI_OK)
        {
            if (jsini_array_size(row) > 0)
            {
                if (has_header && !headers)
                {
                    headers = row; // Last row is header? Unlikely but consistent logic
                }
                else if (has_header && headers)
                {
                    jsini_object_t *obj = jsini_alloc_object();
                    uint32_t i;
                    for (i = 0; i < jsini_array_size(row); i++)
                    {
                        if (i < jsini_array_size(headers))
                        {
                            jsini_value_t *hval = jsini_aget(headers, i);
                            if (hval->type == JSINI_TSTRING)
                            {
                                jsini_string_t *hstr = (jsini_string_t *)hval;
                                jsini_value_t *val = jsini_aget(row, i);
                                if (val->type == JSINI_TSTRING)
                                {
                                    jsini_string_t *vstr = (jsini_string_t *)val;
                                    jsini_set_string(obj, hstr->data.data, vstr->data.data);
                                }
                                else
                                {
                                    jsini_set_null(obj, hstr->data.data);
                                }
                            }
                        }
                    }
                    jsini_push_value(root, (jsini_value_t *)obj);
                    jsini_free_array(row);
                }
                else
                {
                    jsini_push_value(root, (jsini_value_t *)row);
                }
            }
            else
            {
                jsini_free_array(row);
            }
        }
        else if (row)
        {
            jsini_free_array(row);
        }
    }

    if (headers)
        jsini_free_array(headers);
    jsb_clean(&sb);
    return (jsini_value_t *)root;
}