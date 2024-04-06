/*
 * Copyright (c) Weidong Fang
 */

#include "jsb.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define JSB_MAX_SIZE   536870912   // 512M

#define xmalloc         malloc
#define xrealloc        realloc
#define xfree           free
#define xfree_safe(p)   if (p) free(p)

jsb_t *jsb_create() {
    jsb_t *sb = (jsb_t *) xmalloc(sizeof(jsb_t));
    sb->data = NULL;
    sb->size = 0;
    sb->alloc_size = 0;
    return sb;
}

jsb_t *jsb_dup(jsb_t*s) {
    jsb_t *t = jsb_create();
    jsb_append(t, s->data, s->size);
    return t;
}

void jsb_clean(jsb_t *sb) {
    if (sb->alloc_size > 0) {
        xfree(sb->data);
        sb->data = NULL;
        sb->alloc_size = 0;
        sb->size = 0;
    }
}

void jsb_free(jsb_t *sb) {
    assert(sb != NULL);
    xfree_safe(sb->data);
    xfree(sb);
}

int jsb_alloc(jsb_t * sb, size_t size) {
    if (sb->alloc_size < size) {
        char *data;

        if ((size = ((size + 31) / 32) * 32) > JSB_MAX_SIZE) {
            return JSB_ERROR;
        }

        if ((data = (char *) xrealloc(sb->data, size)) == NULL ) {
            return JSB_ERROR;
        }

        sb->data = data;
        sb->alloc_size = size;
    }

    return JSB_OK;
}

jsb_t *jsb_append(jsb_t *sb, const char *s, size_t len) {
    int do_free;

    if (sb == NULL) {
        sb = jsb_create();
        do_free = 1;
    }
    else {
        do_free = 0;
    }

    if (s) {
        if (len == 0) len = strlen(s);
        if (sb->size + len + 1 > sb->alloc_size) {
            if (jsb_alloc(sb, sb->size + len + 1) != JSB_OK) {
                if (do_free) {
                    xfree(sb);
                }
                return NULL;
            }
        }
        memcpy(sb->data + sb->size, s, len);
        sb->size += len;
        sb->data[sb->size] = '\0';
    }

    return sb;
}

int jsb_append_char(jsb_t * sb, const char c) {
    assert(sb != NULL);
    if (sb->size + 2 > sb->alloc_size) {
        if (jsb_alloc(sb, sb->size + 2) != JSB_OK) {
            return JSB_ERROR;
        }
    }
    sb->data[sb->size++] = c;
    sb->data[sb->size] = '\0';
    return JSB_OK;
}

jsb_t *jsb_clear(jsb_t *sb) {
    jsb_resize(sb, 0);
    return sb;
}

void jsb_shift(jsb_t * sb, size_t size) {
    assert(size <= sb->size);
    if (size > 0) {
        memmove(sb->data, sb->data + size, sb->size - size + 1);
        sb->size -= size;
        assert(sb->data[sb->size] == '\0');
    }
}

void jsb_init(jsb_t *sb) {
    sb->data = NULL;
    sb->size = 0;
    sb->alloc_size = 0;
}

int jsb_load(jsb_t *sb, const char* filename) {
    FILE  *fp;
    size_t size;

    if ((fp = fopen(filename, "rb")) == NULL) {
        return JSB_ERROR;
    }

    fseek(fp, 0, SEEK_END);
    size = (size_t) ftell(fp);
    fseek(fp, 0, SEEK_SET);

    if (jsb_alloc(sb, size + 1) == JSB_OK) {
        if (fread(sb->data, 1, size, fp) != size) {
            fclose(fp);
            return JSB_ERROR;
        }
        jsb_resize(sb, size);
        fclose(fp);
        return JSB_OK;
    }

    return JSB_ERROR;
}

void jsb_lstrip(jsb_t *sb) {
    size_t n = 0;
    while (n < sb->size) {
        char c = sb->data[n];
        if (c == ' ' || c == '\t' || c == '\r' || c == '\n') {
            n++;
        }
        else {
            break;
        }
    }
    if (n > 0) jsb_shift(sb, n);
}

