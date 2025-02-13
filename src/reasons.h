#ifndef __BETREE_REASONS_H
#define __BETREE_REASONS_H

#include "arraylist.h"
#include "var.h"

struct reasonlist {
    unsigned int size; // Count of items currently in list
    struct arraylist**
        body; // Pointer to allocated memory for items (of size capacity * sizeof(void*))
};

enum addtional_reason_t {
    REASON_GEO = 1,
    REASON_INVALID_EVENT = 2,
    REASON_UNKNOWN = 3,
    REASON_ADDITIONAL_MAX = REASON_UNKNOWN
};

#define ADDITIONAL_REASON(domain_count, reason) ((domain_count - 1) + reason)

struct reasonlist* reasonlist_create(size_t size);
unsigned int reasonlist_size(struct reasonlist* l);
struct arraylist* reasonlist_get(struct reasonlist* l, betree_var_t reason);
void reasonlist_additem(struct reasonlist* l, betree_var_t reason, betree_sub_t value);
void reasonlist_join(struct reasonlist* l, betree_var_t reason, struct arraylist* sub_ids);
void reasonlist_destroy(struct reasonlist* l);

#endif
