#include "jsini.hpp"
#include <assert.h>

extern "C" jsini_value_t *jsini_read_json(jsl_t *lex);

static void test_read_env_vars() {
    setenv("JSINI_TEST_PASSWORD", "secret", 1);

    std::string big_key("JSINI_TEST_");
    while (big_key.size() < 255) {
        big_key.push_back('X');
    }
    setenv(big_key.data(), "hello", 1);

    jsl_t lex;
    {
        const char *code = R"json(`password ${JSINI_TEST_PASSWORD}`)json";
        jsl_init(&lex, code, strlen(code), 0);
        auto js = jsl_read_json_string(&lex);
        assert(!strcmp(js->data.data, "password secret"));
        jsini_free_string(js);
    }

    {
        const char *code = R"json(`password $JSINI_TEST_PASSWORD}`)json";
        jsl_init(&lex, code, strlen(code), 0);
        auto js = jsl_read_json_string(&lex);
        assert(!strcmp(js->data.data, "password $JSINI_TEST_PASSWORD}"));
        jsini_free_string(js);
    }

    {
        const char *code = R"json(`password ${JSINI_TEST_PASSWORD `)json";
        jsl_init(&lex, code, strlen(code), 0);
        auto js = jsl_read_json_string(&lex);
        assert(!strcmp(js->data.data, "password ${JSINI_TEST_PASSWORD "));
        jsini_free_string(js);
    }

    {
        std::string code = "`message ${";
        code += big_key;
        code += "X}` 123";
        jsl_init(&lex, code.data(), code.size(), 0);
        auto js = jsl_read_json_string(&lex);
        assert(!strcmp(js->data.data, "message hello"));
        jsini_free_string(js);

        jsl_skip_space(&lex, nullptr);

        auto val = jsl_read_primitive(&lex);
        assert(val->type == JSINI_TINTEGER);
        assert(((jsini_integer_t*)val)->data == 123);
        jsini_free(val);
    }

    {
        const char *code = R"json(${JSINI_TEST_PASSWORD} 123)json";
        jsl_init(&lex, code, strlen(code), 0);
        auto js = (jsini_string_t *)jsini_read_json(&lex);
        assert(!strcmp(js->data.data, "secret"));
        jsini_free_string(js);
        assert(!strcmp(lex.input, " 123"));
    }
    {
        std::string text = R"json({
          # some comment
          `the ${JSINI_TEST_PASSWORD}`: none,
          password: ${JSINI_TEST_PASSWORD}, # secret
          message: `hello`
        })json";
        jsini::Value value(text);
        assert(value.is_object());
        assert(value["the secret"] == "none");
        assert(value["password"] == "secret");
        assert(value["message"] == "hello");
    }
}

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
  test_read_env_vars();
}