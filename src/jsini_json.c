/*
 * Copyright (c) Weidong Fang
 */

#include "jsini.h"

#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#define xmalloc malloc
#define xfree free

jsini_value_t *jsini_read_json(jsl_t *js);
int jsl_decode_json_string(jsl_t *lex, jsb_t *s);;

static jsini_array_t *jsini_read_json_array(jsl_t *lex) {
    jsini_array_t *array = jsini_alloc_array();
    array->lineno = lex->lineno;

    char array_open = *lex->input;
    char array_end;

    switch (array_open) {
        case '[':
            array_end = ']';
            break;
        case '(':
            array_end = ')';
            break;
        default:
            assert(0);
    }

    lex->error_char = *lex->input++;

    while (lex->input < lex->input_end) {
        jsini_value_t *value;

        jsl_skip_space(lex, ",");

        if (lex->input == lex->input_end) {
            lex->error = JSINI_ERROR_NOT_CLOSED;
            goto fail;
        }

        if (*lex->input == array_end) {
            lex->input++;
            return array;
        }

        if ((value = jsini_read_json(lex)) == NULL) {
            goto fail;
        }

        jsa_push(&array->data, value);
    }

    lex->error = JSINI_ERROR_NOT_CLOSED;

fail:
    jsini_free_array(array);
    assert(lex->error != JSINI_OK);
    return NULL;
}

jsini_string_t *jsl_read_attr_name(jsl_t *lex) {
    jsini_string_t *result = jsini_alloc_string(NULL, 0);
    result->lineno = lex->lineno;

    jsb_t *sb = &result->data;

    if (*lex->input == '\'' || *lex->input == '"' || *lex->input == '`') {
        if (jsl_decode_json_string(lex, sb) != JSINI_OK) {
            jsini_free_string(result);
            return NULL;
        }
        return result;
    }

    while (lex->input != lex->input_end) {
        char c = *lex->input;
        if (!isalnum(c) && c != '_') break;
        jsb_append_char(sb, c);
        lex->input++;
    }

    if (sb->size == 0) {
        jsini_free_string(result);
        return NULL;
    }

    return result;
}

static int jsl_read_env(jsl_t *lex, jsb_t *s) {
    char name[256];
    size_t len = 0;
    const char *brace_open = lex->input++; // {

    while (lex->input < lex->input_end && *lex->input != '`') {
        if (*lex->input != '}') {
            if (len < sizeof (name) - 1) {
                name[len++] = *lex->input;
            }
            lex->input++;
        }
        else if (*lex->input != '`') {
            lex->input++;
            break;
        }
    }

    if (len > 0) {
        if (*(lex->input - 1) != '}') {
            jsb_append_char(s, '$');
            jsb_append(s, brace_open, lex->input - brace_open);
        } else {
            name[len] = '\0';
            const char *value = getenv(name);
            if (value) {
                jsb_append(s, value, strlen(value));
            } else {
                jsb_append_char(s, '$');
                jsb_append(s, brace_open, lex->input - brace_open);
            }
        }
    } else {
        jsb_append(s, "${}", 3);
    }

    return JSINI_OK;
}

static jsini_string_t *jsini_read_json_bare_string(jsl_t *lex) {
    jsini_string_t *s = jsini_alloc_string(NULL, 0);
    s->lineno = lex->lineno;

    while (lex->input != lex->input_end) {
        char c = *lex->input++;
        if (c == '$' && lex->input < lex->input_end && *lex->input == '{') {
            jsl_read_env(lex, &s->data);
            return s;
        }
        if (c == ',' || isspace(c)) {
            break;
        }
        if (c == ']'|| c == '}') {
            lex->input--;
            break;
        }
        jsb_append_char(&s->data, c);
    }

    return s;
}

