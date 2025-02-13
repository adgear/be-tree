#include "reasons.h"
#include "alloc.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>

struct reasonlist* reasonlist_create(size_t size)
{
    struct reasonlist* new_list = bmalloc(sizeof(struct reasonlist));
    new_list->size = size;
    new_list->body = bmalloc(sizeof(struct arraylist*) * size);
    for(size_t i = 0; i < size; i++) {
        new_list->body[i] = arraylist_create();
    }
    assert(new_list->body);
    return new_list;
}

unsigned int reasonlist_size(struct reasonlist* l)
{
    return l->size;
}

void reasonlist_additem(struct reasonlist* l, betree_var_t reason, betree_sub_t value)
{
    assert(reason >= 0);
    assert(reason < l->size);
    arraylist_add(l->body[reason], value);
}

void reasonlist_join(struct reasonlist* l, betree_var_t reason, struct arraylist* sub_ids)
{
    assert(reason >= 0);
    assert(reason < l->size);
    if(sub_ids->size > 0) arraylist_join(l->body[reason], sub_ids);
}

/**
 * Return item located at index.
 */
struct arraylist* reasonlist_get(struct reasonlist* l, betree_var_t reason)
{
    assert(reason >= 0);
    assert(reason < l->size);
    return l->body[reason];
}

void reasonlist_destroy(struct reasonlist* l)
{
    assert(l);
    assert(l->body);
    for(size_t i = 0; i < l->size; i++) {
        assert(l->body[i]);
        arraylist_destroy(l->body[i]);
    }
    bfree(l->body);
    bfree(l);
}
