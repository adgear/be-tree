#include <float.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "alloc.h"
#include "dyn_arr.h"
#include "betree_err.h"
#include "debug.h"
#include "debug_err.h"
#include "helper.h"
#include "minunit.h"
#include "printer.h"
#include "special.h"
#include "tree_err.h"
#include "utils.h"

enum ATTR_DOMAIN_POSITION {
    ATTR_BOOL = 0,
    ATTR_INT,
    ATTR_FLOAT,
    ATTR_STR,
    ATTR_INT_LIST,
    ATTR_STR_LIST,
    ATTR_SEGMENTS,
    ATTR_FCAP,
    ATTR_GEO,
    ATTR_INT64,
};

#define ATTR_CONFIG_1(r) ((1 << r))
#define ATTR_CONFIG_2(r, s) ((1 << r) | (1 << s))
#define ATTR_CONFIG_3(r, s, t) ((1 << r) | (1 << s) | (1 << t))
#define ATTR_CONFIG_4(r, s, t, u) ((1 << r) | (1 << s) | (1 << t) | (1 << u))
#define ATTR_CONFIG_5(r, s, t, u, v) ((1 << r) | (1 << s) | (1 << t) | (1 << u) | (1 << v))
#define ATTR_CONFIG_6(r, s, t, u, v, w) \
    ((1 << r) | (1 << s) | (1 << t) | (1 << u) | (1 << v) | (1 << w))
#define ATTR_CONFIG_7(r, s, t, u, v, w, x) \
    ((1 << r) | (1 << s) | (1 << t) | (1 << u) | (1 << v) | (1 << w) | (1 << x))
#define ATTR_CONFIG_8(r, s, t, u, v, w, x, y) \
    ((1 << r) | (1 << s) | (1 << t) | (1 << u) | (1 << v) | (1 << w) | (1 << x) | (1 << y))
#define ATTR_CONFIG_9(r, s, t, u, v, w, x, y, z)                                           \
    ((1 << r) | (1 << s) | (1 << t) | (1 << u) | (1 << v) | (1 << w) | (1 << x) | (1 << y) \
        | (1 << z))
#define ATTR_CONFIG_10(r, s, t, u, v, w, x, y, z, q)                                       \
    ((1 << r) | (1 << s) | (1 << t) | (1 << u) | (1 << v) | (1 << w) | (1 << x) | (1 << y) \
        | (1 << z) | (1 << q))


void betree_bulk_insert(struct betree_err* tree, const char** exprs, const size_t count);
void betree_bulk_insert_with_constants(struct betree_err* tree,
    const char** exprs,
    size_t exprs_count,
    const struct betree_constant** constants,
    size_t constants_count);

void make_attr_domains(struct betree_err* tree, size_t config);
void make_attr_domains_undefined(struct betree_err* tree, size_t config, size_t config_undefined);

int test_bool_fail()
{
    struct betree_err* tree = betree_make_err();

    make_attr_domains(tree, ATTR_CONFIG_2(ATTR_BOOL, ATTR_INT));

    const char* exprs[] = { "b and i = 1" };
    const size_t exprs_count = 1;
    betree_bulk_insert(tree, exprs, exprs_count);

    betree_make_sub_ids(tree);
    const char* event = "{\"b\": false, \"i\": 1}";
    struct report_err* report = make_report_err(tree);
#if defined(DEBUG)
    fprintf(stderr, "search ... %s\n", event);
#endif
    betree_search_err(tree, event, report);

    mu_assert(report->matched == 0, "goodEvent");
    dynamic_array_t* rlist = betree_reason_map_get(report->reason_sub_id_list, 0);
    mu_assert(rlist != NULL, "goodReport");
    mu_assert((betree_var_t)rlist->data[0] == (betree_var_t)1, "goodReason");
    mu_assert(strcmp(report->reason_sub_id_list->reasons[0]->name, "b") == 0, "goodReason");

    free_report_err(report);
    betree_free_err(tree);
#if defined(DEBUG)
    fprintf(stderr, "\n");
#endif
    return 0;
}