static jsini_object_t *jsini_read_json_object(jsl_t *lex) {
    jsini_object_t *object = jsini_alloc_object();
    object->lineno = lex->lineno;

    assert(*lex->input == '{');

    lex->error_char = *lex->input++;

    while (lex->input < lex->input_end) {
        jsini_attr_t *attr;
        jsini_string_t *name;

        jsl_skip_space(lex, ",");

        if (lex->input == lex->input_end) {
            lex->error = JSINI_ERROR_NOT_CLOSED;
            goto fail;
        }

        if (*lex->input == '}') {
            lex->input++;
            return object;
        }

        if ((name = jsl_read_attr_name(lex)) == NULL) {
            lex->error = JSINI_ERROR_NAME;
            goto fail;
        }

        attr = jsini_alloc_attr(object, name);

        jsl_skip_space(lex, NULL);

        if (*lex->input != ':' && *lex->input != '=') {
            lex->error_char = ':';
            lex->error = JSINI_ERROR_SEPARATOR;
            goto fail;
        }

        lex->input++;

        /* Perl style hash */
        jsl_skip_space(lex, ">");

        if ((attr->value = jsini_read_json(lex)) == NULL) {
            goto fail;
        }
    }

    lex->error = JSINI_ERROR_NOT_CLOSED;

fail:
    jsini_free_object(object);
    assert(lex->error != JSINI_OK);
    return NULL;
}

jsini_value_t *jsini_read_json(jsl_t *lex) {
    jsini_value_t *value = NULL;

    jsl_skip_space(lex, NULL);

    if (lex->input >= lex->input_end) return NULL;

    switch (*lex->input) {
    case '{':
        return (jsini_value_t*)jsini_read_json_object(lex);
    case '[':
    case '(':
        return (jsini_value_t*)jsini_read_json_array(lex);
    case '"':
    case '\'':
    case '`':
        return (jsini_value_t*)jsl_read_json_string(lex);
    default:
        if ((value = jsl_read_primitive(lex)) == NULL) {
            value = (jsini_value_t*)jsini_read_json_bare_string(lex);
        }
        break;
    }

    return value;
}

static int jsini_read_json_utf8(jsl_t *lex, int offset, jsb_t *s) {
    int32_t c;
    int32_t m = json_unescape_unicode(lex->input + offset, lex->input_end, &c);

    if (m <= 0) {
        return (lex->error = JSINI_ERROR_ESCAPE);
    }
    else {
        char data[4];
        int n = encode_utf8(c, data);
        jsb_append(s, data, n);
        lex->input += m + offset;
        return JSINI_OK;
    }
}

int jsl_decode_json_string(jsl_t *lex, jsb_t *s) {
    char quote = *lex->input++;
    while (lex->input != lex->input_end) {
        char c = *lex->input++;
        if (c == quote) {
            return JSINI_OK;
        }
        else if (c == '\\') {
            if (lex->input == lex->input_end) {
                return (lex->error = JSINI_ERROR_EOF);
            }
            switch (*lex->input++) {
            case '"':
                jsb_append_char(s, '"');
                break;
            case '/':
                jsb_append_char(s, '/');
                break;
            case '\\':
                jsb_append_char(s, '\\');
                break;
            case 'b':
                jsb_append_char(s, '\b');
                break;
            case 'f':
                jsb_append_char(s, '\f');
                break;
            case 'n':
                jsb_append_char(s, '\n');
                break;
            case 'r':
                jsb_append_char(s, '\r');
                break;
            case 't':
                jsb_append_char(s, '\t');
                break;
            case 'u':
                if (jsini_read_json_utf8(lex, -2, s) != JSINI_OK) {
                    return (lex->error = JSINI_ERROR_ESCAPE);
                }
                break;
            default: {
                    return (lex->error = JSINI_ERROR_ESCAPE);
                }
            }
        }
        else if (c == '$') {
            if (quote == '`' && lex->input < lex->input_end && *lex->input == '{') {
                jsl_read_env(lex, s);
            }
            else {
                jsb_append_char(s, '$');
            }
        } else {
            jsb_append_char(s, c);
        }
    }

    return (lex->error = JSINI_ERROR_EOF);
}

jsini_string_t *jsl_read_json_string(jsl_t *lex) {
    jsini_string_t *s = jsini_alloc_string(NULL, 0);
    s->lineno = lex->lineno;
    if (jsl_decode_json_string(lex, &s->data) != JSINI_OK) {
        jsini_free_string(s);
        return NULL;
    }
    return s;
}

jsini_value_t *jsini_parse_string(const char *s, uint32_t len) {
    jsini_value_t *res;
    jsl_t lex;
    jsl_init(&lex, s, len, JSINI_COMMENT);
    res = jsini_read_json(&lex);
    if (!res) jsini_write_error(&lex, stderr);
    jsl_skip_space(&lex, NULL);
    if (lex.input < lex.input_end) {
        fprintf(stderr, "WARNING: Unexpected character '%c' at line %zu\n",
                *lex.input, lex.lineno + 1);
    }
    return res;
}

