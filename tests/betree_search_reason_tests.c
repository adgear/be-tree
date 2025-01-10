#include <float.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "alloc.h"
#include "betree.h"
#include "debug.h"
#include "helper.h"
#include "minunit.h"
#include "printer.h"
#include "special.h"
#include "tree.h"
#include "utils.h"

#define DEBUG 1

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


void make_attr_domains(struct betree* tree, size_t config);
void make_attr_domains_undefined(struct betree* tree, size_t config, size_t config_undefined);
void betree_bulk_insert(struct betree* tree, const char** exprs, int count);
void betree_bulk_insert_with_constants(struct betree* tree,
    const char** exprs,
    int exprs_count,
    struct betree_constant** constants,
    int constants_count);

int test_bool_fail()
{
    struct betree* tree = betree_make();

    make_attr_domains(tree, ATTR_CONFIG_2(ATTR_BOOL, ATTR_INT));

    const char* exprs[] = { "b and i = 1" };
    const size_t exprs_count = 1;
    betree_bulk_insert(tree, exprs, exprs_count);

    const char* event = "{\"b\": false, \"i\": 1}";
    struct report* report = make_report();
#if defined(DEBUG)
    fprintf(stderr, "search ... %s\n", event);
#endif
    /* TODO: change function & asserts to check fail reasons */
    betree_search(tree, event, report);
    mu_assert(report->matched == 0, "goodEvent");

    // write_dot_to_file(tree, "tests/beetree_search_reason_tests.dot");

    free_report(report);
    betree_free(tree);
#if defined(DEBUG)
    fprintf(stderr, "\n");
#endif
    return 0;
}


int test_int_fail()
{
    struct betree* tree = betree_make();

    make_attr_domains(tree, ATTR_CONFIG_2(ATTR_BOOL, ATTR_INT));

    const char* exprs[] = { "b and i = 1" };
    const size_t exprs_count = 1;
    betree_bulk_insert(tree, exprs, exprs_count);

    const char* event = "{\"b\": true, \"i\": 2}";
    struct report* report = make_report();
#if defined(DEBUG)
    fprintf(stderr, "search ... %s\n", event);
#endif
    /* TODO: change function & asserts to check fail reasons */
    betree_search(tree, event, report);
    mu_assert(report->matched == 0, "goodEvent");

    // write_dot_to_file(tree, "tests/beetree_search_reason_tests.dot");

    free_report(report);
    betree_free(tree);
#if defined(DEBUG)
    fprintf(stderr, "\n");
#endif
    return 0;
}


int test_float_fail()
{
    struct betree* tree = betree_make();

    make_attr_domains(tree, ATTR_CONFIG_2(ATTR_BOOL, ATTR_FLOAT));

    const char* exprs[] = { "b and f = 0.1" };
    const size_t exprs_count = 1;
    betree_bulk_insert(tree, exprs, exprs_count);

    const char* event = "{\"b\": true, \"f\": 0.2}";
    struct report* report = make_report();
#if defined(DEBUG)
    fprintf(stderr, "search ... %s\n", event);
#endif
    /* TODO: change function & asserts to check fail reasons */
    betree_search(tree, event, report);
    mu_assert(report->matched == 0, "goodEvent");

    // write_dot_to_file(tree, "tests/beetree_search_reason_tests.dot");

    free_report(report);
    betree_free(tree);
#if defined(DEBUG)
    fprintf(stderr, "\n");
#endif
    return 0;
}


int test_bin_fail()
{
    struct betree* tree = betree_make();

    make_attr_domains(tree, ATTR_CONFIG_2(ATTR_BOOL, ATTR_STR));

    const char* exprs[] = { "b and s = \"betrees\"" };
    const size_t exprs_count = 1;
    betree_bulk_insert(tree, exprs, exprs_count);

    const char* event = "{\"b\": true, \"s\": \"betree\"}";
    struct report* report = make_report();
#if defined(DEBUG)
    fprintf(stderr, "search ... %s\n", event);
#endif
    /* TODO: change function & asserts to check fail reasons */
    betree_search(tree, event, report);
    mu_assert(report->matched == 0, "goodEvent");

    // write_dot_to_file(tree, "tests/beetree_search_reason_tests.dot");

    free_report(report);
    betree_free(tree);
#if defined(DEBUG)
    fprintf(stderr, "\n");
#endif
    return 0;
}


