#include "jsx.h"

#include <string.h>
#include <assert.h>

static void test_empty() {
    jsb_t *sb = jsb_create();
    jsx_t *sx = jsx_create(sb, 0);
    jsb_clear(sb);
    jsx_node_open(sx, "a", 1);
    jsx_free(sx);
    assert(!strcmp(sb->data, "<a/>"));
    jsb_free(sb);
}

static void test_node() {
    jsb_t *sb = jsb_create();
    jsx_t *sx = jsx_create(sb, 0);
    jsb_clear(sb);
    jsx_node_open(sx, "a", 1);
    jsx_node_open(sx, "b", 1);
    jsx_node_open(sx, "c", 1);
    jsx_node_close(sx);
    jsx_node_open(sx, "d", 1);
    jsx_node_close(sx);
    jsx_node_close(sx);
    jsx_node_open(sx, "e", 1);
    jsx_free(sx);
    assert(!strcmp(sb->data, "<a><b><c/><d/></b><e/></a>"));
    jsb_free(sb);
}

static void test_attr() {
    jsb_t *sb = jsb_create();
    jsx_t *sx = jsx_create(sb, 0);
    jsb_clear(sb);
    jsx_node_open(sx, "a", 1);
    jsx_print_attr(sx, "k1", 2, "&", 1);
    jsx_node_open(sx, "b", 1);
    jsx_print_attr_int(sx, "k1", 2, 1);
    jsx_print_attr_double(sx, "k2", 2, 1.5);
    jsx_free(sx);
    assert(!strcmp(sb->data, "<a k1=\"&amp;\"><b k1=\"1\" k2=\"1.5\"/></a>"));
    jsb_free(sb);
}

static void test_text() {
    jsb_t *sb = jsb_create();
    jsx_t *sx = jsx_create(sb, 0);
    jsb_clear(sb);
    jsx_node_open(sx, "a", 1);
    jsx_print_attr(sx, "k1", 2, "&", 1);
    jsx_print_text(sx, "foo", 3);
    jsx_node_open(sx, "b", 1);
    jsx_print_double(sx, 1.5);
    jsx_free(sx);
    assert(!strcmp(sb->data, "<a k1=\"&amp;\">foo<b>1.5</b></a>"));
    jsb_free(sb);
}

int main() {
  test_empty();
  test_node();
  test_attr();
  test_text();
}