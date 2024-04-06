#include <assert.h>

#include <iostream>
#include <sstream>

#include "jsini.hpp"

#define eps 0.0001

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

    // Array
    {
        std::string data = "( )";
        jsini::Value value(data);
        assert(value.is_array());
        assert(value.size() == 0);
    }

    {
        std::string data = "(, )";
        jsini::Value value(data);
        assert(value.is_array());
        assert(value.size() == 0);
    }

    {
        std::string data = "(1, 2,)";
        jsini::Value value(data);
        assert(value.is_array());
        assert(value.size() == 2);
        assert(value[size_t(0)] == 1);
        assert(value[1] == 2);
    }

    // Keywords
    {
        std::string data = "(False, True, NULL)";
        jsini::Value value(data);
        assert(value.is_array());
        assert(value.size() == 3);
        assert(value[size_t(0)].is_bool());
        assert(value[size_t(0)] == false);
        assert(value[1].is_bool());
        assert(value[1] == true);
        assert(value[2].is_null());
    }
}

void test_types() {
    std::string data = "[1, null, 'string', {array: [], bool => false}]";
    jsini::Value value(data);
    assert(value[(uint32_t)0].is_number());
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
        if (n == 0) {
            assert(it.key() == std::string("a"));
            assert(it.value() == 1);
        } else if (n == 1) {
            assert(it.key() == std::string("b"));
            assert(it.value() == 2);
        }
        it++;
        n++;
    }
    assert(n == 3);

    jsini::Value array(std::string("[1,2,3]"));
    array.push(4);
    array.remove(0);
    for (size_t i = 0; i < array.size(); i++) {
        jsini::Value& value = array[i];
        assert(value == (int)i + 2);
    }
}

static std::string to_string(jsini::Value& value, int options = 0, int indent = 0) {
    std::stringstream ss;
    value.dump(ss, options, indent);
    // value.dump(std::cout, options, indent);
    // std::cout << '\n';
    return ss.str();
}

void test_dumping() {
    {
        std::string data = "{a:1,b:2,c:{d:3,e:4}}";
        jsini::Value value(data);

        value["continue"] = true;
        value["break"] = false;
        value.remove("continue");

        assert(value.size() == 4);
        assert(to_string(value) == "{\"a\":1,\"b\":2,\"c\":{\"d\":3,\"e\":4},\"break\":false}");

        assert(to_string(value, JSINI_SORT_KEYS) == "{\"a\":1,\"b\":2,\"break\":false,\"c\":{\"d\":3,\"e\":4}}");

        assert(to_string(value, JSINI_PRETTY_PRINT | JSINI_SORT_KEYS, 1) ==
               "{\n \"a\": 1,\n \"b\": 2,\n \"break\": false,\n \"c\": {\n  \"d\": 3,\n  \"e\": 4\n }\n}");

        value.remove("b");
        assert(to_string(value) == "{\"a\":1,\"c\":{\"d\":3,\"e\":4},\"break\":false}");
        value.remove("c");
        assert(to_string(value) == "{\"a\":1,\"break\":false}");

        {
            jsini::Value value(std::string("[1,[2,3,[4],[]]]"));
            assert(to_string(value) == "[1,[2,3,[4],[]]]");
            assert(to_string(value, JSINI_PRETTY_PRINT, 1) == "[\n 1,\n [\n  2,\n  3,\n  [\n   4\n  ],\n  []\n ]\n]");
        }
    }
}

void test_type_cast() {
    {
        std::string data = R"js("hello")js";
        jsini::Value value(data);
        std::string hello = (std::string)(value);
        assert(hello == "hello");
    }
    {
        std::string data = R"js(3.14)js";
        jsini::Value value(data);
        float pi = (float)(value);
        assert(abs(pi - 3.14) < eps);
    }
}