int test_int_list_fail()
{
    struct betree* tree = betree_make();

    make_attr_domains(tree, ATTR_CONFIG_2(ATTR_BOOL, ATTR_INT_LIST));

    const char* exprs[] = { "b and il one of (1,2)" };
    const size_t exprs_count = 1;
    betree_bulk_insert(tree, exprs, exprs_count);

    const char* event = "{\"b\": true, \"il\": [3]}";
    struct report* report = make_report();
#if defined(DEBUG)
    fprintf(stderr, "search ... %s\n", event);
#endif
    /* TODO: change function & asserts to check fail reasons */
    betree_search(tree, event, report);
    mu_assert(report->matched == 0, "goodEvent");

    // write_dot_to_file(tree, "tests/beetree_search_reason_tests.dot");

    free_report(report);
    betree_free(tree);
#if defined(DEBUG)
    fprintf(stderr, "\n");
#endif
    return 0;
}


int test_bin_list_fail()
{
    struct betree* tree = betree_make();

    make_attr_domains(tree, ATTR_CONFIG_2(ATTR_BOOL, ATTR_STR_LIST));

    const char* exprs[] = { "b and sl one of (\"How\",\"is\", \"it\", \"going\")" };
    const size_t exprs_count = 1;
    betree_bulk_insert(tree, exprs, exprs_count);

    const char* event = "{\"b\": true, \"sl\": [\"how\"]}";
    struct report* report = make_report();
#if defined(DEBUG)
    fprintf(stderr, "search ... %s\n", event);
#endif
    /* TODO: change function & asserts to check fail reasons */
    betree_search(tree, event, report);
    mu_assert(report->matched == 0, "goodEvent");

    // write_dot_to_file(tree, "tests/beetree_search_reason_tests.dot");

    free_report(report);
    betree_free(tree);
#if defined(DEBUG)
    fprintf(stderr, "\n");
#endif
    return 0;
}


int test_segments_fail()
{
    struct betree* tree = betree_make();

    make_attr_domains(tree, ATTR_CONFIG_2(ATTR_INT64, ATTR_SEGMENTS));

    const char* exprs[] = { "segment_within(1, 10)" };
    const size_t exprs_count = 1;
    betree_bulk_insert(tree, exprs, exprs_count);

    const char* event
        = "{\"now\": 30, \"seg\": [[1, 10000000]], \"segments_with_timestamp\": [[1, 10000000]]}";
    struct report* report = make_report();
#if defined(DEBUG)
    fprintf(stderr, "search ... %s\n", event);
#endif
    /* TODO: change function & asserts to check fail reasons */
    betree_search(tree, event, report);
    mu_assert(report->matched == 0, "goodEvent");

    // write_dot_to_file(tree, "tests/beetree_search_reason_tests.dot");

    free_report(report);
    betree_free(tree);
#if defined(DEBUG)
    fprintf(stderr, "\n");
#endif
    return 0;
}


int test_frequency_cap_fail()
{
    struct betree* tree = betree_make();

    const int constant_count = 1;
    const struct betree_constant* constants[]
        = { betree_make_integer_constant("advertiser_id", 20) };

    make_attr_domains(tree, ATTR_CONFIG_2(ATTR_INT64, ATTR_FCAP));

    const char* exprs[] = { "not within_frequency_cap(\"advertiser\", \"namespace\", 100, 100)" };
    const size_t exprs_count = 1;
    betree_bulk_insert_with_constants(tree, exprs, exprs_count, constants, constant_count);

    const char* event
        = "{\"now\": 30, \"frequency_caps\": [[\"campaign\", 30, \"namespace\", 20, 10]]}";
    struct report* report = make_report();
#if defined(DEBUG)
    fprintf(stderr, "search ... %s\n", event);
#endif
    /* TODO: change function & asserts to check fail reasons */
    betree_search(tree, event, report);
    mu_assert(report->matched == 0, "goodEvent");

    // write_dot_to_file(tree, "tests/beetree_search_reason_tests.dot");

    free_report(report);
    betree_free(tree);
#if defined(DEBUG)
    fprintf(stderr, "\n");
#endif
    return 0;
}


int test_geo_fail()
{
    struct betree* tree = betree_make();

    make_attr_domains(tree, ATTR_CONFIG_2(ATTR_BOOL, ATTR_GEO));

    const char* exprs[] = { "b and geo_within_radius(10, 100, 100)" };
    const size_t exprs_count = 1;
    betree_bulk_insert(tree, exprs, exprs_count);

    const char* event = "{\"b\": true, \"latitude\": 101.0, \"longitude\": 99.0}";
    struct report* report = make_report();
#if defined(DEBUG)
    fprintf(stderr, "search ... %s\n", event);
#endif
    /* TODO: change function & asserts to check fail reasons */
    betree_search(tree, event, report);
    mu_assert(report->matched == 0, "goodEvent");

    // write_dot_to_file(tree, "tests/beetree_search_reason_tests.dot");

    free_report(report);
    betree_free(tree);
#if defined(DEBUG)
    fprintf(stderr, "\n");
#endif
    return 0;
}