int test_int_fail()
{
    struct betree_err* tree = betree_make_err();

    make_attr_domains(tree, ATTR_CONFIG_2(ATTR_BOOL, ATTR_INT));

    const char* exprs[] = { "b and i = 1" };
    const size_t exprs_count = 1;
    betree_bulk_insert(tree, exprs, exprs_count);

    betree_make_sub_ids(tree);
    const char* event = "{\"b\": true, \"i\": 2}";
    struct report_err* report = make_report_err(tree);
#if defined(DEBUG)
    fprintf(stderr, "search ... %s\n", event);
#endif
    betree_search_err(tree, event, report);

    mu_assert(report->matched == 0, "goodEvent");
    dynamic_array_t* rlist = betree_reason_map_get(report->reason_sub_id_list, 1);
    mu_assert(rlist != NULL, "goodReport");
    mu_assert((betree_var_t)rlist->data[0] == (betree_var_t)1, "goodReason");
    mu_assert(strcmp(report->reason_sub_id_list->reasons[1]->name, "i") == 0, "goodReason");

    free_report_err(report);
    betree_free_err(tree);
#if defined(DEBUG)
    fprintf(stderr, "\n");
#endif
    return 0;
}


int test_float_fail()
{
    struct betree_err* tree = betree_make_err();

    make_attr_domains(tree, ATTR_CONFIG_2(ATTR_BOOL, ATTR_FLOAT));

    const char* exprs[] = { "b and f = 0.1" };
    const size_t exprs_count = 1;
    betree_bulk_insert(tree, exprs, exprs_count);

    betree_make_sub_ids(tree);
    const char* event = "{\"b\": true, \"f\": 0.2}";
    struct report_err* report = make_report_err(tree);
#if defined(DEBUG)
    fprintf(stderr, "search ... %s\n", event);
#endif
    betree_search_err(tree, event, report);

    mu_assert(report->matched == 0, "goodEvent");
    dynamic_array_t* rlist = betree_reason_map_get(report->reason_sub_id_list, 1);
    mu_assert(rlist != NULL, "goodReport");
    mu_assert((betree_var_t)rlist->data[0] == (betree_var_t)1, "goodReason");
    mu_assert(strcmp(report->reason_sub_id_list->reasons[1]->name, "f") == 0, "goodReason");

    free_report_err(report);
    betree_free_err(tree);
#if defined(DEBUG)
    fprintf(stderr, "\n");
#endif
    return 0;
}

int test_bin_fail()
{
    struct betree_err* tree = betree_make_err();

    make_attr_domains(tree, ATTR_CONFIG_2(ATTR_BOOL, ATTR_STR));

    const char* exprs[] = { "b and s = \"betrees\"" };
    const size_t exprs_count = 1;
    betree_bulk_insert(tree, exprs, exprs_count);

    betree_make_sub_ids(tree);
    const char* event = "{\"b\": true, \"s\": \"betree\"}";
    struct report_err* report = make_report_err(tree);
#if defined(DEBUG)
    fprintf(stderr, "search ... %s\n", event);
#endif

    betree_search_err(tree, event, report);
    mu_assert(report->matched == 0, "goodEvent");
    dynamic_array_t* rlist = betree_reason_map_get(report->reason_sub_id_list, 1);
    mu_assert(rlist != NULL, "goodReport");
    mu_assert((betree_var_t)rlist->data[0] == (betree_var_t)1, "goodReason");
    mu_assert(strcmp(report->reason_sub_id_list->reasons[1]->name, "s") == 0, "goodReason");

    free_report_err(report);
    betree_free_err(tree);
#if defined(DEBUG)
    fprintf(stderr, "\n");
#endif
    return 0;
}


