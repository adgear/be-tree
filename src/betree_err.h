#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include "arraylist.h"
#include "hashtable.h"

typedef uint64_t betree_sub_t;

struct cnode_err;

struct betree_err {
    struct config* config;
    struct cnode_err* cnode;
    struct arraylist* sub_ids;
};

struct report_err {
    size_t evaluated;
    size_t matched;
    size_t memoized;
    size_t shorted;
    betree_sub_t* subs;
    hashtable* reason_sub_id_list;
};

/*
 * Initialization
 */
void betree_init_err(struct betree_err* betree);
struct betree_err* betree_make_err();
struct betree_err* betree_make_with_parameters_err(uint64_t lnode_max_cap, uint64_t min_partition_size);

void betree_add_boolean_variable_err(struct betree_err* betree, const char* name, bool allow_undefined);
void betree_add_integer_variable_err(struct betree_err* betree, const char* name, bool allow_undefined, int64_t min, int64_t max);
void betree_add_float_variable_err(struct betree_err* betree, const char* name, bool allow_undefined, double min, double max);
void betree_add_string_variable_err(struct betree_err* betree, const char* name, bool allow_undefined, size_t count);
void betree_add_integer_list_variable_err(struct betree_err* betree, const char* name, bool allow_undefined, int64_t min, int64_t max);
void betree_add_integer_enum_variable_err(struct betree_err* betree, const char* name, bool allow_undefined, size_t count);
void betree_add_string_list_variable_err(struct betree_err* betree, const char* name, bool allow_undefined, size_t count);
void betree_add_segments_variable_err(struct betree_err* betree, const char* name, bool allow_undefined);
void betree_add_frequency_caps_variable_err(struct betree_err* betree, const char* name, bool allow_undefined);

bool betree_change_boundaries_err(struct betree_err* tree, const char* expr);

const struct betree_sub* betree_make_sub_err(
    struct betree_err* tree, betree_sub_t id, size_t constant_count, const struct betree_constant** constants, const char* expr);
bool betree_insert_sub_err(struct betree_err* tree, const struct betree_sub* sub);

/*
 * Runtime
 */

struct betree_event* betree_make_event_err(const struct betree_err* betree);

bool betree_insert_err(struct betree_err* tree, betree_sub_t id, const char* expr);
bool betree_insert_with_constants_err(struct betree_err* tree, betree_sub_t id, size_t constant_count, const struct betree_constant** constants, const char* expr);

bool betree_search_err(const struct betree_err* tree, const char* event_str, struct report_err* report);
bool betree_search_ids_err(const struct betree_err* tree, const char* event_str, struct report_err* report, const uint64_t* ids, size_t sz);
bool betree_search_with_event_err(const struct betree_err* betree, struct betree_event* event, struct report_err* report);
bool betree_search_with_event_ids_err(const struct betree_err* betree, struct betree_event* event, struct report_err* report, const uint64_t* ids, size_t sz);

//bool betree_delete(struct betree_err* betree, betree_sub_t id);

struct report_err* make_report_err();
void free_report_err(struct report_err* report);

/*
 * Destruction
 */
void betree_deinit_err(struct betree_err* betree);
void betree_free_err(struct betree_err* betree);