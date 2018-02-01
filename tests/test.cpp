#include "jsini.hpp"

#include <assert.h>
#include <iostream>
#include <sstream>

void test_parsing() {
    // JSON
    {
        std::string data = "{\"a\":1,\"b\":2,\"c\":{\"d\":3,\"e\":[4,5]}}";
        jsini::Value value(data);
        assert(value["b"] == 2);
        assert(value["c"]["e"][1] == 5);
    }

    {
        std::string data = "[1,2,{\"a\":3,\"b\":\"c\"}]";
        jsini::Value value(data);
        assert(value[2]["b"] == "c");
    }

    // Javascript
    {
        std::string data = "{ a: 1, b: 2, c: { d: 3, e: [ 4, 5 ] } }";
        jsini::Value value(data);
        assert(value["b"] == 2);
        assert(value["c"]["e"][1] == 5);
    }

    {
        std::string data = "[ 1, 2, { a: 3, b: 'c' } ]";
        jsini::Value value(data);
        assert(value[2]["b"] == "c");
    }

    // Python
    {
        std::string data = "{'a': 1, 'c': {'e': [4, 5], 'd': 3}, 'b': 2}";
        jsini::Value value(data);
        assert(value["b"] == 2);
        assert(value["c"]["e"][1] == 5);
    }

    {
        std::string data = "[1, 2, {'a': 3, 'b': 'c'}]";
        jsini::Value value(data);
        assert(value[2]["b"] == "c");
    }

    // Perl
    {
        std::string data = "{'a' => 1, 'c' => {'e' => [4, 5], 'd' => 3}, 'b' => 2}";
        jsini::Value value(data);
        assert(value["b"] == 2);
        assert(value["c"]["e"][1] == 5);
    }

    {
        std::string data = "[1, 2, {a => 3, b => 'c'}]";
        jsini::Value value(data);
        assert(value[2]["b"] == "c");
    }

    // Ad hoc
    {
        std::string data = "{'a' = 1, 'c' = {'e': [4, 5], 'd' = 3}, 'b': 2}";
        jsini::Value value(data);
        assert(value["b"] == 2);
        assert(value["c"]["e"][1] == 5);
    }

    {
        std::string data = "[1, 2, {a: 3, b => 'c'}]";
        jsini::Value value(data);
        assert(value[2]["b"] == "c");
    }
}

void test_types() {
    std::string data = "[1, null, 'string', {array: [], bool => false}]";
    jsini::Value value(data);
    assert(value[0].is_number());
    assert(value[1].is_null());
    assert(value[2].is_string());
    assert(value[3].is_object());
    assert(value[3]["array"].is_array());
    assert(value[3]["bool"].is_bool());
    assert(value[3]["undefined"].is_undefined());
}

void test_building() {
    jsini::Value value;

    assert(value.is_undefined());

    value.push("Alice");
    value.push("Bob");

    assert(value.is_array());
    assert(value.size() == 2);
    assert(value[1] == std::string("Bob"));

    value = false;

    assert(value.is_bool());

    value["database"]["host"] = "localhost";
    value["database"]["port"] = 3306;
    value["thread_count"] = 8;

    assert(value.is_object());
    assert(value.size() == 2);
    assert(value["database"]["host"] == "localhost");
    assert(value["thread_count"] == 8);

}

void test_removing() {
    {
        std::string data = "[1, 2, {a: 3, b => 'c'}]";
        jsini::Value value(data);

        assert(value.size() == 3);

        value.remove(1);

        assert(value.size() == 2);
        assert(value[1].is_object());

        value.push(123);

        assert(value.size() == 3);
        assert(value[2] == 123);
    }

    {
        jsini::Value value;

        value["foo"]["bar"]["blah"] = "Alice";
        value["foo"]["bar"]["bazz"] = "Bob";

        assert(value["foo"]["bar"].is_object());

        value["foo"].remove("bar");

        assert(value["foo"]["bar"].is_undefined());
    }
}

void test_iterating() {
    std::string data = "{a:1,b:2,c:{d:3,e:4}}";
    jsini::Value value(data);
    jsini::Value::Iterator it = value.begin();
    size_t n = 0;
    while (it != value.end()) {
#       if JSINI_KEEP_KEYS
        if (n == 0) {
            assert(it.key() == std::string("a"));
            assert(it.value() == 1);
        } else if (n == 1) {
            assert(it.key() == std::string("b"));
            assert(it.value() == 2);
        }
#       endif
        it++;
        n++;
    }
    assert(n == 3);

    jsini::Value array(std::string("[1,2,3]"));
    array.push(4);
    array.remove(0);
    for (size_t i = 0; i < array.size(); i++) {
        jsini::Value &value = array[i];
        assert(value == (int )i + 2);
    }
}

static std::string to_string(jsini::Value& value, int options=0, int indent=0) {
    std::stringstream ss;
    value.dump(ss, options, indent);
    //value.dump(std::cout, options, indent);
    //std::cout << '\n';
    return ss.str();
}

void test_dumping() {
    {
        std::string data = "{a:1,b:2,c:{d:3,e:4}}";
        jsini::Value value(data);

        value["continue"] = true;
        value["break"] = false;
        value.remove("continue");

#if     JSINI_KEEP_KEYS
        assert(to_string(value) ==
                "{\"a\":1,\"b\":2,\"c\":{\"d\":3,\"e\":4},\"break\":false}");
#endif

        assert(to_string(value, JSINI_SORT_KEYS) ==
                "{\"a\":1,\"b\":2,\"break\":false,\"c\":{\"d\":3,\"e\":4}}");

        assert(to_string(value, JSINI_PRETTY_PRINT|JSINI_SORT_KEYS, 1) ==
                "{\n \"a\": 1,\n \"b\": 2,\n \"break\": false,\n \"c\": {\n  \"d\": 3,\n  \"e\": 4\n }\n}");

#if     JSINI_KEEP_KEYS
        value.remove("b");
        assert(to_string(value) ==
                "{\"a\":1,\"c\":{\"d\":3,\"e\":4},\"break\":false}");
        value.remove("c");
        assert(to_string(value) == "{\"a\":1,\"break\":false}");
#endif

        {
            jsini::Value value(std::string("[1,[2,3,[4],[]]]"));
            assert(to_string(value) == "[1,[2,3,[4],[]]]");
            assert(to_string(value, JSINI_PRETTY_PRINT, 1) ==
                    "[\n 1,\n [\n  2,\n  3,\n  [\n   4\n  ],\n  []\n ]\n]");
        }
    }
}

int main() {
    test_parsing();
    test_types();
    test_building();
    test_removing();
    test_iterating();
    test_dumping();

    return 0;
}
