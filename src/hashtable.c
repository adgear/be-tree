/**
 * Hashtable implementation
 * (c) 2011-2019 @marekweb
 *
 * Uses dynamic addressing with linear probing.
 */

#include "hashtable.h"
#include "alloc.h"
#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

/*
 * Interface section used for `makeheaders`.
 */
#if INTERFACE
struct hashtable_entry {
#if defined(USE_UINT64_KEY)
    uint64_t key;
    bool is_key;
#else
    char* key;
#endif
    void* value;
};

struct hashtable {
    unsigned int size;
    unsigned int capacity;
    hashtable_entry* body;
};
#endif

#define HASHTABLE_INITIAL_CAPACITY 4096

#if defined(USE_UINT64_KEY)
uint64_t hashtable_hash(uint64_t ul)
{
    uint64_t p = 0x5555555555555555ull;
    uint64_t c = 17316035218449499591ull;
    uint64_t ul2 = p * (ul ^ (ul >> 32));
    return c * (ul2 ^ (ul2 >> 32));
}
#else
/**
 * Compute the hash value for the given string.
 * Implements the djb k=33 hash function.
 */
uint64_t hashtable_hash(char* str)
{
    uint64_t hash = 5381;
    int c;
    while((c = *str++)) hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
    return hash;
}
#endif

/**
 * Find an available slot for the given key, using linear probing.
 */
#if defined(USE_UINT64_KEY)
unsigned int hashtable_find_slot(hashtable* t, uint64_t key)
#else
unsigned int hashtable_find_slot(hashtable* t, char* key)
#endif
{
    int index = hashtable_hash(key) % t->capacity;
#if defined(USE_UINT64_KEY)
    while(t->body[index].is_set != false && t->body[index].key != key) {
#else
    while(t->body[index].key != NULL && strcmp(t->body[index].key, key) != 0) {
#endif
        index = (index + 1) % t->capacity;
    }
    return index;
}

/**
 * Return the item associated with the given key, or NULL if not found.
 */
#if defined(USE_UINT64_KEY)
void* hashtable_get(hashtable* t, uint64_t key)
#else
void* hashtable_get(hashtable* t, char* key)
#endif
{
    int index = hashtable_find_slot(t, key);
#if defined(USE_UINT64_KEY)
    if(t->body[index].is_set != false) {
#else
    if(t->body[index].key != NULL) {
#endif
        return t->body[index].value;
    }
    else {
        return NULL;
    }
}

/**
 * Assign a value to the given key in the table.
 */
#if defined(USE_UINT64_KEY)
void hashtable_set(hashtable* t, uint64_t key, void* value)
#else
void hashtable_set(hashtable* t, char* key, void* value)
#endif
{
    int index = hashtable_find_slot(t, key);
#if defined(USE_UINT64_KEY)
    if(t->body[index].is_set != false) {
#else
    if(t->body[index].key != NULL) {
#endif
        /* Entry exists; update it. */
        t->body[index].value = value;
    }
    else {
        t->size++;
        /* Create a new  entry */
        if((float)t->size / t->capacity > 0.8) {
            /* Resize the hash table */
            hashtable_resize(t, t->capacity * 2);
            index = hashtable_find_slot(t, key);
        }
        t->body[index].key = key;
        t->body[index].value = value;
#if defined(USE_UINT64_KEY)
        t->body[index].is_set = true;
#endif
    }
}

/**
 * Remove a key from the table
 */
#if defined(USE_UINT64_KEY)
void hashtable_remove(hashtable* t, uint64_t key)
#else
void hashtable_remove(hashtable* t, char* key)
#endif
{
    int index = hashtable_find_slot(t, key);
#if defined(USE_UINT64_KEY)
    if(t->body[index].is_set != false) {
#else
    if(t->body[index].key != NULL) {
#endif
        t->body[index].key = NULL;
        t->body[index].value = NULL;
#if defined(USE_UINT64_KEY)
        t->body[index].is_set = false;
#endif
        t->size--;
    }
}

/**
 * Create a new, empty hashtable
 */
hashtable* hashtable_create()
{
    hashtable* new_ht = bmalloc(sizeof(hashtable));
    new_ht->size = 0;
    new_ht->capacity = HASHTABLE_INITIAL_CAPACITY;
    new_ht->body = hashtable_body_allocate(new_ht->capacity);
    return new_ht;
}

/**
 * Allocate a new memory block with the given capacity.
 */
hashtable_entry* hashtable_body_allocate(unsigned int capacity)
{
    // bcalloc fills the allocated memory with zeroes
    return (hashtable_entry*)bcalloc(capacity * sizeof(hashtable_entry));
}

/**
 * Resize the allocated memory.
 */
void hashtable_resize(hashtable* t, unsigned int capacity)
{
    assert(capacity >= t->size);
    unsigned int old_capacity = t->capacity;
    hashtable_entry* old_body = t->body;
    t->body = hashtable_body_allocate(capacity);
    t->capacity = capacity;

    // Copy all the old values into the newly allocated body
    for(size_t i = 0; i < old_capacity; i++) {
#if defined(USE_UINT64_KEY)
        if(old_body[i].is_set != false) {
#else
        if(old_body[i].key != NULL) {
#endif
            hashtable_set(t, old_body[i].key, old_body[i].value);
        }
    }

    bfree(old_body);
}

/**
 * Destroy the table and deallocate it from memory. This does not deallocate the contained items.
 */
void hashtable_destroy(hashtable* t)
{
    bfree(t->body);
    bfree(t);
}