void test_to() {
    // bool
    {
        jsini::Value value(std::string("1"));
        bool dst;
        assert(value.to(dst) == JSINI_ERROR);
    }
    {
        jsini::Value value(std::string("true"));
        bool dst;
        assert(value.to(dst) == JSINI_OK);
        assert(dst);
    }

    // int
    {
        jsini::Value value(std::string("1.0"));
        int dst;
        assert(value.to(dst) == JSINI_ERROR);
    }
    {
        jsini::Value value(std::string("1"));
        int dst;
        assert(value.to(dst) == JSINI_OK);
        assert(dst == 1);
    }
    {
        jsini::Value value(std::string("1"));
        size_t dst;
        assert(value.to(dst) == JSINI_OK);
        assert(dst == 1);
    }
    {
        jsini::Value value(std::string("1"));
        long dst;
        assert(value.to(dst) == JSINI_OK);
        assert(dst == 1);
    }

    // double
    {
        jsini::Value value(std::string("true"));
        double dst;
        assert(value.to(dst) == JSINI_ERROR);
    }
    {
        jsini::Value value(std::string("1"));
        double dst;
        assert(value.to(dst) == JSINI_OK);
        assert(abs(dst - 1) < eps);
    }
    {
        jsini::Value value(std::string("1.1"));
        double dst;
        assert(value.to(dst) == JSINI_OK);
        assert(abs(dst - 1.1) < eps);
    }

    // float
    {
        jsini::Value value(std::string("true"));
        float dst;
        assert(value.to(dst) == JSINI_ERROR);
    }
    {
        jsini::Value value(std::string("1"));
        float dst;
        assert(value.to(dst) == JSINI_OK);
        assert(abs(dst - 1) < eps);
    }
    {
        jsini::Value value(std::string("1.1"));
        float dst;
        assert(value.to(dst) == JSINI_OK);
        assert(abs(dst - 1.1) < eps);
    }

    // const char *
    {
        jsini::Value value(std::string("null"));
        const char* dst = "";
        assert(value.to(dst) == JSINI_OK);
        assert(dst == nullptr);
    }
    {
        jsini::Value value(std::string("1"));
        const char* dst;
        assert(value.to(dst) == JSINI_ERROR);
    }
    {
        jsini::Value value(std::string("\"hello\""));
        const char* dst;
        assert(value.to(dst) == JSINI_OK);
        assert(dst == std::string("hello"));
    }

    // std::string
    {
        jsini::Value value(std::string("null"));
        std::string dst;
        assert(value.to(dst) == JSINI_ERROR);
    }
    {
        jsini::Value value(std::string("1"));
        std::string dst;
        assert(value.to(dst) == JSINI_ERROR);
    }
    {
        jsini::Value value(std::string("\"hello\""));
        std::string dst;
        assert(value.to(dst) == JSINI_OK);
        assert(dst == "hello");
    }
}

namespace ns1 {
struct OrderItem {
    int quantity;
    int from_json(jsini::Value& value) { return value["quantity"].to(quantity); }
};

struct Order {
    int id;
    std::vector<OrderItem*> items;
    ~Order() {
        for (auto& item : items) {
            delete item;
        }
    }
    int from_json(jsini::Value& value) {
        int error = 0;
        if ((error = value["id"].to(id))) {
            return error;
        }
        return value["items"].to(items, [](jsini::Value& value, int& error) -> OrderItem* {
            if (value.is_null()) {
                return nullptr;
            }
            auto item = new OrderItem();
            error = item->from_json(value);
            return item;
        });
    }
};

}  // namespace ns1