int test_int64_fail()
{
    struct betree* tree = betree_make();

    make_attr_domains(tree, ATTR_CONFIG_2(ATTR_BOOL, ATTR_INT64));

    const char* exprs[] = { "b and now = 1" };
    const size_t exprs_count = 1;
    betree_bulk_insert(tree, exprs, exprs_count);

    const char* event = "{\"b\": true, \"now\": 2}";
    struct report* report = make_report();
#if defined(DEBUG)
    fprintf(stderr, "search ... %s\n", event);
#endif
    /* TODO: change function & asserts to check fail reasons */
    betree_search(tree, event, report);
    mu_assert(report->matched == 0, "goodEvent");

    // write_dot_to_file(tree, "tests/beetree_search_reason_tests.dot");

    free_report(report);
    betree_free(tree);
#if defined(DEBUG)
    fprintf(stderr, "\n");
#endif
    return 0;
}


int test_short_circuit_fail()
{
    struct betree* tree = betree_make();

    make_attr_domains_undefined(tree,
        ATTR_CONFIG_5(ATTR_BOOL, ATTR_INT, ATTR_FLOAT, ATTR_STR, ATTR_INT_LIST),
        ATTR_CONFIG_3(ATTR_FLOAT, ATTR_STR, ATTR_INT_LIST));

    const char* exprs[] = { "b and i = 1 and f = 0.1 and s = \"s1\"",
        "b and i = 2 and s = \"s2\"",
        "b and i = 3 and (il one of (1, 2, 3))" };
    const size_t exprs_count = 3;
    betree_bulk_insert(tree, exprs, exprs_count);

    const char* event = "{\"b\": true, \"i\": 0}";
    struct report* report = make_report();
#if defined(DEBUG)
    fprintf(stderr, "search ... %s\n", event);
#endif
    /* TODO: change function & asserts to check fail reasons */
    betree_search(tree, event, report);
    mu_assert(report->matched == 0, "goodEvent");

    // write_dot_to_file(tree, "tests/beetree_search_reason_tests_short_circuit.dot");

    free_report(report);
    betree_free(tree);
#if defined(DEBUG)
    fprintf(stderr, "\n");
#endif
    return 0;
}


int test_multiple_bool_exprs_fail()
{
    struct betree* tree = betree_make();

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

    const char* event = "{\"b\": false, \"i\": 2, \"f\": 0.2, \"s\": \"s3\"}";
    struct report* report = make_report();
#if defined(DEBUG)
    fprintf(stderr, "search ... %s\n", event);
#endif
    betree_search(tree, event, report);
    mu_assert(report->matched == 0, "goodEvent");

    write_dot_to_file(tree, "tests/beetree_search_reason_tests_multipl_bool_exprs.dot");

    free_report(report);
    betree_free(tree);
#if defined(DEBUG)
    fprintf(stderr, "\n");
#endif
    return 0;
}


int test_memoize_fail()
{
    struct betree* tree = betree_make();

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

    const char* event = "{\"b\": false, \"i\": 3, \"f\": 0.0, \"s\": \"s12\"}";
    struct report* report = make_report();
#if defined(DEBUG)
    fprintf(stderr, "search ... %s\n", event);
#endif
    betree_search(tree, event, report);
    mu_assert(report->matched == 0, "goodEvent");

    write_dot_to_file(tree, "tests/beetree_search_reason_tests_multipl_bool_exprs.dot");

    free_report(report);
    betree_free(tree);
#if defined(DEBUG)
    fprintf(stderr, "\n");
#endif
    return 0;
}


void make_attr_domains(struct betree* tree, size_t config)
{
    make_attr_domains_undefined(tree, config, 0);
}


void make_attr_domains_undefined(struct betree* tree, size_t config, size_t config_undefined)
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
        add_attr_domain_segments(
            tree->config, "segments_with_timestamp", ((1 << ATTR_SEGMENTS) & config_undefined));
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


void betree_bulk_insert(struct betree* tree, const char** exprs, int count)
{
    for(size_t i = 0; i < count; i++) {
        int idx = i + 1;
#if defined(DEBUG)
        fprintf(stderr, "betree_insert exprs[%d] ... %s\n", idx, exprs[i]);
#endif
        mu_assert(betree_insert(tree, idx, exprs[i]), "");
    }
}


void betree_bulk_insert_with_constants(struct betree* tree,
    const char** exprs,
    int exprs_count,
    struct betree_constant** constants,
    int constants_count)
{
    for(size_t i = 0; i < exprs_count; i++) {
        int idx = i + 1;
#if defined(DEBUG)
        fprintf(stderr, "betree_insert exprs[%d] ... %s\n", idx, exprs[i]);
#endif
        mu_assert(
            betree_insert_with_constants(tree, idx, constants_count, constants, exprs[i]), "");
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

    return 0;
}


RUN_TESTS()
