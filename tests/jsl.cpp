#include "jsini.hpp"
#include <assert.h>

static void test_hash_comment() {
    std::string text = R"json({
        # hash comment
        address: {
            country: "Australia",
            city:
                "Adelaide"
        },
        /*
          some comment
        */
        tags: [
            abc,
            // this seems weird
            def
        ]
    })json";
    jsini::Value value(text);
    assert(value.is_object());
    assert(value.lineno() == 1);
    assert(value["address"].lineno() == 3);
    assert(value["tags"].lineno() == 11);
    assert(value["tags"][1].lineno() == 14);
    assert(value["tags"].size() == 2);
}

void test_jsl() {
  test_hash_comment();
}