#include <float.h>
#include <inttypes.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "betree.h"
#include "debug.h"
#include "hashmap.h"
#include "minunit.h"
#include "utils.h"

#define MAX_EXPRS 5000
#define MAX_EVENTS 1000

struct events {
    size_t count;
    struct event** events;
};

extern bool MATCH_NODE_DEBUG;

struct value_bound get_integer_events_bound(betree_var_t var, struct event** events, size_t event_count) 
{
    struct value_bound bound = { .imin = INT64_MAX, .imax = INT64_MIN };
    for(size_t i = 0; i < event_count; i++) {
        struct event* event = events[i];
        for(size_t j = 0; j < event->pred_count; j++) {
            struct pred* pred = event->preds[j];
            if(pred->attr_var.var == var) {
                if(pred->value.ivalue < bound.imin) {
                    bound.imin = pred->value.ivalue;
                }
                if(pred->value.ivalue > bound.imax) {
                    bound.imax = pred->value.ivalue;
                }
                break;
            }
        }
    }
    return bound;
}

struct value_bound get_float_events_bound(betree_var_t var, struct event** events, size_t event_count) 
{
    struct value_bound bound = { .fmin = DBL_MAX, .fmax = -DBL_MAX };
    for(size_t i = 0; i < event_count; i++) {
        struct event* event = events[i];
        for(size_t j = 0; j < event->pred_count; j++) {
            struct pred* pred = event->preds[j];
            if(pred->attr_var.var == var) {
                if(pred->value.fvalue < bound.fmin) {
                    bound.fmin = pred->value.fvalue;
                }
                if(pred->value.fvalue > bound.fmax) {
                    bound.fmax = pred->value.fvalue;
                }
                break;
            }
        }
    }
    return bound;
}

void add_event(struct event* event, struct events* events)
{
    if(events->count == 0) {
        events->events = calloc(1, sizeof(*events->events));
        if(events == NULL) {
            fprintf(stderr, "%s calloc failed", __func__);
            abort();
        }
    }
    else {
        struct event** new_events
            = realloc(events->events, sizeof(*new_events) * ((events->count) + 1));
        if(new_events == NULL) {
            fprintf(stderr, "%s realloc failed", __func__);
            abort();
        }
        events->events = new_events;
    }
    events->events[events->count] = event;
    events->count++;
}

char* strip_chars(const char* string, const char* chars)
{
    char* new_string = malloc(strlen(string) + 1);
    int counter = 0;

    for(; *string; string++) {
        if(!strchr(chars, *string)) {
            new_string[counter] = *string;
            ++counter;
        }
    }
    new_string[counter] = 0;
    return new_string;
}

int event_parse(const char* text, struct event** event);

size_t read_betree_events(struct config* config, struct events* events)
{
    FILE* f = fopen("betree_events", "r");
    size_t count = 0;

    char line[LINE_MAX * 8];
    while(fgets(line, sizeof(line), f)) {
        if(MAX_EVENTS != 0 && events->count == MAX_EVENTS) {
            break;
        }

        struct event* event;
        if(event_parse(line, &event) != 0) {
            fprintf(stderr, "Can't parse event %zu: %s", events->count + 1, line);
            abort();
        }

        fill_event(config, event);
        if(!validate_event(config, event)) {
            abort();
        }
        add_event(event, events);
        count++;
    }
    fclose(f);
    return count;
}

size_t read_betree_exprs(struct config* config, struct cnode* cnode)
{
    FILE* f = fopen("betree_exprs", "r");

    char line[LINE_MAX * 2];
    betree_sub_t sub_id = 1;
    while(fgets(line, sizeof(line), f)) {
        betree_insert(config, sub_id, line, cnode);
        sub_id++;
        if(MAX_EXPRS != 0 && sub_id - 1 == MAX_EXPRS) {
            break;
        }
    }

    fclose(f);
    return sub_id - 1;
}

void read_betree_defs(struct config* config)
{
    FILE* f = fopen("betree_defs", "r");

    char line[LINE_MAX];
    while(fgets(line, sizeof(line), f)) {
        char* line_rest = line;
        const char* name = strtok_r(line_rest, "|", &line_rest);
        const char* type = strtok_r(line_rest, "|", &line_rest);
        bool allow_undefined = strcmp(strtok_r(line_rest, "|\n", &line_rest), "true") == 0;
        const char* min_str = strtok_r(line_rest, "|\n", &line_rest);
        const char* max_str = strtok_r(line_rest, "|\n", &line_rest);
        if(strcmp(type, "integer") == 0) {
            int64_t min = INT64_MIN, max = INT64_MAX;
            if(min_str != NULL) {
                min = strtoll(min_str, NULL, 10);
            }
            if(max_str != NULL) {
                max = strtoll(max_str, NULL, 10);
            }
            add_attr_domain_i(config, name, min, max, allow_undefined);
        }
        else if(strcmp(type, "float") == 0) {
            double min = -DBL_MAX, max = DBL_MAX;
            if(min_str != NULL) {
                min = atof(min_str);
            }
            if(max_str != NULL) {
                max = atof(max_str);
            }
            add_attr_domain_i(config, name, min, max, allow_undefined);
        }
        else if(strcmp(type, "boolean") == 0) {
            add_attr_domain_b(config, name, false, true, allow_undefined);
        }
        else if(strcmp(type, "frequency") == 0) {
            add_attr_domain_frequency(config, name, allow_undefined);
        }
        else if(strcmp(type, "segments") == 0) {
            add_attr_domain_segments(config, name, allow_undefined);
        }
        else if(strcmp(type, "string") == 0) {
            if(min_str != NULL) {
                size_t max = atoi(min_str);
                add_attr_domain_bounded_s(config, name, allow_undefined, max);
            }
            else {
                add_attr_domain_s(config, name, allow_undefined);
            }
        }
        else if(strcmp(type, "integer list") == 0) {
            add_attr_domain_il(config, name, allow_undefined);
        }
        else if(strcmp(type, "string list") == 0) {
            add_attr_domain_sl(config, name, allow_undefined);
        }
        else {
            fprintf(stderr, "Unknown definition type");
            abort();
        }
    }

    fclose(f);
}