int test_int_list_fail()
{
    struct betree_err* tree = betree_make_err();

    make_attr_domains(tree, ATTR_CONFIG_2(ATTR_BOOL, ATTR_INT_LIST));

    const char* exprs[] = { "b and il one of (1,2)" };
    const size_t exprs_count = 1;
    betree_bulk_insert(tree, exprs, exprs_count);

    betree_make_sub_ids(tree);
    const char* event = "{\"b\": true, \"il\": [3]}";
    struct report_err* report = make_report_err(tree);
#if defined(DEBUG)
    fprintf(stderr, "search ... %s\n", event);
#endif
    betree_search_err(tree, event, report);

    mu_assert(report->matched == 0, "goodEvent");
    dynamic_array_t* rlist = betree_reason_map_get(report->reason_sub_id_list, 1);
    mu_assert(rlist != NULL, "goodReport");
    mu_assert((betree_var_t)rlist->data[0] == (betree_var_t)1, "goodReason");
    mu_assert(strcmp(report->reason_sub_id_list->reasons[1]->name, "il") == 0, "goodReason");

    free_report_err(report);
    betree_free_err(tree);
#if defined(DEBUG)
    fprintf(stderr, "\n");
#endif
    return 0;
}


int test_bin_list_fail()
{
    struct betree_err* tree = betree_make_err();

    make_attr_domains(tree, ATTR_CONFIG_2(ATTR_BOOL, ATTR_STR_LIST));

    const char* exprs[] = { "b and sl one of (\"How\",\"is\", \"it\", \"going\")" };
    const size_t exprs_count = 1;
    betree_bulk_insert(tree, exprs, exprs_count);

    betree_make_sub_ids(tree);
    const char* event = "{\"b\": true, \"sl\": [\"how\"]}";
    struct report_err* report = make_report_err(tree);
#if defined(DEBUG)
    fprintf(stderr, "search ... %s\n", event);
#endif
    betree_search_err(tree, event, report);

    mu_assert(report->matched == 0, "goodEvent");
    dynamic_array_t* rlist = betree_reason_map_get(report->reason_sub_id_list, 1);
    mu_assert(rlist != NULL, "goodReport");
    mu_assert((betree_var_t)rlist->data[0] == (betree_var_t)1, "goodReason");
    mu_assert(strcmp(report->reason_sub_id_list->reasons[1]->name, "sl") == 0, "goodReason");

    free_report_err(report);
    betree_free_err(tree);
#if defined(DEBUG)
    fprintf(stderr, "\n");
#endif
    return 0;
}


int test_segments_fail()
{
    struct betree_err* tree = betree_make_err();

    make_attr_domains(tree, ATTR_CONFIG_2(ATTR_INT64, ATTR_SEGMENTS));

    const char* exprs[] = { "segment_within(seg, 1, 10)" };
    const size_t exprs_count = 1;
    betree_bulk_insert(tree, exprs, exprs_count);

    betree_make_sub_ids(tree);
    const char* event = "{\"now\": 30, \"seg\": [[1, 10000000]]}";
    struct report_err* report = make_report_err(tree);
#if defined(DEBUG)
    fprintf(stderr, "search ... %s\n", event);
#endif
    betree_search_err(tree, event, report);

    mu_assert(report->matched == 0, "goodEvent");
    dynamic_array_t* rlist = betree_reason_map_get(report->reason_sub_id_list, 0);
    mu_assert(rlist != NULL, "goodReport");
    mu_assert((betree_var_t)rlist->data[0] == (betree_var_t)1, "goodReason");
    mu_assert(strcmp(report->reason_sub_id_list->reasons[0]->name, "seg") == 0, "goodReason");

    free_report_err(report);
    betree_free_err(tree);
#if defined(DEBUG)
    fprintf(stderr, "\n");
#endif
    return 0;
}


