#include "dyn_arr.h"
#include "minunit.h"

#define TEST_INITIAL_CAPACITY     8
#define TEST_INITIAL_CAPACITY_NEW 16

int test_create_dynamic_array(void) {
    dynamic_array_t* arr = create_dynamic_array(TEST_INITIAL_CAPACITY);

    mu_assert(arr != NULL, "wellCreated");
    mu_assert(arr->size == 0, "goodInitialSize");
    mu_assert(arr->capacity == TEST_INITIAL_CAPACITY, "goodCapacity");

    destroy_dynamic_array(arr);

    return 0; 
}

int test_clear_dynamic_array(void) { 
    dynamic_array_t* arr = create_dynamic_array(TEST_INITIAL_CAPACITY);

    clear_dynamic_array(arr);

    mu_assert(arr->size == 0, "wellCleared");

    destroy_dynamic_array(arr);

    return 0; 
}

int test_resize_dynamic_array(void) { 
    dynamic_array_t* arr = create_dynamic_array(TEST_INITIAL_CAPACITY);

    mu_assert(arr != NULL, "wellCreated");

    resize_dynamic_array(arr, TEST_INITIAL_CAPACITY_NEW);

    mu_assert(arr->capacity == TEST_INITIAL_CAPACITY_NEW, "goodCapacity");

    destroy_dynamic_array(arr);

    return 0; 
}

int test_dynamic_array_add(void) { 
    dynamic_array_t* arr = create_dynamic_array(TEST_INITIAL_CAPACITY);

    size_t i = 0;
    for(; i < TEST_INITIAL_CAPACITY; i++) {
        dynamic_array_add(arr, i + 1);
    }
    dynamic_array_add(arr, i + 1);

    mu_assert(arr->size == (i + 1), "goodSize");
    mu_assert(arr->capacity == TEST_INITIAL_CAPACITY_NEW, "goodCapacity");

    destroy_dynamic_array(arr);

    return 0; 
}

int test_dynamic_array_merge(void) { 
    dynamic_array_t* arr = create_dynamic_array(TEST_INITIAL_CAPACITY);
    dynamic_array_t* arr2 = create_dynamic_array(TEST_INITIAL_CAPACITY_NEW);

    for(size_t i = 0; i < TEST_INITIAL_CAPACITY; i++) {
        dynamic_array_add(arr, i + 1);
    }

    for(size_t i = 0; i < TEST_INITIAL_CAPACITY_NEW; i++) {
        dynamic_array_add(arr2, i + TEST_INITIAL_CAPACITY + 1);
    }

    dynamic_array_t* arr3 = dynamic_array_merge(arr, arr2);

    mu_assert(arr3->size == TEST_INITIAL_CAPACITY + TEST_INITIAL_CAPACITY_NEW, "goodSize");
    mu_assert(arr3->capacity == TEST_INITIAL_CAPACITY + TEST_INITIAL_CAPACITY_NEW, "goodCapacity");

    destroy_dynamic_array(arr);
    destroy_dynamic_array(arr2);
    destroy_dynamic_array(arr3);

    return 0; 
}

int all_tests()
{
    mu_run_test(test_create_dynamic_array);
    mu_run_test(test_clear_dynamic_array);
    mu_run_test(test_resize_dynamic_array);
    mu_run_test(test_dynamic_array_add);
    mu_run_test(test_dynamic_array_merge);

    return 0;
}

RUN_TESTS()

