#ifndef __DYNAMIC_ARRAY_H__
#define __DYNAMIC_ARRAY_H__

#include <stdio.h>
#include <stdint.h>

typedef struct {
    size_t size;
    size_t capacity;
    uint64_t *data;
} dynamic_array_t;

#define INITIAL_CAPACITY 16

dynamic_array_t* create_dynamic_array(size_t initial_capacity);
void clear_dynamic_array(dynamic_array_t* arr);
void destroy_dynamic_array(dynamic_array_t* arr);
void resize_dynamic_array(dynamic_array_t* arr, size_t new_capacity);
void dynamic_array_add(dynamic_array_t* arr, uint64_t element);
dynamic_array_t* dynamic_array_merge(dynamic_array_t* arr1, dynamic_array_t* arr2);

#endif
