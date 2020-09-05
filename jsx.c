#include "jsx.h"

#include <stdlib.h>

#include "jsa.h"

#define XML_DECL "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
#define XML_DECL2 "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>"

typedef struct {
    uint32_t name_start;
    uint32_t name_length;
    uint32_t node_count;
} stack_entry;

struct jsx_s {
    jsa_t *stack;
    uint32_t stack_size;
    jsb_t *buf;
};

#define xmalloc malloc
#define xfree free

jsx_t *jsx_create(jsb_t *buf, int standalone) {
    jsx_t *sx = (jsx_t *)xmalloc(sizeof(*sx));
    sx->stack = jsa_create();
    sx->stack_size = 0;
    sx->buf = buf;
    if (standalone) {
        jsb_append(buf, XML_DECL2, sizeof XML_DECL2 - 1);
    } else {
        jsb_append(buf, XML_DECL, sizeof XML_DECL - 1);
    }
    return sx;
}

void jsx_free(jsx_t *sx) {
    while (sx->stack_size > 0) {
        jsx_node_close(sx);
    }
    for (uint32_t i = 0; i < sx->stack->size; i++) {
        xfree((stack_entry *)jsa_get(sx->stack, i));
    }
    jsa_free(sx->stack);
    xfree(sx);
}

void jsx_node_open(jsx_t *sx, const char *name, size_t size) {
    stack_entry *entry;
    jsb_t *sb = sx->buf;

    if (sx->stack_size > 0) {
        entry = (stack_entry *)sx->stack->item[sx->stack_size - 1];
        if (entry->node_count == 0 && entry->name_length > 0) {
            jsb_append_char(sb, '>');
        }
        entry->node_count++;
    }

    if (sx->stack_size == sx->stack->size) {
        entry = (stack_entry *)xmalloc(sizeof(*entry));
        jsa_push(sx->stack, (JSA_TYPE)entry);
    } else {
        entry = (stack_entry *)sx->stack->item[sx->stack_size];
    }

    if (size > 0) {
        jsb_append_char(sb, '<');
    }

    entry->name_start = sb->size;
    entry->name_length = size;
    entry->node_count = 0;

    if (size > 0) {
        jsb_append(sb, name, size);
    }

    sx->stack_size++;
}

static void print_text(jsb_t *sb, const char *begin, size_t length) {
    const char *end = begin + length;
    while (begin != end) {
        switch (*begin) {
            case '<':
                jsb_append(sb, "&lt;", sizeof "&lt;" - 1);
                break;
            case '>':
                jsb_append(sb, "&gt;", sizeof "&gt;" - 1);
                break;
            case '\'':
                jsb_append(sb, "&apos;", sizeof "&apos;" - 1);
                break;
            case '"':
                jsb_append(sb, "&quot;", sizeof "&quot;" - 1);
                break;
            case '&':
                jsb_append(sb, "&amp;", sizeof "&amp;" - 1);
                break;
            default:
                jsb_append_char(sb, *begin);
                break;
        }
        ++begin;
    }
}

void jsx_print_text(jsx_t *sx, const char *text, size_t size) {
    jsx_node_open(sx, 0, 0);
    print_text(sx->buf, text, size);
}

void jsx_print_attr(jsx_t *sx, const char *name, size_t name_size,
                    const char *value, size_t value_size) {
    jsb_t *sb = sx->buf;
    jsb_append_char(sb, ' ');
    jsb_append(sb, name, name_size);
    jsb_append_char(sb, '=');
    jsb_append_char(sb, '"');
    print_text(sb, value, value_size);
    jsb_append_char(sb, '"');
}

void jsx_print_attr_int(jsx_t *sx, const char *name, size_t name_size, int value) {
    jsb_t *sb = sx->buf;
    jsb_append_char(sb, ' ');
    jsb_append(sb, name, name_size);
    jsb_append_char(sb, '=');
    jsb_append_char(sb, '"');
    jsb_printf(sb, "%d", value);
    jsb_append_char(sb, '"');
}

void jsx_print_attr_double(jsx_t *sx, const char *name, size_t name_size, double value) {
    jsb_t *sb = sx->buf;
    jsb_append_char(sb, ' ');
    jsb_append(sb, name, name_size);
    jsb_append_char(sb, '=');
    jsb_append_char(sb, '"');
    jsb_printf(sb, "%g", value);
    jsb_append_char(sb, '"');
}

void jsx_node_close(jsx_t *sx) {
    stack_entry *entry = (stack_entry *)sx->stack->item[sx->stack_size - 1];
    jsb_t *sb = sx->buf;
    if (entry->name_length > 0) {
        if (entry->node_count == 0) {
            jsb_append(sb, "/>", 2);
        } else {
            jsb_append(sb, "</", 2);
            jsb_append(sb, sb->data + entry->name_start, entry->name_length);
            jsb_append_char(sb, '>');
        }
    }
    sx->stack_size--;
}