static void test_vector_of_pointers() {
    jsini::Value value(std::string(R"json(
        {
            id: 100,
            items: [
                {
                    quantity: 10
                },
                {
                    quantity: 20
                },
                null
            ]
        }
    )json"));

    ns1::Order order;
    assert(order.from_json(value) == 0);
    assert(order.id == 100);
    assert(order.items.size() == 3);
    assert(order.items[0]->quantity == 10);
    assert(order.items[1]->quantity == 20);
    assert(order.items[2] == nullptr);
}

namespace ns2 {
struct OrderItem {
    int quantity = 0;
    int from_json(jsini::Value& value) {
        if (!value.is_object()) {
            return 1;
        }
        return value["quantity"].to(quantity);
    }
};

struct Order {
    int id;
    std::vector<OrderItem> items;
    int from_json(jsini::Value& value) {
        if (!value.is_object()) {
            return 1;
        }
        int error = 0;
        if ((error = value["id"].to(id))) {
            return error;
        }
        return value["items"].to(items, [](jsini::Value& value, int& error) {
            OrderItem item;
            error = item.from_json(value);
            return item;
        });
    }
};

}  // namespace ns2

static void test_vector_of_objects() {
    {
        jsini::Value value(std::string(R"json(
        {
            id: 100,
            items: [
                {
                    quantity: 10
                },
                {
                    quantity: 20
                },
            ]
        }
    )json"));

        ns2::Order order;
        assert(order.from_json(value) == 0);
        assert(order.id == 100);
        assert(order.items.size() == 2);
        assert(order.items[0].quantity == 10);
        assert(order.items[1].quantity == 20);
    }
    {
        jsini::Value value(std::string(R"json(
        {
            id: 100,
            items: [
                {
                    quantity: 10
                },
                {
                    qty: 20
                },
            ]
        }
    )json"));

        ns2::Order order;
        assert(order.from_json(value) == JSINI_ERROR);
        assert(order.id == 100);
        assert(order.items.size() == 2);
        assert(order.items[0].quantity == 10);
        assert(order.items[1].quantity == 0);
    }
}

static void test_vector() {
    test_vector_of_pointers();
    test_vector_of_objects();
}

namespace ns3 {

struct Order {
    int quantity;
    int from_json(jsini::Value& value) {
        if (!value.is_object()) {
            return 1;
        }
        return value["quantity"].to(quantity);
    }
};

struct Product {
    std::string code;
    std::vector<Order> orders;
    int from_json(jsini::Value& value) {
        if (!value.is_object()) {
            return 1;
        }
        int err = value["code"].to(code);
        if (err) return err;
        return value["orders"].to(orders, [](jsini::Value& value, int& error) {
            Order order;
            error = order.from_json(value);
            return order;
        });
    }
};

}  // namespace ns3

static void test_to_map() {
    std::string json_text = R"json(
            {
                'p-1': {
                    code: 'p-1',
                    orders: [
                        {
                            quantity: 10
                        },
                        {
                            quantity: 20
                        },
                    ]
                },
                'p-2': {
                    code: 'p-2',
                    orders: [
                        {
                            quantity: 30
                        },
                        {
                            quantity: 40
                        },
                    ]
                },
            })json";

    // 1) it parses objects
    {
        jsini::Value value(json_text);
        std::map<std::string, ns3::Product> map1;
        int err = value.to(map1, [](jsini::Value& value, int& error) {
            ns3::Product p;
            error = p.from_json(value);
            return p;
        });
        assert(err == 0);
        assert(map1.size() == 2);
        assert(map1["p-1"].orders[0].quantity == 10);
        assert(map1["p-2"].orders[1].quantity == 40);
    }

    // 2) it parses pointers
    {
        std::string json_text2 = json_text.substr(0, json_text.find_last_of('\n')) + R"json(
            ,"p-3": null})json";
        jsini::Value value(json_text2);
        std::map<std::string, ns3::Product*> map1;
        int err = value.to(map1, [](jsini::Value& value, int& error) {
            if (value.is_null()) {
                return (ns3::Product*)nullptr;
            }
            auto p = new ns3::Product();
            error = p->from_json(value);
            return p;
        });
        assert(err == 0);
        assert(map1.size() == 3);
        assert(map1["p-1"]->code == "p-1");
        assert(map1["p-2"]->orders[0].quantity == 30);
        const auto& it = map1.find("p-3");
        assert(it != map1.end());
        assert(it->second == nullptr);
        for (auto& it : map1) {
            delete it.second;
        }
    }

    // 3) it reports errors of wrong object types
    {
        std::string json_text2 = json_text.substr(0, json_text.find_last_of('\n')) + ",\"p-3\": []}";
        jsini::Value value(json_text2);
        std::map<std::string, ns3::Product*> map1;
        int err = value.to(map1, [](jsini::Value& value, int& error) {
            if (value.is_null()) {
                return (ns3::Product*)nullptr;
            }
            auto p = new ns3::Product();
            error = p->from_json(value);
            if (error) {
                delete p;
                return (ns3::Product*)nullptr;
            }
            return p;
        });
        assert(err != 0);
        const auto& it = map1.find("p-3");
        assert(it != map1.end());
        assert(it->second == nullptr);
        for (auto& it : map1) {
            delete it.second;
        }
    }
}

static void test_lineno() {
    std::string text = R"json({
        address: {
            country: "Australia",
            city:
                "Adelaide"
        },
        tags: [
            abc,
            def
        ]
    })json";
    jsini::Value value(text);
    assert(value.lineno() == 1);
    assert(value["address"].lineno() == 2);
    assert(value["tags"][1].lineno() == 9);

    auto it = value["address"].begin();
    it++;
    assert(it.key().lineno() == 4);
    assert(it.value().lineno() == 5);
}

extern "C" {
    void test_jsa();
    void test_jsh();
    void test_utf8();
}

int main(int argc, char** argv) {
    std::string spec(argc > 1 ? argv[1] : "");

    if (spec == "all" || spec == "jsa") {
        test_jsa();
    }

    if (spec == "all" || spec == "jsh") {
        test_jsh();
    }

    if (spec == "all" || spec == "utf8") {
        test_jsh();
    }

    if (spec == "all" || spec == "") {
        test_parsing();
        test_types();
        test_building();
        test_removing();
        test_iterating();
        test_dumping();
        test_type_cast();
        test_to();
        test_vector();
        test_to_map();
        test_lineno();
    }

    return 0;
}
