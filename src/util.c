#include <stdlib.h>
#include <string.h>

#include "jsini.h"

#define xmalloc malloc
#define xfree free

static jsini_key_stats_t* get_or_create_stats(jsini_key_stats_map_t* stats, const char* key) {
    jsini_key_stats_t* entry = jsh_get(stats, key);
    if (!entry) {
        entry = jsini_alloc_key_stats();
        jsh_put(stats, strdup(key), entry);
    }
    return entry;
}

static void jsini_collect_key_stats_internal(jsini_value_t* value, jsini_key_stats_map_t* stats,
                                             const char* current_path);

static void jsini_collect_key_stats_from_object(jsini_object_t* obj, jsini_key_stats_map_t* stats,
                                                const char* current_path) {
    jsini_key_stats_t* entry = get_or_create_stats(stats, current_path);
    entry->object_count++;

    if (!entry->key_frequencies) {
        entry->key_frequencies = jsh_create_simple(32, 0);
    }

    const jsh_iterator_t* it;
    for (it = jsh_first(obj->map); it; it = jsh_next(obj->map, it)) {
        const char* key = jsini_iter_key(it);
        jsini_value_t* val = jsini_iter_value(it);

        // Update frequency for the local key
        int count = jsh_get_int(entry->key_frequencies, key);
        if (count == 0 && !jsh_exists(entry->key_frequencies, key)) {
            jsh_put_int(entry->key_frequencies, strdup(key), 1);
        } else {
            jsh_put_int(entry->key_frequencies, key, count + 1);
        }

        // Build child path: current_path + "." + key
        size_t path_len = strlen(current_path) + strlen(key) + 2;
        char* child_path = (char*)xmalloc(path_len);
        sprintf(child_path, "%s.%s", current_path, key);

        jsini_collect_key_stats_internal(val, stats, child_path);
        xfree(child_path);
    }
}

static void jsini_collect_key_stats_from_array(jsini_array_t* arr, jsini_key_stats_map_t* stats,
                                               const char* current_path) {
    size_t size = jsini_array_size(arr);
    for (size_t i = 0; i < size; i++) {
        // Elements in an array share the same path context as the array itself
        // (We treat the array as a transparent container for the objects' schema)
        jsini_collect_key_stats_internal(jsini_aget(arr, i), stats, current_path);
    }
}

static void jsini_collect_key_stats_internal(jsini_value_t* value, jsini_key_stats_map_t* stats,
                                             const char* current_path) {
    if (!value) return;

    switch (value->type) {
        case JSINI_TOBJECT:
            jsini_collect_key_stats_from_object((jsini_object_t*)value, stats, current_path);
            break;
        case JSINI_TARRAY:
            jsini_collect_key_stats_from_array((jsini_array_t*)value, stats, current_path);
            break;
        default:
            break;
    }
}

void jsini_collect_key_stats(jsini_value_t* value, jsini_key_stats_map_t* stats) {
    jsini_collect_key_stats_internal(value, stats, "root");
}

jsini_key_stats_t* jsini_alloc_key_stats() {
    jsini_key_stats_t* stats = (jsini_key_stats_t*)xmalloc(sizeof(jsini_key_stats_t));
    stats->object_count = 0;
    stats->key_frequencies = NULL;
    return stats;
}

static void free_stats_entry(void* key, void* value) {
    free(key);
    jsini_free_key_stats((jsini_key_stats_t*)value);
}

static void free_key(void* key, void* value) {
    free(key);
}

void jsini_free_key_stats(jsini_key_stats_t* stats) {
    jsh_t* ht = stats->key_frequencies;
    if (ht) {
        jsh_free_ex(ht, free_key);
    }
    xfree(stats);
}

typedef struct {
    const char* key;
    int frequency;
} key_freq_entry_t;

static int compare_key_freq(const void* a, const void* b) {
    const key_freq_entry_t* e1 = *(const key_freq_entry_t**)a;
    const key_freq_entry_t* e2 = *(const key_freq_entry_t**)b;
    // Sort by frequency descending, then by key ascending
    if (e2->frequency != e1->frequency) {
        return e2->frequency - e1->frequency;
    }
    return strcmp(e1->key, e2->key);
}

static int compare_strings(const void* a, const void* b) {
    const char* s1 = *(const char**)a;
    const char* s2 = *(const char**)b;
    return strcmp(s1, s2);
}

static void print_stats_tree(FILE* out, jsini_key_stats_map_t* stats, const char* current_path,
                             char* prefix, int current_level, int max_level, double min_ratio) {
    jsini_key_stats_t* entry = (jsini_key_stats_t*)jsh_get(stats, current_path);
    if (!entry || !entry->key_frequencies) return;

    // Stop if we've reached max_level (unless max_level is -1 for unlimited)
    if (max_level >= 0 && current_level > max_level) return;

    jsa_t* keys = jsa_create();
    const jsh_iterator_t* it;
    for (it = jsh_first(entry->key_frequencies); it; it = jsh_next(entry->key_frequencies, it)) {
        key_freq_entry_t* fe = (key_freq_entry_t*)xmalloc(sizeof(key_freq_entry_t));
        fe->key = (const char*)it->key;
        fe->frequency = jsh_get_int(entry->key_frequencies, fe->key);
        jsa_push(keys, fe);
    }

    qsort(keys->item, keys->size, sizeof(keys->item[0]), compare_key_freq);

    size_t prefix_len = strlen(prefix);

    for (uint32_t i = 0; i < keys->size; i++) {
        key_freq_entry_t* fe = (key_freq_entry_t*)jsa_get(keys, i);

        double ratio = (double)fe->frequency / (double)entry->object_count;

        // Skip entries below minimum ratio threshold
        if (ratio < min_ratio) {
            xfree(fe);
            continue;
        }

        int is_last = (i == keys->size - 1);

        fprintf(out, "%s%s── %s", prefix, is_last ? "└" : "├", fe->key);

        double percentage = ratio * 100.0;
        fprintf(out, " (freq %d, %.1f%%)\n", fe->frequency, percentage);

        // Build child path to look up in the global stats map
        size_t child_path_len = strlen(current_path) + strlen(fe->key) + 2;
        char* child_path = (char*)xmalloc(child_path_len);
        sprintf(child_path, "%s.%s", current_path, fe->key);

        if (jsh_exists(stats, child_path)) {
            char* new_prefix = (char*)xmalloc(prefix_len + 5);
            sprintf(new_prefix, "%s%s   ", prefix, is_last ? " " : "│");
            print_stats_tree(out, stats, child_path, new_prefix, current_level + 1, max_level, min_ratio);
            xfree(new_prefix);
        }

        xfree(child_path);
        xfree(fe);
    }

    jsa_free(keys);
}

void jsini_print_key_stats(FILE* out, jsini_key_stats_map_t* stats, int max_level, double min_ratio) {
    if (!stats) return;

    jsini_key_stats_t* root_entry = (jsini_key_stats_t*)jsh_get(stats, "root");
    if (!root_entry) return;

    fprintf(out, ". (root, %zu objects)\n", root_entry->object_count);
    char prefix[1] = "";
    print_stats_tree(out, stats, "root", prefix, 0, max_level, min_ratio);
}

void jsini_free_key_stats_map(jsini_key_stats_map_t* stats) {
    jsh_free_ex(stats, free_stats_entry);
}