int test_frequency_cap_fail()
{
    struct betree_err* tree = betree_make_err();

    const size_t constant_count = 1;
    const struct betree_constant* constants[]
        = { betree_make_integer_constant("advertiser_id", 20) };

    make_attr_domains(tree, ATTR_CONFIG_2(ATTR_INT64, ATTR_FCAP));

    const char* exprs[] = { "not within_frequency_cap(\"advertiser\", \"namespace\", 100, 100)" };
    const size_t exprs_count = 1;
    betree_bulk_insert_with_constants(tree, exprs, exprs_count, constants, constant_count);

    betree_make_sub_ids(tree);
    const char* event
        = "{\"now\": 30, \"frequency_caps\": [[\"campaign\", 30, \"namespace\", 20, 10]]}";
    struct report_err* report = make_report_err(tree);
#if defined(DEBUG)
    fprintf(stderr, "search ... %s\n", event);
#endif
    betree_search_err(tree, event, report);

    mu_assert(report->matched == 0, "goodEvent");
    dynamic_array_t* rlist = betree_reason_map_get(report->reason_sub_id_list, 0);
    mu_assert(rlist != NULL, "goodReport");
    mu_assert((betree_var_t)rlist->data[0] == (betree_var_t)1, "goodReason");
    mu_assert(
        strcmp(report->reason_sub_id_list->reasons[0]->name, "frequency_caps") == 0, "goodReason");

    free_report_err(report);
    betree_free_err(tree);
#if defined(DEBUG)
    fprintf(stderr, "\n");
#endif
    return 0;
}


int test_geo_fail()
{
    struct betree_err* tree = betree_make_err();

    make_attr_domains(tree, ATTR_CONFIG_2(ATTR_BOOL, ATTR_GEO));

    const char* exprs[] = { "b and geo_within_radius(10, 100, 100)" };
    const size_t exprs_count = 1;
    betree_bulk_insert(tree, exprs, exprs_count);

    betree_make_sub_ids(tree);
    const char* event = "{\"b\": true, \"latitude\": 101.0, \"longitude\": 99.0}";
    struct report_err* report = make_report_err(tree);
#if defined(DEBUG)
    fprintf(stderr, "search ... %s\n", event);
#endif
    betree_search_err(tree, event, report);

    mu_assert(report->matched == 0, "goodEvent");
    dynamic_array_t* rlist = betree_reason_map_get(report->reason_sub_id_list, 3);
    mu_assert(rlist != NULL, "goodReport");
    mu_assert((betree_var_t)rlist->data[0] == (betree_var_t)1, "goodReason");

    free_report_err(report);
    betree_free_err(tree);
#if defined(DEBUG)
    fprintf(stderr, "\n");
#endif
    return 0;
}


int test_int64_fail()
{
    struct betree_err* tree = betree_make_err();

    make_attr_domains(tree, ATTR_CONFIG_2(ATTR_BOOL, ATTR_INT64));

    const char* exprs[] = { "b and now = 1" };
    const size_t exprs_count = 1;
    betree_bulk_insert(tree, exprs, exprs_count);

    betree_make_sub_ids(tree);
    const char* event = "{\"b\": true, \"now\": 2}";
    struct report_err* report = make_report_err(tree);
#if defined(DEBUG)
    fprintf(stderr, "search ... %s\n", event);
#endif
    betree_search_err(tree, event, report);

    mu_assert(report->matched == 0, "goodEvent");
    dynamic_array_t* rlist = betree_reason_map_get(report->reason_sub_id_list, 1);
    mu_assert(rlist != NULL, "goodReport");
    mu_assert((betree_var_t)rlist->data[0] == (betree_var_t)1, "goodReason");

    free_report_err(report);
    betree_free_err(tree);
#if defined(DEBUG)
    fprintf(stderr, "\n");
#endif
    return 0;
}


