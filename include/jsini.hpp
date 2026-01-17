// Copyright (c) Weidong Fang

#ifndef JSINI_HPP_
#define JSINI_HPP_

#include <string>
#include <cstring>
#include <iostream>
#include <vector>
#include <map>
#include <functional>

#include "jsini.h"

namespace jsini {

class Value {
private:
    static const uintptr_t INVALID_INDEX = (uintptr_t) -1;

    class Root;
    class Node;

private:
    Root *root_;
    Node *node_;

    Value(const Value &other);
    Value &operator=(const Value &other);

    Value(Root *root, Node *node) : root_(root), node_(node) {
    }

    void init(jsini_value_t *data) {
        if (!data) {
            data = jsini_alloc_undefined();
        }
        node_ = new Node(data);
        root_ = new Root(this);
    }

    jsini_array_t *cast_array() {
        jsini_value_t *value = node_->value();
        uint8_t type = value->type;

        if (type == JSINI_UNDEFINED || type == JSINI_TNULL
                || type == JSINI_TBOOL) {
            value = (jsini_value_t*) jsini_alloc_array();
            node_->set_value(value);
        }

        if (value->type != JSINI_TARRAY) {
            throw -1;
        }

        return (jsini_array_t *) value;
    }

    jsini_object_t *cast_object() {
        jsini_value_t *value = node_->value();
        uint8_t type = value->type;
        if (type == JSINI_UNDEFINED || type == JSINI_TNULL
                || type == JSINI_TBOOL) {
            value = (jsini_value_t*) jsini_alloc_object();
            node_->set_value(value);
        }

        if (value->type != JSINI_TOBJECT) {
            throw;
        }

        return (jsini_object_t *) value;
    }

    inline bool is_root() const{
        return node_->is_root();
    }

    inline bool is(uint8_t type) const {
        return this->type() == type;
    }

    friend class Root;

public:
    Value(jsini_value_t *data = NULL) {
        init(data);
    }

    Value(const char *filename) {
        init(jsini_parse_file(filename));
    }

    Value(const std::string &s) {
        init(jsini_parse_string(s.c_str(), s.length()));
    }

    Value(const char *s, size_t length) {
        init(jsini_parse_string(s, length));
    }

    static jsini_value_t* from_jsonl(const std::string &s) {
        return jsini_parse_string_jsonl(s.c_str(), s.length());
    }

    static jsini_value_t* from_jsonl_file(const std::string &filename) {
        return jsini_parse_file_jsonl(filename.c_str());
    }

    static int parse_jsonl_file(const std::string &filename, std::function<int(Value&)> cb) {
        struct Context {
            std::function<int(Value&)> cb;
            Root *root;
        } ctx = { cb, nullptr };

        jsini_jsonl_cb c_cb = [](jsini_value_t *v, void *data) -> int {
            Context *c = (Context*)data;
            Value val(v);
            int res = c->cb(val);
            return res;
        };

        return jsini_parse_file_jsonl_ex(filename.c_str(), c_cb, &ctx);
    }

    ~Value() {
        if (node_->is_root()) {
            jsini_free(node_->value());
            delete root_;
        }
        delete node_;
    }

    inline uint8_t type() const {
        return node_->value()->type;
    }

    inline bool is_null() const {
        return is(JSINI_TNULL);
    }

    inline bool is_bool() const {
        return is(JSINI_TBOOL);
    }

    inline bool is_integer() const {
        return is(JSINI_TINTEGER);
    }

    inline bool is_number() const {
        return is(JSINI_TINTEGER) || is(JSINI_TNUMBER);
    }

    inline bool is_string() const {
        return is(JSINI_TSTRING);
    }

    inline bool is_array() const {
        return is(JSINI_TARRAY);
    }

    inline bool is_object() const {
        return is(JSINI_TOBJECT);
    }

    inline bool is_undefined() const {
        return is(JSINI_UNDEFINED);
    }

    inline uint32_t lineno() const {
        return node_->value()->lineno;
    }

    size_t size() const {
        jsini_value_t *value = node_->value();
        switch (value->type) {
        case JSINI_TARRAY:
            return jsini_array_size((jsini_array_t* )value);
        case JSINI_TOBJECT:
            return jsini_object_size((jsini_object_t* )value);
        default:
            return 0;
        }
    }

    operator bool() const {
        jsini_value_t *value = node_->value();
        switch (value->type) {
        case JSINI_UNDEFINED:
        case JSINI_TNULL:
            return false;
        case JSINI_TBOOL:
            return (bool) ((jsini_bool_t*) value)->data;
        case JSINI_TINTEGER:
            return ((jsini_integer_t*) value)->data != 0;
        case JSINI_TNUMBER:
            return ((jsini_number_t*) value)->data != 0.0;
        default:
            return true;
        }
    }

