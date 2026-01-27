#include <assert.h>
#include <string.h>
#include <iostream>

#include "jsini.hpp"

// Replicating the static cleanup helper from src/util.c since it's not public
static void free_stats_entry(void* key, void* value) {
    free(key);
    jsini_free_key_stats((jsini_key_stats_t*)value);
}

void test_stats() {
    // Case 1: Simple object
    {
        std::string json = R"json({
            "name": "Alice",
            "age": 30,
            "city": "Wonderland"
        })json";

        jsini::Value val(json);
        jsh_t* stats = jsh_create_simple(32, 0);

        jsini_collect_key_stats(val.raw(), stats);

        // Check root stats
        jsini_key_stats_t* root_stats = (jsini_key_stats_t*)jsh_get(stats, "root");
        assert(root_stats != nullptr);
        assert(root_stats->object_count == 1);
        
        assert(jsh_get_int(root_stats->key_frequencies, "name") == 1);
        assert(jsh_get_int(root_stats->key_frequencies, "age") == 1);
        assert(jsh_get_int(root_stats->key_frequencies, "city") == 1);

        jsh_free_ex(stats, free_stats_entry);
    }

    // Case 2: Array of objects (common pattern)
    {
        std::string json = R"json([
            {"id": 1, "active": true},
            {"id": 2, "name": "Bob"},
            {"id": 3, "active": false, "role": "admin"}
        ])json";

        jsini::Value val(json);
        jsh_t* stats = jsh_create_simple(32, 0);

        jsini_collect_key_stats(val.raw(), stats);

        // Root stats
        jsini_key_stats_t* root_stats = (jsini_key_stats_t*)jsh_get(stats, "root");
        assert(root_stats != nullptr);
        assert(root_stats->object_count == 3); // 3 objects in the array
        
        assert(jsh_get_int(root_stats->key_frequencies, "id") == 3);
        assert(jsh_get_int(root_stats->key_frequencies, "active") == 2);
        assert(jsh_get_int(root_stats->key_frequencies, "name") == 1);
        assert(jsh_get_int(root_stats->key_frequencies, "role") == 1);

        jsh_free_ex(stats, free_stats_entry);
    }

    // Case 3: Nested objects
    {
        std::string json = R"json({
            "config": {
                "host": "localhost",
                "port": 8080
            },
            "users": [
                { "name": "A", "meta": { "login": "2023" } },
                { "name": "B", "meta": { "login": "2024", "admin": true } }
            ]
        })json";

        jsini::Value val(json);
        jsh_t* stats = jsh_create_simple(32, 0);

        jsini_collect_key_stats(val.raw(), stats);

        // Root object
        jsini_key_stats_t* root_stats = (jsini_key_stats_t*)jsh_get(stats, "root");
        assert(root_stats != nullptr);
        assert(root_stats->object_count == 1);

        // Nested "config" object
        jsini_key_stats_t* config_stats = (jsini_key_stats_t*)jsh_get(stats, "root.config");
        assert(config_stats != nullptr);
        assert(config_stats->object_count == 1);

        // Nested objects in "users" array. 
        jsini_key_stats_t* users_stats = (jsini_key_stats_t*)jsh_get(stats, "root.users");
        assert(users_stats != nullptr);
        assert(users_stats->object_count == 2);

        // Deeply nested "meta" objects.
        jsini_key_stats_t* meta_stats = (jsini_key_stats_t*)jsh_get(stats, "root.users.meta");
        assert(meta_stats != nullptr);
        assert(meta_stats->object_count == 2);

        jsh_free_ex(stats, free_stats_entry);
    }

    // Case 4: Print stats
    {
        std::string json = R"json({
            "users": [
                { "id": 1, "name": "Alice", "email": "alice@example.com" },
                { "id": 2, "name": "Bob" },
                { "id": 3, "name": "Charlie", "email": "charlie@example.com", "admin": true }
            ],
            "settings": {
                "theme": "dark",
                "notifications": {
                    "email": true,
                    "push": false
                }
            }
        })json";

        jsini::Value val(json);
        jsh_t* stats = jsh_create_simple(32, 0);

        jsini_collect_key_stats(val.raw(), stats);

        std::cout << "--- Visual Check: jsini_print_key_stats ---" << std::endl;
        jsini_print_key_stats(stdout, stats, -1, 0.0);
        std::cout << "-------------------------------------------" << std::endl;

        jsh_free_ex(stats, free_stats_entry);
    }

    // Case 5: Recursive-like structure (reusing keys)
    {
        std::string json = R"json({
            "node": {
                "name": "root",
                "node": {
                    "name": "child",
                    "node": {
                        "name": "grandchild"
                    }
                }
            }
        })json";

        jsini::Value val(json);
        jsh_t* stats = jsh_create_simple(32, 0);

        jsini_collect_key_stats(val.raw(), stats);

        std::cout << "--- Visual Check: Recursive keys ---" << std::endl;
        jsini_print_key_stats(stdout, stats, -1, 0.0);
        std::cout << "------------------------------------" << std::endl;

        jsh_free_ex(stats, free_stats_entry);
    }

    // Case 6: Separate path verification (book.name vs seller.name)
    {
        std::string json = R"json({
            "book": { "name": "Deep Work", "author": "Cal" },
            "seller": { "name": "Amazon", "rating": 5 }
        })json";

        jsini::Value val(json);
        jsh_t* stats = jsh_create_simple(32, 0);

        jsini_collect_key_stats(val.raw(), stats);

        std::cout << "--- Visual Check: book.name vs seller.name ---" << std::endl;
        jsini_print_key_stats(stdout, stats, -1, 0.0);
        std::cout << "----------------------------------------------" << std::endl;

        // Verify root contains book and seller
        jsini_key_stats_t* root_stats = (jsini_key_stats_t*)jsh_get(stats, "root");
        assert(jsh_exists(root_stats->key_frequencies, "book"));
        assert(jsh_exists(root_stats->key_frequencies, "seller"));

        // Verify book stats
        jsini_key_stats_t* book_stats = (jsini_key_stats_t*)jsh_get(stats, "root.book");
        assert(book_stats != nullptr);
        assert(jsh_exists(book_stats->key_frequencies, "name"));
        assert(jsh_exists(book_stats->key_frequencies, "author"));
        assert(!jsh_exists(book_stats->key_frequencies, "rating")); // rating belongs to seller

        // Verify seller stats
        jsini_key_stats_t* seller_stats = (jsini_key_stats_t*)jsh_get(stats, "root.seller");
        assert(seller_stats != nullptr);
        assert(jsh_exists(seller_stats->key_frequencies, "name"));
        assert(jsh_exists(seller_stats->key_frequencies, "rating"));
        assert(!jsh_exists(seller_stats->key_frequencies, "author")); // author belongs to book

        jsh_free_ex(stats, free_stats_entry);
    }
}

