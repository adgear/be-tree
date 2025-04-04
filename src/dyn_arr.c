#include "dyn_arr.h"
#include "alloc.h"

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

dynamic_array_t* create_dynamic_array(size_t initial_capacity) {
    dynamic_array_t *arr = (dynamic_array_t *)bmalloc(sizeof(dynamic_array_t));
    if(!arr) {
        perror("Failed to allocate memory for dynamic array");
        return NULL;
    }

    arr->data = (uint64_t *)bmalloc(initial_capacity * sizeof(uint64_t));
    if(!(arr->data)) {
        perror("Failed to allocate memory for array data");
        bfree(arr);
        return NULL;
    }

    arr->size = 0;
    arr->capacity = initial_capacity;

    return arr;
}

void clear_dynamic_array(dynamic_array_t* arr) {
    arr->size = 0;
}

void destroy_dynamic_array(dynamic_array_t* arr) {
    if(arr) {
        if(arr->data) bfree(arr->data);
        bfree(arr);
    }
}

void resize_dynamic_array(dynamic_array_t* arr, size_t new_capacity) {
    uint64_t* new_data = (uint64_t *)brealloc(arr->data, new_capacity * sizeof(uint64_t));
    if(!new_data) {
        perror("Failed to reallocate memory for array data");
        return;
    }

    arr->data = new_data;
    arr->capacity = new_capacity;
}

void dynamic_array_add(dynamic_array_t* arr, uint64_t element) {
    if(arr->size >= arr->capacity) {
        size_t new_capacity = ((arr->capacity == 0) ? 1 : (arr->capacity << 1));
        resize_dynamic_array(arr, new_capacity);
    }

    arr->data[arr->size++] = element;
}

dynamic_array_t* dynamic_array_merge(dynamic_array_t* arr1, dynamic_array_t* arr2) {
    dynamic_array_t* merged_arr = create_dynamic_array(arr1->size + arr2->size);

    memmove(merged_arr->data, arr1->data, arr1->size * sizeof(uint64_t));
    memmove(merged_arr->data + arr1->size, arr2->data, arr2->size * sizeof(uint64_t));

    merged_arr->size = arr1->size + arr2->size;

    return merged_arr;
}