    operator int() const {
        jsini_value_t *value = node_->value();
        return jsini_cast_int(value);
    }

    operator double() const {
        jsini_value_t *value = node_->value();
        return jsini_cast_double(value);
    }

    operator float() const {
        jsini_value_t *value = node_->value();
        return (float) jsini_cast_double(value);
    }

    operator const char *() const {
        jsini_value_t *value = node_->value();
        if (value->type != JSINI_TSTRING) {
            return nullptr;
        }
        return ((jsini_string_t*) value)->data.data;
    }

    operator std::string() const {
        std::string s;
        jsini_value_t *value = node_->value();
        if (value->type != JSINI_TSTRING) {
            return s;
        }
        jsb_t* sb = &((jsini_string_t*) value)->data;
        s.append(sb->data, sb->size);
        return s;
    }

    int to(bool &dst) const {
        jsini_value_t *value = node_->value();
        if (value->type != JSINI_TBOOL) {
            return JSINI_ERROR;
        }
        dst = (bool)((jsini_bool_t *)value)->data;
        return JSINI_OK;
    }

    template<class T>
    int to(T &dst) const {
        if (type() == JSINI_TINTEGER) {
            dst = (T)((jsini_integer_t *)node_->value())->data;
            return JSINI_OK;
        }
        return JSINI_ERROR;
    }

    int to(float &dst) const {
        double dval;
        if (int error = to(dval)) {
            return error;
        }
        dst = (float)dval;
        return JSINI_OK;
    }

    int to(double &dst) const {
        jsini_value_t *value = node_->value();
        if (value->type != JSINI_TINTEGER && value->type != JSINI_TNUMBER) {
            return JSINI_ERROR;
        }
        dst = jsini_cast_double(value);
        return JSINI_OK;
    }

    int to(const char *&dst) const {
        jsini_value_t *value = node_->value();
        if (value->type == JSINI_TNULL) {
            dst = nullptr;
            return JSINI_OK;
        }
        if (value->type != JSINI_TSTRING) {
            return JSINI_ERROR;
        }
        dst = ((jsini_string_t *)value)->data.data;
        return JSINI_OK;
    }

    int to(std::string &dst) const {
        jsini_value_t *value = node_->value();
        if (value->type != JSINI_TSTRING) {
            return JSINI_ERROR;
        }
        dst.clear();
        const jsb_t *sb = &((jsini_string_t *)value)->data;
        dst.append(sb->data, sb->size);
        return JSINI_OK;
    }

    template <class V, class F>
    int to(std::vector<V> &dst, F parse) {
        if (type() != JSINI_TARRAY) {
            return JSINI_ERROR;
        }
        dst.clear();
        int err = 0;
        for (size_t i = 0; i < size(); i++) {
            dst.push_back(parse(get(i), err));
            if (err != JSINI_OK) {
                return err;
            }
        }
        return JSINI_OK;
    }

    template <class V, class F, class K=std::string>
    int to(std::map<K,V> &dst, F parse) {
        if (type() != JSINI_TOBJECT) {
            return JSINI_ERROR;
        }
        dst.clear();
        int err = 0;
        for (Value::Iterator it = begin(); it != end(); it++) {
            dst[K(it.key())] = parse(it.value(), err);
            if (err != JSINI_OK) {
                return err;
            }
        }
        return JSINI_OK;
    }

    inline jsini_value_t *raw() {
        return node_->value();
    }

    jsini_value_t *clone() {
        auto value = node_->value();
        if (value && value->type != JSINI_UNDEFINED) {
            jsb_t sb;
            jsb_init(&sb);
            jsini_stringify(value, &sb, 0, 0);
            auto result = jsini_parse_string(sb.data, sb.size);
            jsb_clean(&sb);
            return result;
        }
        return nullptr;
    }

    bool operator==(bool value) {
        return this->operator bool() == value;
    }

    bool operator==(int value) {
        return this->operator int() == value;
    }

    bool operator==(double value) {
        return this->operator double() == value;
    }

    bool operator==(const char *value) {
        const char *data = this->operator const char *();
        if (data && value) {
            return strcmp(data, value) == 0;
        }
        return false;
    }

    bool operator==(const Value &other) {
        if (type() != other.type()) {
            return false;
        }
        switch (type()) {
        case JSINI_UNDEFINED:
        case JSINI_TNULL:
            return true;
        case JSINI_TBOOL:
            return operator==(other.operator bool());
        case JSINI_TINTEGER:
            return operator==(other.operator int());
        case JSINI_TNUMBER:
            return operator==(other.operator double());
        case JSINI_TSTRING:
            return operator==(other.operator const char *());
        default:
            return false;
        }
    }

