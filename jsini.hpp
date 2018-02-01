// Copyright (c) Weidong Fang

#ifndef JSINI_HPP_
#define JSINI_HPP_

#include <string>
#include <cstring>
#include <iostream>

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

    operator const char *() const {
        jsini_value_t *value = node_->value();
        return ((jsini_string_t*) value)->data.data;
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

    Value& operator[](int index) {
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
#       if JSINI_KEEP_KEYS
        uint32_t data_;
#       else
        jsh_iterator_t *data_;
#       endif
        friend class Value;

    public:
        Iterator(Value *value=NULL) : value_(value), data_(0) {
        }

        Iterator& operator++(int) {
#           if JSINI_KEEP_KEYS
            data_++;
#           else
            jsh_t *map = ((jsini_object_t*) value_->node_->value())->map;
            data_ = (jsh_iterator_t*) jsh_next(map, data_);
#           endif
            return *this;
        }

        bool operator!=(const Iterator &other) const {
            return value_ != other.value_ || data_ != other.data_;
        }

        const char *key() const {
#           if JSINI_KEEP_KEYS
            jsini_object_t *obj = (jsini_object_t*) value_->node_->value();
            return ((jsini_attr_t*) jsa_get(&obj->data, data_))->name->data;
#           else
            return (const char *) data_->key;
#           endif
        }

        Value& value() const {
#           if JSINI_KEEP_KEYS
            return value_->operator[](key());
#           else
            return value_->operator[]((const char*) data_->key);
#           endif
        }
    };

    friend class Iterator;

    Iterator begin() {
        Value::Iterator it(this);
#       if JSINI_KEEP_KEYS
        it.data_ = 0;
#       else
        jsh_t *map = ((jsini_object_t*) node_->value())->map;
        it.data_ = (jsh_iterator_t*) jsh_first(map);
#       endif
        return it;
    }

    Iterator end() {
        Value::Iterator it(this);
#       if JSINI_KEEP_KEYS
        it.data_ = ((jsini_object_t*) node_->value())->data.size;
#       else
        it.data_ = NULL;
#       endif
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
