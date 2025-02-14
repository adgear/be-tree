/* This file was automatically generated.  Do not edit! */
#pragma once

#include <stdbool.h>
#include <stdint.h>

#define USE_UINT64_KEY

typedef struct hashtable hashtable;
void hashtable_destroy(hashtable* t);
typedef struct hashtable_entry hashtable_entry;
hashtable_entry* hashtable_body_allocate(unsigned int capacity);
hashtable* hashtable_create();
void hashtable_resize(hashtable* t, unsigned int capacity);

#if defined(USE_UINT64_KEY)
void hashtable_remove(hashtable* t, uint64_t key);
void hashtable_set(hashtable* t, uint64_t key, void* value);
void* hashtable_get(hashtable* t, uint64_t key);
unsigned int hashtable_find_slot(hashtable* t, uint64_t key);
uint64_t hashtable_hash(uint64_t ul);
#else
void hashtable_remove(hashtable* t, char* key);
void hashtable_set(hashtable* t, char* key, void* value);
void* hashtable_get(hashtable* t, char* key);
unsigned int hashtable_find_slot(hashtable* t, char* key);
uint64_t hashtable_hash(char* str);
#endif

struct hashtable {
    unsigned int size;
    unsigned int capacity;
    hashtable_entry* body;
};
struct hashtable_entry {
#if defined(USE_UINT64_KEY)
    uint64_t key;
    bool is_set;
#else
    char* key;
#endif
    void* value;
};
#define INTERFACE 0