int jsb_printf(jsb_t * sb, const char *fmt, ...) {
    assert(sb != NULL);
    assert(sb->alloc_size >= sb->size);

    while (1) {
        va_list args;
        int count;
        int space = sb->alloc_size - sb->size;

        va_start(args, fmt);
        count = vsnprintf(sb->data + sb->size, space, fmt, args);
        va_end(args);

        if (count >= 0 && count < space) {
            sb->size += count;
            return count;
        }

        space = count >= 0 ? count + 1 : 2 * space;
        assert (sb->size + space > sb->alloc_size);

        if (jsb_alloc(sb, sb->size + space) != JSB_OK) {
            sb->data[sb->size] = '\0';
            return JSB_ERROR;
        }
    }

    return JSB_ERROR;
}

int jsb_resize(jsb_t *sb, size_t size) {
    assert(sb != NULL);
    if (size + 1 > sb->alloc_size) {
        if (jsb_alloc(sb, size + 1) != JSB_OK) {
            return JSB_ERROR;
        }
    }
    sb->size = size;
    sb->data[size] = '\0';
    return JSB_OK;
}

void jsb_rstrip(jsb_t *sb) {
    while (sb->size > 0) {
        char c = sb->data[sb->size - 1];
        if (c == ' ' || c == '\t' || c == '\r' || c == '\n') {
            sb->size--;
        }
        else {
            break;
        }
    }
    sb->data[sb->size] = '\0';
}

int jsb_save(jsb_t *sb, const char* file) {
    FILE *fp;

    if ((fp = fopen(file, "wb")) == NULL) {
        return JSB_ERROR;
    }

    if (fwrite(sb->data, 1, sb->size, fp) != sb->size) {
        fclose(fp);
        return JSB_ERROR;
    }

    fclose(fp);

    return JSB_OK;
}

void jsb_strip(jsb_t *sb) {
    jsb_lstrip(sb);
    jsb_rstrip(sb);
}

int jsb_equals(jsb_t *a, jsb_t *b) {
  if (a == b) return 1;
  if (!a || !b) return 0;
  return a->size == b->size && !memcmp(a->data, b->data, a->size);
}

/**
 * Quote and append a string to the buffer. Special characters, including “\”,
 * “'”, “"”, NUL (ASCII 0), “\n”, “\r”, and Control+Z, will be escaped in the
 * buffer. This is conformant to MySQL's mysql_real_escape_string() function.
 * See http://dev.mysql.com/doc/refman/5.6/en/mysql-real-escape-string.html for
 * the details about how MySQL escapes literal strings and mysys/charset.c for
 * their implementation.
 */
int jsb_sql_quote(jsb_t *sb, const char *s, size_t len) {
    const char *end;

    if (len == 0) len = strlen(s);

    if (sb->size + 2 * len + 3 > sb->alloc_size) {
        jsb_alloc(sb, sb->size + 2 * len + 3);
    }

    jsb_append_char(sb, '\'');

    for (end = s + len; s < end; s++) {
        char escape = 0;
        switch (*s) {
        case 0:
            escape = '0';
            break;
        case '\n':
            escape = 'n';
            break;
        case '\r':
            escape = 'r';
            break;
        case '\\':
            escape = '\\';
            break;
        case '\'':
            escape = '\'';
            break;
        case '"':
            escape = '"';
            break;
        case '\032':
            escape = 'Z';
            break;
        }
        if (escape) {
        	jsb_append_char(sb, '\\');
        	jsb_append_char(sb, escape);
        } else {
        	jsb_append_char(sb, *s);
        }
    }

    jsb_append_char(sb, '\'');

	return JSB_OK;
}

int jsb_log_quote(jsb_t *sb, const char *s, size_t len) {
    const char *end;

    if (len == 0) len = strlen(s);

    if (sb->size + 2 * len + 3 > sb->alloc_size) {
        jsb_alloc(sb, sb->size + 2 * len + 3);
    }

    for (end = s + len; s < end; s++) {
        char escape = 0;
        switch (*s) {
        case 0:
            escape = '0';
            break;
        case '\n':
            escape = 'n';
            break;
        case '\r':
            escape = 'r';
            break;
        case '\\':
            escape = '\\';
            break;
        case '\t':
            escape = 't';
            break;
        case '\032':
            escape = 'Z';
            break;
        }
        if (escape) {
        	jsb_append_char(sb, '\\');
        	jsb_append_char(sb, escape);
        } else {
        	jsb_append_char(sb, *s);
        }
    }

	return JSB_OK;
}

