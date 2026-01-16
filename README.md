A C/C++ library that parses json-like (including json) data. It comes with a C++ wrapper that makes accessing/manipulating JSON data in C++ easy and fun.

## Features
- Very forgiving parser. Jsini parses a variety of json-link data that often appears in configuration files and other programming languages (in names like object, hash, dictionary, array, list, etc.). For example, the following data can be parsed properly:
  ```
  {
    database: {
        host = localhost
        port = 3306
        user = root
        password = secret
    }
    thread_count = 5
  }
  ```

- The order of object keys are kept after parsing. For example, the example above produces the following result:
  ```
  {
    "database": {
        "host": "localhost",
        "port": 3306,
        "user": "root",
        "password": ${MY_SECRET}
    },
    "thread_count": 5
  }
  ```
- Object keys can be optionally sorted when exporting (similar to Python's `sort_keys` option in their `json.dumps`).
- No dependencies. The library is written in pure C and only requires the standard C runtime.
- Reusable components. The hash table, array and string buffer in the source code can be used on their own without relying on other parts of the library. See `jsh.h`, `jsa.h` and `jsb.h` for their API.

## Usage
Because of its ease of use, the C++ wrapper is recommended whenever a suitable compiler is available (although currently it does not expose all functions provided by the library, e.g. parsing .ini files). The wrapper is fully contained in one header file `jsini.hpp`. See `tests/test.cpp` for usage examples.

## Parsing
To parse a string that contains json-like data pass it to the constructor of `jsini::Value`:
```cpp
std::string data = "[1, 2, {'a': 3, 'b': 'c'}]";
jsini::Value value(data);
assert(value[2]["b"] == "c");
```
The constructor of `jsini::Value` also accepts a file name (when its type is `const char*`):
```cpp
jsini::Value value("config.json");
assert(value["database"]["host"] == "localhost");
```

### Accessing
The following operators are overloaded to allow accessing object/array elements via subscripts:
```cpp
Value& operator[](const char *key);
Value& operator[](const std::string &key);
Value& operator[](int index);
```
Comparison/Assignment between a `jsini::Value` and an integer/boolean/float number/string are also possible:
```cpp
jsini::Value config("config.json");
assert(config["database"]["host"] == "localhost");
int port = config["database"]["port"];
config["thread_count"] = 8;
```

### Manipulating
Objects and arrays can be constructed from "undefined" values:
```cpp
// Constructing a JSON array
jsini::Value value;

value.push("Alice");
value.push("Bob");

assert(value.is_array());
assert(value.size() == 2);
assert(value[1] == std::string("Bob"));

// Constructing a JSON object object from scratch
jsini::Value config;

config["database"]["host"] = "localhost";
config["database"]["port"] = 3306
config["thread_count"] = 8;

assert(config.is_object());
assert(config.size() == 2);
assert(config["database"]["host"] == "localhost");
assert(config["thread_count"] == 8);
```
Deleting elements:

 ```cpp
// Removes the 2nd element of an array
array.remove(1);

// Removes 'thread_count' of config
config.remove("thread_count");
assert(config["thread_count"].is_undefined());
```

### Iterating
Arrays can be iterated using integer indexes:
```cpp
for (size_t i = 0; i < array.size(); i++) {
    jsini::Value &value = array[i];
    // Do something with value
    // ...
}
```

Objects can be traversed using iterators:

```cpp
jsini::Value::Iterator it = object.begin();
while (it != value.end()) {
    // Do something with it.key() or it.value()
    // ...
    it++;
}
```

### Stringifying
Values can be dumped (stringified) to `std::ostream` objects. The outputs of jsini are compliant with Javascript's `JSON.stringify()`.
```cpp
// Prints the string form of a JSON value to std::cout
value.dump(std::cout);

// Same as above but pretty-printed with an indent level of 4
value.dump(std::cout, JSINI_PRETTY_PRINT, 4);

// Same as above but with object keys sorted in output
value.dump(std::cout, JSINI_PRETTY_PRINT|JSINI_SORT_KEYS, 4);
```

## Bugs
Please report any bugs to https://github.com/fangwd/jsini/issues

## A note on the C++ wrapper

The C++ jsini::Value class uses a proxy pattern that relies on a Root object to manage the lifetime of all
sub-values. It favours safety and a "natural" syntax (preserving references) over raw speed. However, this
comes with a cost:

Every time you access a value using operator[] (e.g., value[i]), the following happens inside Root::allocate:
  1. Hash Map Lookup: It checks a hash map (node_map_) to see if a proxy for this specific element already exists.
  2. Double Allocation: If it's a new access, it performs two heap allocations:
      * new Node(...)
      * new Value(...)
  3. Hash Map Insertion: It inserts these new objects into the map.

```cpp
// internal helper in jsini.hpp
Value *allocate(jsini_value_t *container, uintptr_t index) {
    // ... lookup ...
    if (value == NULL) {
        Node *node = new Node(container, index); // Allocation #1
        value = new Value(this, node);           // Allocation #2
        jsh_put(node_map_, node, value);
    }
    return value;
}
```
In contrast, the C implementation jsini_aget simply performs a bounds check and returns a pointer from the array,
which incurs zero overhead.

For performance-critical loops, using the raw jsini_value_t* (accessible via value.raw()) or the C API directly
are recommended.
