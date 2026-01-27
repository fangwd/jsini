/*
 * Copyright (c) Weidong Fang
 */

#include "jsini.h"

#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

typedef struct {
    jsini_key_stats_map_t *stats;
    size_t line_count;
} stats_context_t;

static int stats_callback(jsini_value_t *value, void *user_data) {
    stats_context_t *ctx = (stats_context_t *)user_data;
    jsini_collect_key_stats(value, ctx->stats);
    jsini_free(value);

    ctx->line_count++;
    if (ctx->line_count % 1000 == 0) {
        fprintf(stderr, "Processed %zu lines\n", ctx->line_count);
    }

    return JSINI_OK;
}

int main(int argc, char **argv) {
    int print_options = 0;
    int parse_ini = 0;
    int parse_jsonl = 0;
    int parse_csv = 0;
    int replace = 0;
    int print_stats = 0;
    int max_level = -1;  // -1 means unlimited
    double min_ratio = 0.0;  // 0.0 means no minimum
    const char *key = NULL;

    while (1) {
       static struct option options[] = {
           {"ascii",       no_argument,        0, 'a'},
           {"from",        required_argument,  0, 'f'},
           {"ini",         no_argument,        0, 'i'},
           {"jsonl",       no_argument,        0, 'L'},
           {"csv",         no_argument,        0, 'c'},
           {"key",         required_argument,  0, 'k'},
           {"to",          required_argument,  0, 't'},
           {"output",      required_argument,  0, 'o'},
           {"pretty",      no_argument,        0, 'p'},
           {"sort",        no_argument,        0, 'S'},
           {"replace",     no_argument,        0, 'r'},
           {"stats",       no_argument,        0, 's'},
           {"level",       required_argument,  0, 'l'},
           {"min-ratio",   required_argument,  0, 'm'},
           {0, 0, 0, 0}
       };

       static const char *opts = "af:g:k:iLct:o:prSsl:m:";

       int n = 0;
       int c = getopt_long (argc, argv, opts, options, &n);

       if (c == -1) break;

       switch (c) {
       case 'a':
           print_options |= JSINI_SORT_KEYS;
           break;
       case 'f':
           break;
       case 'i':
           parse_ini = 1;
           break;
       case 'L':
           parse_jsonl = 1;
           break;
       case 'c':
           parse_csv = 1;
           break;
        case 'k':
           key = optarg;
           break;
       case 't':
           break;
       case 'o':
           break;
       case 'p':
           print_options |= JSINI_PRETTY_PRINT;
           break;
       case 'S':
           print_options |= JSINI_SORT_KEYS;
           break;
       case 'r':
           replace = 1;
           break;
       case 's':
           print_stats = 1;
           break;
       case 'l':
           max_level = atoi(optarg);
           break;
       case 'm':
           min_ratio = atof(optarg);
           break;
       default:
           return 1;
       }
    }

    if (optind < argc) {
        const char *file = argv[optind++];

        if (print_stats && (parse_jsonl || parse_csv)) {
            jsini_key_stats_map_t *stats = jsh_create_simple(0, 0);
            stats_context_t ctx = { stats, 0 };
            if (parse_jsonl) {
                jsini_parse_file_jsonl_ex(file, stats_callback, &ctx);
            } else {
                jsini_parse_file_csv_ex(file, JSINI_CSV_DEFAULT, stats_callback, &ctx);
            }
            if (ctx.line_count > 0) {
                fprintf(stderr, "Total: %zu lines processed\n", ctx.line_count);
            }
            jsini_print_key_stats(stdout, stats, max_level, min_ratio);
            jsini_free_key_stats_map(stats);
            return 0;
        }

        jsini_value_t *value = parse_ini
                                 ? jsini_parse_file_ini(file)
                                 : (parse_jsonl
                                    ? jsini_parse_file_jsonl(file)
                                    : (parse_csv
                                        ? jsini_parse_file_csv(file)
                                        : jsini_parse_file(file)));
        if (value != NULL)
        {
            if (print_stats) {
                jsini_key_stats_map_t *stats = jsh_create_simple(0, 0);
                jsini_collect_key_stats(value, stats);
                jsini_print_key_stats(stdout, stats, max_level, min_ratio);
                jsini_free_key_stats_map(stats);
            }
            else if (key) {
                jsini_print(stdout, jsini_select((jsini_object_t*)value, key), 0);
            }
            else {
                if (replace) {
                    FILE *fp = fopen(file, "w");
                    if (!fp) {
                        fprintf(stderr, "Can't open %s (%s)\n", file, strerror(errno));
                        return 1;
                    }
                    jsini_print(fp, value, print_options);
                    fclose(fp);
                }
                else {
                    jsini_print(stdout, value, print_options);
                }
            }
            jsini_free(value);
        }
    }
    else {
      char buf[512];
      int n;
      jsb_t *sb = jsb_create();
      while((n = fread(buf, 1, sizeof(buf), stdin)) > 0) {
        jsb_append(sb, buf, n);
      }
      jsini_value_t *value = parse_jsonl ? jsini_parse_string_jsonl(sb->data, sb->size)
                             : parse_csv ? jsini_parse_string_csv(sb->data, sb->size)
                                         : jsini_parse_string(sb->data, sb->size);
      if (value != NULL) {
          if (print_stats) {
              jsini_key_stats_map_t *stats = jsh_create_simple(0, 0);
              jsini_collect_key_stats(value, stats);
              jsini_print_key_stats(stdout, stats, max_level, min_ratio);
              jsini_free_key_stats_map(stats);
          }
          else {
              jsini_print(stdout, value, print_options);
          }
          jsini_free(value);
      }
      jsb_free(sb);
    }


    return 0;
}