int test_short_circuit_fail()
{
    struct betree_err* tree = betree_make_err();

    make_attr_domains_undefined(tree,
        ATTR_CONFIG_5(ATTR_BOOL, ATTR_INT, ATTR_FLOAT, ATTR_STR, ATTR_INT_LIST),
        ATTR_CONFIG_3(ATTR_FLOAT, ATTR_STR, ATTR_INT_LIST));

    const char* exprs[] = { "b and i = 1 and f = 0.1 and s = \"s1\"",
        "b and i = 2 and s = \"s2\"",
        "b and i = 3 and (il one of (1, 2, 3))" };
    const size_t exprs_count = 3;
    betree_bulk_insert(tree, exprs, exprs_count);

    betree_make_sub_ids(tree);
    const char* event = "{\"b\": true, \"i\": 0}";
    struct report_err* report = make_report_err(tree);
#if defined(DEBUG)
    fprintf(stderr, "search ... %s\n", event);
#endif
    betree_search_err(tree, event, report);

    mu_assert(report->matched == 0, "goodEvent");
    dynamic_array_t* rlist = betree_reason_map_get(report->reason_sub_id_list, 2);
    mu_assert(rlist != NULL, "goodReport");
    dynamic_array_t* rlist2 = betree_reason_map_get(report->reason_sub_id_list, 3);
    mu_assert(rlist2 != NULL, "goodReport");
    dynamic_array_t* rlist3 = betree_reason_map_get(report->reason_sub_id_list, 4);
    mu_assert(rlist3 != NULL, "goodReport");
    mu_assert((betree_var_t)rlist->data[0] == (betree_var_t)1, "goodReason");
    mu_assert((betree_var_t)rlist2->data[0] == (betree_var_t)2, "goodReason");
    mu_assert((betree_var_t)rlist3->data[0] == (betree_var_t)3, "goodReason");

    free_report_err(report);
    betree_free_err(tree);
#if defined(DEBUG)
    fprintf(stderr, "\n");
#endif
    return 0;
}


int test_multiple_bool_exprs_fail()
{
    struct betree_err* tree = betree_make_err();

    make_attr_domains(tree, ATTR_CONFIG_4(ATTR_BOOL, ATTR_INT, ATTR_FLOAT, ATTR_STR));

    const char* exprs[] = {
        "not (((not b) and i = 2 and f = 0.3) or (s <> \"s0\"))",
        "not ((b and i = 1 and f = 0.0) or (s <> \"s1\"))",
        "(b or i = 0 or f = 0.1) or (s <> \"s3\")",
        "not ((b or i = 1 or f = 0.2) or (s = \"s2\"))",
        "not ((b or i = 2 or f = 0.1) or (s = \"s3\"))",
    };
    const size_t exprs_count = 5;
    betree_bulk_insert(tree, exprs, exprs_count);

    betree_make_sub_ids(tree);
    const char* event = "{\"b\": false, \"i\": 2, \"f\": 0.2, \"s\": \"s3\"}";
    struct report_err* report = make_report_err(tree);
#if defined(DEBUG)
    fprintf(stderr, "search ... %s\n", event);
#endif
    betree_search_err(tree, event, report);

    mu_assert(report->matched == 0, "goodEvent");
    dynamic_array_t* rlist = betree_reason_map_get(report->reason_sub_id_list, 1);
    mu_assert(rlist != NULL, "goodReport");
    dynamic_array_t* rlist2 = betree_reason_map_get(report->reason_sub_id_list, 2);
    mu_assert(rlist2 != NULL, "goodReport");
    dynamic_array_t* rlist3 = betree_reason_map_get(report->reason_sub_id_list, 3);
    mu_assert(rlist3 != NULL, "goodReport");
    mu_assert((betree_var_t)rlist->data[0] == (betree_var_t)3, "goodReason");
    mu_assert((betree_var_t)rlist2->data[0] == (betree_var_t)4, "goodReason");

    const betree_var_t expected_set[] = { 1, 2, 5 };
    size_t found_total = 0;
    for(size_t j = 0; j < 3; j++) {
        for(size_t k = 0; k < 3; k++) {
            found_total += ((betree_var_t)rlist3->data[k] == (betree_var_t)expected_set[j]) ? 1 : 0;
        }
    }
    mu_assert(found_total == 3, "goodReason");

    write_dot_to_file_err(tree, "tests/beetree_search_reason_tests_multipl_bool_exprs.dot");

    free_report_err(report);
    betree_free_err(tree);
#if defined(DEBUG)
    fprintf(stderr, "\n");
#endif
    return 0;
}


