#pragma once

#include "arraylist.h"
#include "value.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

typedef uint64_t betree_sub_t;

struct cnode_err;

struct betree_err {
    struct config* config;
    struct cnode_err* cnode;
    struct arraylist* sub_ids;
};

struct betree_reason_t {
    char* name;
    struct arraylist* list;
};

enum addtional_reason_t {
    REASON_GEO = 1,
    REASON_INVALID_EVENT = 2,
    REASON_UNKNOWN = 3,
    REASON_ADDITIONAL_MAX = REASON_UNKNOWN
};

struct betree_reason_map_t {
    size_t size;
    struct betree_reason_t** reasons;
};

#define ADDITIONAL_REASON(domain_count, reason) ((domain_count - 1) + reason)

struct report_err {
    size_t evaluated;
    size_t matched;
    size_t memoized;
    size_t shorted;
    betree_sub_t* subs;
    struct betree_reason_map_t* reason_sub_id_list;
};

/*
 * Initialization
 */
void betree_init_err(struct betree_err* betree);
struct betree_err* betree_make_err();
struct betree_err* betree_make_with_parameters_err(
    uint64_t lnode_max_cap, uint64_t min_partition_size);

bool betree_make_sub_ids(struct betree_err* tree);

void betree_add_boolean_variable_err(
    struct betree_err* betree, const char* name, bool allow_undefined);
void betree_add_integer_variable_err(
    struct betree_err* betree, const char* name, bool allow_undefined, int64_t min, int64_t max);
void betree_add_float_variable_err(
    struct betree_err* betree, const char* name, bool allow_undefined, double min, double max);
void betree_add_string_variable_err(
    struct betree_err* betree, const char* name, bool allow_undefined, size_t count);
void betree_add_integer_list_variable_err(
    struct betree_err* betree, const char* name, bool allow_undefined, int64_t min, int64_t max);
void betree_add_integer_enum_variable_err(
    struct betree_err* betree, const char* name, bool allow_undefined, size_t count);
void betree_add_string_list_variable_err(
    struct betree_err* betree, const char* name, bool allow_undefined, size_t count);
void betree_add_segments_variable_err(
    struct betree_err* betree, const char* name, bool allow_undefined);
void betree_add_frequency_caps_variable_err(
    struct betree_err* betree, const char* name, bool allow_undefined);
struct betree_variable_definition betree_get_variable_definition_err(
    struct betree_err* betree, size_t index);

bool betree_change_boundaries_err(struct betree_err* tree, const char* expr);

const struct betree_sub* betree_make_sub_err(struct betree_err* tree,
    betree_sub_t id,
    size_t constant_count,
    const struct betree_constant** constants,
    const char* expr);
bool betree_insert_sub_err(struct betree_err* tree, const struct betree_sub* sub);

/*
 * Runtime
 */

struct betree_event* betree_make_event_err(const struct betree_err* betree);

bool betree_insert_err(struct betree_err* tree, betree_sub_t id, const char* expr);
bool betree_insert_with_constants_err(struct betree_err* tree,
    betree_sub_t id,
    size_t constant_count,
    const struct betree_constant** constants,
    const char* expr);

bool betree_search_err(
    const struct betree_err* tree, const char* event_str, struct report_err* report);
bool betree_search_ids_err(const struct betree_err* tree,
    const char* event_str,
    struct report_err* report,
    const uint64_t* ids,
    size_t sz);
bool betree_search_with_event_err(
    const struct betree_err* betree, struct betree_event* event, struct report_err* report);
bool betree_search_with_event_ids_err(const struct betree_err* betree,
    struct betree_event* event,
    struct report_err* report,
    const uint64_t* ids,
    size_t sz);

// bool betree_delete(struct betree_err* betree, betree_sub_t id);

#if defined(USE_REASONLIST)
struct report_err* make_report_err(const struct betree_err* betree);
#else
struct report_err* make_report_err();
#endif
void free_report_err(struct report_err* report);

/*
 * Destruction
 */
void betree_deinit_err(struct betree_err* betree);
void betree_free_err(struct betree_err* betree);

struct betree_reason_t* betree_reason_create(const char* reason_name);
void betree_reason_destroy(struct betree_reason_t* reason);

struct betree_reason_map_t* betree_reason_map_create(const struct betree_err* betree);
unsigned int betree_reason_map_size(struct betree_reason_map_t* m);
struct arraylist* betree_reason_map_get(struct betree_reason_map_t* l, betree_var_t reason);
void betree_reason_map_additem(
    struct betree_reason_map_t* l, betree_var_t reason, betree_sub_t value);
void betree_reason_map_join(
    struct betree_reason_map_t* l, betree_var_t reason, struct arraylist* sub_ids);
void betree_reason_map_join_with_ids(struct betree_reason_map_t* m,
    betree_var_t reason,
    struct arraylist* sub_ids,
    const uint64_t* ids,
    size_t sz);
void betree_reason_map_destroy(struct betree_reason_map_t* reason);
