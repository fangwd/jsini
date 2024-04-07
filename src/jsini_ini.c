/*
 * Copyright (c) Weidong Fang
 */

#include "jsini.h"

#include <assert.h>
#include <ctype.h>

static void skip_ini_spaces(jsl_t *lex) {
    while (lex->input != lex->input_end) {
        jsl_skip_space(lex, 0);
        if (lex->input == lex->input_end) {
            break;
        }
        if (*lex->input != ';') break;
        jsl_skip_line(lex);
    }

}

static jsini_string_t *jsini_read_section_header(jsl_t *lex) {
    jsini_string_t *result = jsini_alloc_string(NULL, 0);
    jsb_t *sb = &result->data;

    assert(*lex->input == '[');

    lex->error_char = *lex->input++;

    while (lex->input != lex->input_end) {
        char c = *lex->input++;
        if (c == ']') break;
        if (lex->input == lex->input_end) {
            lex->error = JSINI_ERROR_NOT_CLOSED;
            break;
        }
        jsb_append_char(sb, c);
    }

    jsb_strip(sb);

    if (sb->size == 0 || lex->error) {
        jsini_free_string(result);
        return NULL;
    }

    return result;
}

static jsini_string_t *jsini_read_ini_bare_string(jsl_t *lex) {
    jsini_string_t *s = jsini_alloc_string(NULL, 0);
    s->lineno = lex->lineno;

    while (lex->input != lex->input_end) {
        char c = *lex->input++;
        if (((c) == ';'||isspace(c))) {
            break;
        }
        jsb_append_char(&s->data, c);
    }

    return s;
}

static jsini_value_t *read_ini_value(jsl_t *lex) {
    jsini_value_t *value;

    skip_ini_spaces(lex);

    if (lex->input == lex->input_end) {
        lex->error = JSINI_ERROR_EOF;
        return NULL;
    }

    switch (*lex->input) {
    case '"':
    case '\'':
    case '`':
        value = (jsini_value_t*)jsl_read_json_string(lex);
        break;
    default:
        if ((value = jsl_read_primitive(lex)) == NULL) {
            value = (jsini_value_t*) jsini_read_ini_bare_string(lex);
        }
        break;
    }
    return value;
}

static int jsini_read_ini_attrs(jsl_t *lex, jsini_object_t *section) {
    while (1) {
        jsini_attr_t *attr;
        jsini_string_t *name;

        skip_ini_spaces(lex);

        if (lex->input == lex->input_end || *lex->input == '[') {
            break;
        }

        if ((name = jsl_read_attr_name(lex)) == NULL) {
            assert(lex->error != JSINI_OK);
            break;
        }

        attr = jsini_alloc_attr(section, name);

        skip_ini_spaces(lex);

        if (lex->input == lex->input_end) {
            lex->error = JSINI_ERROR_EOF;
            break;
        }

        if (*lex->input != '=') {
            lex->error = JSINI_ERROR_SEPARATOR;
            lex->error_char = '=';
            break;
        }

        lex->input++;

        if ((attr->value = read_ini_value(lex)) == NULL) {
            assert(lex->error != JSINI_OK);
            break;
        }
    }

    return lex->error;
}

static int jsini_read_section(jsl_t *lex, jsini_object_t *parent) {
    jsini_object_t *section;
    jsini_attr_t *attr;
    jsini_string_t *name;

    if ((name = jsini_read_section_header(lex)) == NULL) {
        return (lex->error = JSINI_ERROR_NAME);
    }

    attr = jsini_alloc_attr(parent, name);
    section = jsini_alloc_object();
    attr->value = (jsini_value_t *) section;

    return jsini_read_ini_attrs(lex, section);
}

static jsini_object_t *jsini_parse_ini(jsl_t *lex) {
    jsini_object_t *global = jsini_alloc_object();

    if (jsini_read_ini_attrs(lex, global) == JSINI_OK) {
        while (lex->input != lex->input_end && *lex->input == '[') {
            if (jsini_read_section(lex, global) != JSINI_OK) {
                break;
            }
        }
        if (lex->input == lex->input_end && lex->error == JSINI_OK) {
            return global;
        }
    }

    jsini_free_object(global);

    return NULL;
}

jsini_value_t *jsini_parse_string_ini(const char *s, uint32_t len) {
    jsini_object_t *res;
    jsl_t lex;
    jsl_init(&lex, s, len, 0);
    res = jsini_parse_ini(&lex);
    if (!res) jsini_write_error(&lex, stderr);
    return (jsini_value_t*) res;
}

jsini_value_t *jsini_parse_file_ini(const char *file) {
    jsini_value_t *result = NULL;
    jsb_t *sb = jsb_create();
    if (jsb_load(sb, file) == JSB_OK) {
        result = jsini_parse_string_ini(sb->data, sb->size);
    }
    jsb_free(sb);
    return result;
}