int test_memoize_fail()
{
    struct betree_err* tree = betree_make_err();

    make_attr_domains(tree, ATTR_CONFIG_4(ATTR_BOOL, ATTR_INT, ATTR_FLOAT, ATTR_STR));

    const char* exprs[] = {
        "(b and i = 1 and f = 1.0 and s = \"s3\") or (s = \"s7\" and (not b))",
        "(b and i = 2 and f = 2.0 and s = \"s4\") or (s = \"s8\" and (not b))",
        "(b and i = 2 and f = 3.0 and s = \"s5\") or (s = \"s9\" and (not b))",
        "(b and i = 2 and f = 3.0 and s = \"s6\") or (s = \"s9\" and (not b))",
        "not (b and i = 2 and f = 3.0 and s = \"s6\") and (s = \"s9\" and (not b))",
    };
    const size_t exprs_count = 5;
    betree_bulk_insert(tree, exprs, exprs_count);

    betree_make_sub_ids(tree);

    const char* event = "{\"b\": false, \"i\": 3, \"f\": 0.0, \"s\": \"s12\"}";
    struct report_err* report = make_report_err(tree);
#if defined(DEBUG)
    fprintf(stderr, "search ... %s\n", event);
#endif
    betree_search_err(tree, event, report);

    mu_assert(report->matched == 0, "goodEvent");
    const betree_var_t expected_set[] = { 1, 2, 3, 4, 5 };
    size_t found_total = 0;
    dynamic_array_t* rlist = betree_reason_map_get(report->reason_sub_id_list, 3);
    mu_assert(rlist != NULL, "goodReport");
    for(size_t j = 0; j < 5; j++) {
        for(size_t k = 0; k < 5; k++) {
            found_total += ((betree_var_t)(rlist->data[k]) == (betree_var_t)expected_set[j]) ? 1 : 0;
        }
    }
    mu_assert(found_total == 5, "goodReason");

    // write_dot_to_file_err(tree, "tests/beetree_search_reason_tests_multipl_bool_exprs.dot");

    free_report_err(report);
    betree_free_err(tree);
#if defined(DEBUG)
    fprintf(stderr, "\n");
#endif
    return 0;
}