    Value& operator=(bool data) {
        jsini_value_t *value = node_->value();
        if (value->type == JSINI_TBOOL) {
            ((jsini_bool_t*) value)->data = data;
        } else {
            value = (jsini_value_t*) jsini_alloc_bool((int) data);
            node_->set_value(value);
        }
        return *this;
    }

    Value& operator=(int data) {
        jsini_value_t *value = node_->value();
        if (value->type == JSINI_TINTEGER) {
            ((jsini_integer_t*) value)->data = data;
        } else {
            value = (jsini_value_t*) jsini_alloc_integer(data);
            node_->set_value(value);
        }
        return *this;
    }

    Value& operator=(double data) {
        jsini_value_t *value = node_->value();
        if (value->type == JSINI_TNUMBER) {
            ((jsini_number_t*) value)->data = data;
        } else {
            value = (jsini_value_t*) jsini_alloc_number(data);
            node_->set_value(value);
        }
        return *this;
    }

    Value& operator=(const char *data) {
        jsini_value_t *value = node_->value();
        if (data) {
            if (value->type == JSINI_TSTRING) {
                jsb_t *sb = &((jsini_string_t*) value)->data;
                jsb_clear(sb);
                jsb_append(sb, data, strlen(data));
            } else {
                value = (jsini_value_t*) jsini_alloc_string(data, strlen(data));
                node_->set_value(value);
            }
        } else {
            value = (jsini_value_t*) jsini_alloc_null();
            node_->set_value(value);
        }
        return *this;
    }

    Value& operator[](uint32_t index) {
        return get(index);
    }

    Value& get(uint32_t index) {
        jsini_array_t *array = cast_array();

        if (index >= jsini_array_size(array)) {
            uint32_t size = jsini_array_size(array);
            jsini_array_resize(array, index + 1);
            for (uint32_t i = size; i <= index; i++) {
                jsini_array_set(array, i, jsini_alloc_undefined());
            }
        }

        return *root_->allocate((jsini_value_t*) array, index);
    }

    Value& operator[](const std::string &key) {
        return operator[](key.c_str());
    }

    Value& operator[](const char *key) {
        jsini_object_t *object = cast_object();

        const jsh_iterator_t *it = jsh_find(object->map, key);
        if (it == NULL) {
            jsini_set_undefined(object, key);
            it = jsh_find(object->map, key);
        }

        return *root_->allocate((jsini_value_t*) object, (uintptr_t) it->key);
    }

    Value &push(bool value)  {
        jsini_array_t *array = cast_array();
        uintptr_t index = (uintptr_t)jsini_array_size(array);
        jsini_push_bool(array, (int)value);
        return *root_->allocate((jsini_value_t*)array, index);
    }

    Value &push(int value) {
        jsini_array_t *array = cast_array();
        uintptr_t index = (uintptr_t)jsini_array_size(array);
        jsini_push_integer(array, value);
        return *root_->allocate((jsini_value_t*)array, index);
    }

    Value &push(double value) {
        jsini_array_t *array = cast_array();
        uintptr_t index = (uintptr_t) jsini_array_size(array);
        jsini_push_number(array, value);
        return *root_->allocate((jsini_value_t*) array, index);
    }

    Value &push(const char *s) {
        jsini_array_t *array = cast_array();
        uintptr_t index = (uintptr_t) jsini_array_size(array);
        jsini_push_string(array, s, strlen(s));
        return *root_->allocate((jsini_value_t*) array, index);
    }

    void remove(int index) {
        root_->remove(node_->value(), index);
    }

    void remove(const char *key) {
        root_->remove(node_->value(), (uintptr_t) key);
    }

    void dump(std::ostream& os = std::cout, int options = 0,
            int indent = 0) const {
        jsb_t *sb = jsb_create();
        if (indent > 0) options |= JSINI_PRETTY_PRINT;
        jsini_stringify(node_->value(), sb, options, indent);
        os << sb->data;
        jsb_free(sb);
    }

    class Iterator {
    private:
        Value *value_;
        uint32_t data_;
        friend class Value;

    public:
        Iterator(Value *value=NULL) : value_(value), data_{0} {
        }

        Iterator& operator++(int) {
            data_++;
            return *this;
        }

        bool operator!=(const Iterator &other) const {
            return value_ != other.value_ || data_ != other.data_;
        }

        class Key {
        private:
            const jsini_string_t *data_;

        public:
            Key(const jsini_string_t *data) : data_(data) {
            }

            inline uint32_t lineno() const {
                return data_->lineno;
            }

            inline bool operator==(const char *value) const {
                return value ? strcmp(data_->data.data, value) == 0 : false;
            }