jsini_value_t *jsini_parse_file(const char *file) {
    jsini_value_t *result = NULL;
    jsb_t *sb = jsb_create();
    if (jsb_load(sb, file) == JSB_OK) {
        result = jsini_parse_string(sb->data, sb->size);
    }
    jsb_free(sb);
    return result;
}

void jsini_write_string(jsb_t *sb, jsb_t *s, int options) {
    const char *p = s->data;
    const char *q = s->data + s->size;
    int c;

    jsb_append_char(sb, '"');

    if (p == NULL) {
        jsb_append_char(sb, '"');
        return;
    }

    for (c = *p; p < q; c = *++p) {
        switch (c) {
        case '\"':
            jsb_append(sb, "\\\"", 2);
            break;
        case '\\':
            jsb_append(sb, "\\\\", 2);
            break;
        case '\b':
            jsb_append(sb, "\\b", 2);
            break;
        case '\f':
            jsb_append(sb, "\\f", 2);
            break;
        case '\n':
            jsb_append(sb, "\\n", 2);
            break;
        case '\r':
            jsb_append(sb, "\\r", 2);
            break;
        case '\t':
            jsb_append(sb, "\\t", 2);
            break;
        default:
            if (c > 0 && c <= 0x1F) {
                jsb_printf(sb, "\\u%04x", c);
            } else if ((c & 0x80) && (options & JSINI_ESCAPE_UNICODE)) {
                int32_t ch, n;
                if ((n = decode_utf8(p, &ch)) > 0) {
                    char buf[12];
                    p += n - 1;
                    n = json_escape_unicode(ch, buf);
                    jsb_append(sb, buf, n);
                } else {
                    jsb_append_char(sb, c);
                }
            } else {
                jsb_append_char(sb, c);
            }
            break;
        }
    }
    jsb_append_char(sb, '"');
}

void jsini_stringify_real(const jsini_value_t *, jsb_t *, int, int, int);

#define SHIFT(b,n,t) do{int i;for(i=0;i<(n)*(t);i++)jsb_append_char(b,' ');}while(0)

static void strattr(jsb_t *sb, const jsini_attr_t *attr, int options,
        int level, int indent) {
	if ((options & JSINI_PRETTY_PRINT)!= 0) SHIFT(sb, level, indent);
    jsini_write_string(sb, &attr->name->data, options);
	jsb_append_char(sb, ':');
    if ((options & JSINI_PRETTY_PRINT)!= 0) jsb_append_char(sb, ' ');
    jsini_stringify_real(attr->value, sb, options, level, indent);
}

static int attrcmp(const void *a, const void *b) {
    jsini_attr_t *a1 = *((jsini_attr_t **) a);
    jsini_attr_t *a2 = *((jsini_attr_t **) b);
    return strcmp(a1->name->data.data, a2->name->data.data);
}