int test_real()
{
    if(access("betree_defs", F_OK) == -1 || access("betree_events", F_OK) == -1
        || access("betree_exprs", F_OK) == -1) {
        fprintf(stderr, "Missing files, skipping the tests");
        return 0;
    }
    struct timespec start, insert_done, gen_event_done, search_done;

    // Init
    struct config* config = make_default_config();
    read_betree_defs(config);

    clock_gettime(CLOCK_MONOTONIC_RAW, &start);

    // Insert
    struct cnode* cnode = make_cnode(config, NULL);
    size_t expr_count = read_betree_exprs(config, cnode);

    clock_gettime(CLOCK_MONOTONIC_RAW, &insert_done);
    uint64_t insert_us = (insert_done.tv_sec - start.tv_sec) * 1000000
        + (insert_done.tv_nsec - start.tv_nsec) / 1000;
    printf("    Insert took %" PRIu64 "\n", insert_us);

    struct events events = { .count = 0, .events = NULL };
    size_t event_count = read_betree_events(config, &events);

    // <DEBUG>
    /*for(size_t i = 0; i < config->attr_domain_count; i++) {*/
        /*const struct attr_domain* attr_domain = config->attr_domains[i];*/
        /*if(attr_domain->bound.value_type == VALUE_I) {*/
            /*struct value_bound bound = get_integer_events_bound(attr_domain->attr_var.var, events.events, event_count);*/
            /*printf("    %s: [%ld, %ld]\n", attr_domain->attr_var.attr, bound.imin, bound.imax);*/
        /*}*/
        /*else if(attr_domain->bound.value_type == VALUE_F) {*/
            /*struct value_bound bound = get_float_events_bound(attr_domain->attr_var.var, events.events, event_count);*/
            /*printf("    %s: [%.2f, %.2f]\n", attr_domain->attr_var.attr, bound.fmin, bound.fmax);*/
        /*}*/
    /*}*/
    // </DEBUG>

    uint64_t search_timings[MAX_EVENTS] = { 0 };
    uint64_t search_us_sum = 0;
    uint64_t evaluated_sum = 0;
    uint64_t matched_sum = 0;
    uint64_t memoized_sum = 0;

    /*MATCH_NODE_DEBUG = true;*/

    for(size_t i = 0; i < events.count; i++) {
        clock_gettime(CLOCK_MONOTONIC_RAW, &gen_event_done);

        struct event* event = events.events[i];
        struct matched_subs* matched_subs = make_matched_subs();
        struct report report = { .expressions_evaluated = 0, .expressions_matched = 0 };
        betree_search_with_event(config, event, cnode, matched_subs, &report);

        clock_gettime(CLOCK_MONOTONIC_RAW, &search_done);
        uint64_t search_us = (search_done.tv_sec - gen_event_done.tv_sec) * 1000000
            + (search_done.tv_nsec - gen_event_done.tv_nsec) / 1000;
        /*
        printf("    %05zu: Search took %" PRIu64 ", evaluated %zu expressions, matched %zu\n",
            i,
            search_us,
            report.expressions_evaluated,
            report.expressions_matched);
        */
        search_timings[i] = search_us;
        search_us_sum += search_us;
        evaluated_sum += report.expressions_evaluated;
        matched_sum += report.expressions_matched;
        memoized_sum += report.sub_expressions_memoized;
        free_matched_subs(matched_subs);
    }

    (void)search_timings;

    double search_us_average = (double)search_us_sum / (double)MAX_EVENTS;
    double evaluated_average = (double)evaluated_sum / (double)MAX_EVENTS;
    double matched_average = (double)matched_sum / (double)MAX_EVENTS;
    double memoized_average = (double)memoized_sum / (double)MAX_EVENTS;
    printf("%zu expressions, %zu events, %zu preds. Average: time %.2fus, evaluated %.2f, matched %.2f, memoized %.2f\n",
        expr_count,
        event_count,
        config->pred_map->pred_count,
        search_us_average,
        evaluated_average,
        matched_average,
        memoized_average);
    // DEBUG
    write_dot_file(config, cnode);
    // DEBUG


    free(events.events);
    free_cnode(cnode);
    free_config(config);
    return 0;
}

int all_tests()
{
    mu_run_test(test_real);

    return 0;
}

RUN_TESTS()