            inline bool operator==(const std::string &value) const {
                return value == data_->data.data;
            }

            inline operator const char *() const {
                return data_->data.data;
            }
        };

        Key key() const {
            jsini_object_t *obj = (jsini_object_t*) value_->node_->value();
            return Key(((jsini_attr_t*) jsa_get(&obj->keys, data_))->name);
        }

        Value& value() const {
            return value_->operator[]((const char *)key());
        }
    };

    friend class Iterator;

    Iterator begin() {
        Value::Iterator it(this);
        it.data_ = 0;
        return it;
    }

    Iterator end() {
        Value::Iterator it(this);
        it.data_ = ((jsini_object_t*) node_->value())->keys.size;
        return it;
    }

private:
    class Node {
    private:
        jsini_value_t *container_;
        uintptr_t index_;

        friend class Root;

        static uint32_t Hash(const void *p) {
            const Node *key = (const Node *) p;
            return (uintptr_t) ((uintptr_t) key->container_ * key->index_);
        }

        static int Compare(const void *p1, const void *p2) {
            const Node *key1 = (const Node *) p1;
            const Node *key2 = (const Node *) p2;
            return key1->container_ == key2->container_
                    && key1->index_ == key2->index_;
        }

    public:
        Node(jsini_value_t *container, uintptr_t index = INVALID_INDEX) {
            container_ = container;
            index_ = index;
        }

        jsini_value_t *value() {
            if (is_root()) {
                return container_;
            }

            switch (container_->type) {
            case JSINI_TARRAY:
                return jsini_aget((jsini_array_t * )(container_), index_);
            case JSINI_TOBJECT:
                return jsini_get_value((jsini_object_t *) (container_),
                        (const char *) index_);
            default:
                return container_;
            }
        }

        void set_value(jsini_value_t *data) {
            if (is_root()) {
                jsini_free(container_);
                container_ = data;
                return;
            }

            switch (container_->type) {
            case JSINI_TARRAY:
                jsini_array_set((jsini_array_t *) (container_), index_, data);
                break;
            case JSINI_TOBJECT:
                jsini_set_value((jsini_object_t *) (container_),
                        (const char *) index_, data);
                break;
            default:
                jsini_free(container_);
                container_ = data;
            }

        }

        bool is_root() const {
            return index_ == INVALID_INDEX;
        }

    };

    class Root {
    private:
        jsh_t *node_map_;

    public:
        Root(Value *value) {
            node_map_ = jsh_create(8192, Node::Hash, Node::Compare, 0.7);
        }

        ~Root() {
            const jsh_iterator_t *it = jsh_first(node_map_);
            while (it != NULL) {
                delete (Value*) it->value;
                it = jsh_next(node_map_, it);
            }
            jsh_free(node_map_);
        }

        Value *allocate(jsini_value_t *container, uintptr_t index) {
            Node key(container, index);
            Value *value = (Value*) jsh_get(node_map_, &key);
            if (value == NULL) {
                Node *node = new Node(container, index);
                value = new Value(this, node);
                jsh_put(node_map_, node, value);
            }
            return value;
        }

        void remove(jsini_value_t *container, uintptr_t index) {
            Node key(container, index);

            const jsh_iterator_t *it = jsh_find(node_map_, &key);
            if (it != NULL) {
                jsa_t *containers = jsa_create();
                jsa_t *values = jsa_create();

                jsa_push(containers, key.value());

                while (jsa_size(containers) > 0) {
                    jsini_value_t *container = (jsini_value_t*) jsa_pop(
                            containers);
                    const jsh_iterator_t * it = jsh_first(node_map_);
                    while (it != NULL) {
                        Node *node = (Node*) it->key;
                        if (node->container_ == container) {
                            jsa_push(containers, node->value());
                            jsa_push(values, it->value);
                        }
                        it = jsh_next(node_map_, it);
                    }
                }

                for (uint32_t i = 0; i < jsa_size(values); i++) {
                    Value *value = (Value*) jsa_get(values, i);
                    jsh_remove(node_map_, value->node_);
                    delete value;
                }

                jsa_free(containers);
                jsa_free(values);
            }

            switch (container->type) {
            case JSINI_TARRAY:
                jsini_array_remove((jsini_array_t*) container,
                        (uint32_t) index);
                break;
            case JSINI_TOBJECT:
                jsini_remove((jsini_object_t*) container, (const char *) index);
                break;
            default:
                break;
            }

            if (it != NULL) {
                Value *value = (Value*) it->value;
                jsh_remove(node_map_, &key);
                delete value;
            }
        }

    };
};

} // namespace jsini

#endif // JSINI_HPP_