void jsini_stringify_real(const jsini_value_t *value, jsb_t *sb, int options,
        int level, int indent) {

    if (level > 1024) {
        fprintf(stderr, "** OVERFLOW (%d) **\n", level);
        fprintf(stderr, "%s", sb->data);
        exit(1);
    }

	if ((options & JSINI_PHP_EXPORT) != 0) {
		if (jsini_get_lang_bit(value, 0)) {
			jsb_append_char(sb, '&');
		}
		if (jsini_get_lang_bit(value, 1)) {
			jsb_append_char(sb, '@');
		}
	}

    if (!value) {
        jsb_append(sb, "null", 4);
        return;
    }

    switch (value->type) {
    case JSINI_TNULL:
        jsb_append(sb, "null", 4);
        break;
    case JSINI_TBOOL:
        jsb_printf(sb, "%s", ((jsini_bool_t*)value)->data ? "true" : "false");
        break;
    case JSINI_TINTEGER:
        jsb_printf(sb, "%ld", ((jsini_integer_t*)value)->data);
        break;
    case JSINI_TNUMBER:
        jsb_printf(sb, "%g", ((jsini_number_t*)value)->data);
        break;
    case JSINI_TSTRING:
		jsini_write_string(sb, &((jsini_string_t*)value)->data, options);
        break;
    case JSINI_TOBJECT:
        jsb_append_char(sb, '{');
        {
            jsh_t *map = ((jsini_object_t *) value)->map;
            const jsh_iterator_t *it = jsh_first(map);
            if ((options & JSINI_PRETTY_PRINT) != 0 && jsh_count(map) > 0) {
                jsb_append_char(sb, '\n');
            }
            if (options & JSINI_SORT_KEYS) {
                uint32_t i;
                jsa_t *a = jsa_create();
                for (; it; it = jsh_next(map, it)) {
                    jsini_value_t *value = ((jsini_attr_t *)it->value)->value;
                    if (value->type!= JSINI_UNDEFINED) {
                    jsa_push(a, it->value);
                    }
                }
                qsort(a->item, a->size, sizeof(a->item[0]), attrcmp);
                for (i = 0; i < a->size; i++) {
                    jsini_attr_t *attr = (jsini_attr_t *)a->item[i];
                    if (attr->value->type == JSINI_UNDEFINED) {
                        continue;
                    }

                    strattr(sb, attr, options, level + 1, indent);
                    if (i < a->size - 1) jsb_append_char(sb, ',');
                    if ((options & JSINI_PRETTY_PRINT)!= 0) jsb_append_char(sb, '\n');
                }
                jsa_free(a);
                if ((options & JSINI_PRETTY_PRINT)!= 0) SHIFT(sb, level, indent);
            }
            else {
                uint32_t n = 0;
                jsa_t *array = &((jsini_object_t *) value)->keys;
                uint32_t i;
                for (i = 0; i < array->size; i++) {
                    jsini_attr_t *attr = (jsini_attr_t*)array->item[i];
                    if (attr->value->type == JSINI_UNDEFINED) {
                        continue;
                    }
                    if (n++ > 0) {
                        jsb_append_char(sb, ',');
                        if ((options & JSINI_PRETTY_PRINT) != 0)
                            jsb_append_char(sb, '\n');
                    }
                    strattr(sb, attr, options, level + 1, indent);
                }
                if (n > 0 && (options & JSINI_PRETTY_PRINT) != 0) {
                    jsb_append_char(sb, '\n');
                    SHIFT(sb, level, indent);
                }
            }
        }
        jsb_append_char(sb, '}');
        break;
    case JSINI_TARRAY:
        jsb_append_char(sb, '[');
        {
            jsini_array_t *array = (jsini_array_t *) value;
            uint32_t i, n = 0;
            for (i = 0; i < array->data.size; i++) {
                jsini_value_t *item = (jsini_value_t *)array->data.item[i];
                if (item->type == JSINI_UNDEFINED) {
                    continue;
                }
                if (n++ > 0) {
                    jsb_append_char(sb, ',');
                    if ((options & JSINI_PRETTY_PRINT)!= 0) jsb_append_char(sb, '\n');
                }
                else if ((options & JSINI_PRETTY_PRINT) != 0) {
                    jsb_append_char(sb, '\n');
                }
                if ((options & JSINI_PRETTY_PRINT) != 0) SHIFT(sb, level + 1, indent);
                jsini_stringify_real(item, sb, options, level + 1, indent);
            }
            if (n > 0 && (options & JSINI_PRETTY_PRINT) != 0) {
                jsb_append_char(sb, '\n');
                SHIFT(sb, level, indent);
            }
        }
        jsb_append_char(sb, ']');
        break;
    case JSINI_UNDEFINED:
        jsb_append(sb, "undefined", sizeof("undefined") - 1);
        break;
    default:
        assert(0);
        break;
    }
}

void jsini_stringify(const jsini_value_t *value, jsb_t *sb, int options,
        int indent) {
    jsini_stringify_real(value, sb, options, 0, indent);
}

void jsini_print(FILE *fp, const jsini_value_t *value, int options) {
    jsb_t sb;
    jsb_init(&sb);
    jsini_stringify(value, &sb, options, 2);
    fwrite(sb.data, 1, sb.size, fp);
    jsb_clean(&sb);
}

int jsini_print_file(const char *filename, const jsini_value_t *value, int options) {
    FILE *fp;

    if ((fp = fopen(filename, "wb")) == NULL) {
        return -1;
    }

    jsini_print(fp, value, options);

    return fclose(fp);
}