int test_all_search_term()
{
    struct betree_err* tree = betree_make_err();

    const size_t constant_count = 4;
    const struct betree_constant* constants[] = { betree_make_integer_constant("campaign_id", 10),
        betree_make_integer_constant("advertiser_id", 20),
        betree_make_integer_constant("flight_id", 30),
        betree_make_integer_constant("product_id", 40) };

    make_attr_domains(tree,
        ATTR_CONFIG_9(ATTR_BOOL,
            ATTR_INT,
            ATTR_FLOAT,
            ATTR_STR,
            ATTR_FCAP,
            ATTR_STR_LIST,
            ATTR_INT_LIST,
            ATTR_SEGMENTS,
            ATTR_INT64));

    const char* exprs[]
        = { "b and i = 10 and f > 3.13 and s = \"good\" and 1 in il and sl none of (\"good\") and "
            "segment_within(seg, 1, 20) and within_frequency_cap(\"flight\", \"ns\", 100, 0)",
              "i = 10 and f > 3.13 and s = \"good\" and 1 in il and sl none of (\"good\") and "
              "segment_within(seg, 1, 20) and within_frequency_cap(\"flight\", \"ns\", 100, 0)" };
    const size_t exprs_count = 2;

    betree_bulk_insert_with_constants(tree, exprs, exprs_count, constants, constant_count);

    betree_make_sub_ids(tree);

    const char* event = "{\"b\": true, \"i\": 10, \"f\": 3.14, \"s\": \"good\", \"il\": [1,2,3], "
                        "\"sl\": [\"bad\"], \"seg\": [[1, 20000001]], \"frequency_caps\": "
                        "[[\"flight\", 10, \"ns\", 0, 0]], \"now\": 100}";
    struct report_err* report = make_report_err(tree);
#if defined(DEBUG)
    fprintf(stderr, "search ... %s\n", event);
#endif
    betree_search_err(tree, event, report);

    mu_assert(report->matched == 0, "goodEvent");
    const betree_var_t expected_set[] = { 1, 2 };
    size_t found_total = 0;
    dynamic_array_t* rlist = betree_reason_map_get(report->reason_sub_id_list, 6);
    mu_assert(rlist != NULL, "goodReport");
    for(size_t j = 0; j < 2; j++) {
        for(size_t k = 0; k < 2; k++) {
            found_total += ((betree_var_t)(rlist->data[k]) == (betree_var_t)expected_set[j]) ? 1 : 0;
        }
    }
    mu_assert(found_total == 2, "goodReason");

    free_report_err(report);
    betree_free_err(tree);
#if defined(DEBUG)
    fprintf(stderr, "\n");
#endif
    return 0;
}


int test_event_search_reason()
{
    struct betree_err* tree = betree_make_err();

    make_attr_domains(tree, ATTR_CONFIG_4(ATTR_BOOL, ATTR_INT, ATTR_FLOAT, ATTR_STR));

    const char* exprs[] = {
        "b and i = 10 and f < 3.13 and s = \"good\"",
        "b and i = 10 and f > 3.13 and s = \"bad\"",
        "b and i = 10 and f < 3.13 and s = \"good\"",
        "not b and i = 11 and f > 3.13 and s = \"bad\"",
        "not b and i = 11 and f < 3.13 and s = \"good\"",
        "not b and i = 11 and f > 3.13 and s = \"bad\"",
        "not b and i = 11 and f < 3.13 and s = \"good\"",
    };
    const size_t exprs_count = 7;

    betree_bulk_insert(tree, exprs, exprs_count);

    betree_make_sub_ids(tree);

    const char* event = "{\"b\": true, \"i\": 10, \"f\": 3.14, \"s\": \"cool\"}";

    struct report_err* report = make_report_err(tree);
#if defined(DEBUG)
    fprintf(stderr, "search ... %s\n", event);
#endif
    betree_search_err(tree, event, report);

    mu_assert(report->matched == 0, "goodEvent");
    const betree_var_t expected_set1[] = { 1, 2, 3 };
    size_t found_total1 = 0;
    dynamic_array_t* rlist = betree_reason_map_get(report->reason_sub_id_list, 3);
    mu_assert(rlist != NULL, "goodReport");
    for(size_t j = 0; j < 3; j++) {
        for(size_t k = 0; k < 3; k++) {
            found_total1 += ((betree_var_t)(rlist->data[k]) == (betree_var_t)expected_set1[j]) ? 1 : 0;
        }
    }
    mu_assert(found_total1 == 3, "goodReason");

    const betree_var_t expected_set2[] = { 4, 5, 6, 7 };
    size_t found_total2 = 0;
    dynamic_array_t* rlist2 = betree_reason_map_get(report->reason_sub_id_list, 0);
    mu_assert(rlist2 != NULL, "goodReport");
    for(size_t j = 0; j < 4; j++) {
        for(size_t k = 0; k < 4; k++) {
            found_total2 += ((betree_var_t)(rlist2->data[k]) == (betree_var_t)expected_set2[j]) ? 1 : 0;
        }
    }
    mu_assert(found_total2 == 4, "goodReason");

    free_report_err(report);
    betree_free_err(tree);
#if defined(DEBUG)
    fprintf(stderr, "\n");
#endif
    return 0;
}


void make_attr_domains(struct betree_err* tree, size_t config)
{
    make_attr_domains_undefined(tree, config, 0);
}


void make_attr_domains_undefined(struct betree_err* tree, size_t config, size_t config_undefined)
{
    if((1 << ATTR_BOOL) & config)
        add_attr_domain_b(tree->config, "b", ((1 << ATTR_BOOL) & config_undefined));
    if((1 << ATTR_INT) & config)
        add_attr_domain_i(tree->config, "i", ((1 << ATTR_INT) & config_undefined));
    if((1 << ATTR_FLOAT) & config)
        add_attr_domain_f(tree->config, "f", ((1 << ATTR_FLOAT) & config_undefined));
    if((1 << ATTR_STR) & config)
        add_attr_domain_s(tree->config, "s", ((1 << ATTR_STR) & config_undefined));
    if((1 << ATTR_INT_LIST) & config)
        add_attr_domain_il(tree->config, "il", ((1 << ATTR_INT_LIST) & config_undefined));
    if((1 << ATTR_STR_LIST) & config)
        add_attr_domain_sl(tree->config, "sl", ((1 << ATTR_STR_LIST) & config_undefined));
    if((1 << ATTR_SEGMENTS) & config) {
        add_attr_domain_segments(tree->config, "seg", ((1 << ATTR_SEGMENTS) & config_undefined));
    }
    if((1 << ATTR_GEO) & config) {
        add_attr_domain_f(tree->config, "latitude", ((1 << ATTR_GEO) & config_undefined));
        add_attr_domain_f(tree->config, "longitude", ((1 << ATTR_GEO) & config_undefined));
    }
    if((1 << ATTR_FCAP) & config)
        add_attr_domain_frequency(
            tree->config, "frequency_caps", ((1 << ATTR_FCAP) & config_undefined));
    if((1 << ATTR_INT64) & config)
        add_attr_domain_ie(tree->config, "now", ((1 << ATTR_INT64) & config_undefined));
}


void betree_bulk_insert(struct betree_err* tree, const char** exprs, const size_t count)
{
    for(size_t i = 0; i < count; i++) {
        size_t idx = i + 1;
#if defined(DEBUG)
        fprintf(stderr, "betree_insert exprs[%d] ... %s\n", idx, exprs[i]);
#endif
        betree_insert_err(tree, idx, exprs[i]);
    }
}

void betree_bulk_insert_with_constants(struct betree_err* tree,
    const char** exprs,
    size_t exprs_count,
    const struct betree_constant** constants,
    size_t constants_count)
{
    for(size_t i = 0; i < exprs_count; i++) {
        size_t idx = i + 1;
#if defined(DEBUG)
        fprintf(stderr, "betree_insert exprs[%d] ... %s\n", idx, exprs[i]);
#endif
        const struct betree_sub* sub
            = betree_make_sub_err(tree, idx, constants_count, constants, exprs[i]);
        betree_insert_sub_err(tree, sub);
    }
}


int all_tests()
{
    mu_run_test(test_bool_fail);
    mu_run_test(test_int_fail);
    mu_run_test(test_float_fail);
    mu_run_test(test_bin_fail);
    mu_run_test(test_int_list_fail);
    mu_run_test(test_bin_list_fail);
    mu_run_test(test_segments_fail);
    mu_run_test(test_frequency_cap_fail);
    mu_run_test(test_geo_fail);
    mu_run_test(test_int64_fail);

    mu_run_test(test_short_circuit_fail);

    mu_run_test(test_multiple_bool_exprs_fail);

    mu_run_test(test_memoize_fail);

    mu_run_test(test_all_search_term);

    mu_run_test(test_event_search_reason);

    return 0;
}


RUN_TESTS()
