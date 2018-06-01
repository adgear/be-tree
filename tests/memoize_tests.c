#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ast.h"
#include "hashmap.h"
#include "parser.h"
#include "memoize.h"
#include "minunit.h"
#include "utils.h"
#include "value.h"
#include "var.h"

extern bool MATCH_NODE_DEBUG;

struct report test(const char* expr_a, const char* expr_b, const char* event, struct config* config)
{
    if(config->pred_map == NULL) {
        config->pred_map = make_pred_map();
    }
    struct cnode* cnode = make_cnode(config, NULL);
    betree_insert(config, 1, expr_a, cnode);
    betree_insert(config, 2, expr_b, cnode);
    struct matched_subs* matched_subs = make_matched_subs();
    struct report report = make_empty_report();
    betree_search(config, event, cnode, matched_subs, &report);
    free_cnode(cnode);
    free_matched_subs(matched_subs);
    free_pred_map(config->pred_map);
    config->pred_map = NULL;
    return report;
}

bool test_same(const char* expr, const char* event, struct config* config)
{
    struct report report = test(expr, expr, event, config);
    return
      report.expressions_evaluated == 2 &&
      report.expressions_matched == 2 &&
      report.expressions_memoized == 1;
}

bool test_diff(const char* expr_a, const char* expr_b, const char* event, struct config* config)
{
    struct report report = test(expr_a, expr_b, event, config);
    return
      report.expressions_evaluated == 2 &&
      report.expressions_matched == 2 &&
      report.expressions_memoized == 0;
}

bool test_same_sub(const char* expr_a, const char* expr_b, const char* event, size_t memoized, struct config* config)
{
    struct report report = test(expr_a, expr_b, event, config);
    return
      report.expressions_evaluated == 2 &&
      report.expressions_matched == 2 &&
      report.expressions_memoized == 0 &&
      report.sub_expressions_memoized == memoized;
}

int test_numeric_compare_integer()
{
    struct config* config = make_default_config();
    add_attr_domain_i(config, "i", 0, 10, false);

    mu_assert(test_same("i > 1", "{\"i\": 2}", config), "integer gt");
    mu_assert(test_same("i >= 1", "{\"i\": 2}", config), "integer ge");
    mu_assert(test_same("i < 1", "{\"i\": 0}", config), "integer lt");
    mu_assert(test_same("i <= 1", "{\"i\": 1}", config), "integer le");
    mu_assert(test_diff("i > 0", "i > 1", "{\"i\": 2}", config), "integer diff");

    free_config(config);
    return 0;
}

int test_numeric_compare_float()
{
    struct config* config = make_default_config();
    add_attr_domain_f(config, "f", 0., 10., false);

    mu_assert(test_same("f > 1.", "{\"f\": 2.}", config), "float gt");
    mu_assert(test_same("f >= 1.", "{\"f\": 2.}", config), "float ge");
    mu_assert(test_same("f < 1.", "{\"f\": 0.}", config), "float lt");
    mu_assert(test_same("f <= 1.", "{\"f\": 1.}", config), "float le");
    mu_assert(test_diff("f > 0.", "f > 1.", "{\"f\": 2.}", config), "float diff");

    free_config(config);
    return 0;
}

int test_equality_integer()
{
    struct config* config = make_default_config();
    add_attr_domain_i(config, "i", 0, 10, false);

    mu_assert(test_same("i = 1", "{\"i\": 1}", config), "integer eq");
    mu_assert(test_same("i <> 1", "{\"i\": 0}", config), "integer ne");
    mu_assert(test_diff("i <> 0", "i <> 1", "{\"i\": 2}", config), "integer diff");

    free_config(config);
    return 0;
}

int test_equality_float()
{
    struct config* config = make_default_config();
    add_attr_domain_f(config, "f", 0., 10., false);

    mu_assert(test_same("f = 1.", "{\"f\": 1.}", config), "float eq");
    mu_assert(test_same("f <> 1.", "{\"f\": 0.}", config), "float ne");
    mu_assert(test_diff("f <> 0.", "f <> 1.", "{\"f\": 2.}", config), "float diff");

    free_config(config);
    return 0;
}

int test_equality_string()
{
    struct config* config = make_default_config();
    add_attr_domain_s(config, "s", false);

    mu_assert(test_same("s = \"a\"", "{\"s\": \"a\"}", config), "string eq");
    mu_assert(test_same("s <> \"a\"", "{\"s\": \"b\"}", config), "string ne");
    mu_assert(test_diff("s <> \"a\"", "s <> \"b\"", "{\"s\": \"c\"}", config), "string diff");

    free_config(config);
    return 0;
}

int test_set_var_integer()
{
    struct config* config = make_default_config();
    add_attr_domain_i(config, "i", 0, 10, false);

    mu_assert(test_same("i in (1,2)", "{\"i\": 1}", config), "integer set var in");
    mu_assert(test_same("i not in (1,2)", "{\"i\": 3}", config), "integer set var not in");
    mu_assert(test_diff("i in (1, 3)", "i in (1, 2)", "{\"i\": 1}", config), "integer set var diff");

    free_config(config);
    return 0;
}

int test_set_var_string()
{
    struct config* config = make_default_config();
    add_attr_domain_s(config, "s", false);

    mu_assert(test_same("s in (\"1\",\"2\")", "{\"s\": \"1\"}", config), "string set var in");
    mu_assert(test_same("s not in (\"1\",\"2\")", "{\"s\": \"3\"}", config), "string set var not in");
    mu_assert(test_diff("s in (\"1\",\"3\")", "s in (\"1\",\"2\")", "{\"s\": \"1\"}", config), "string set var diff");

    free_config(config);
    return 0;
}

int test_set_list_integer()
{
    struct config* config = make_default_config();
    add_attr_domain_il(config, "il", false);

    mu_assert(test_same("1 in il", "{\"il\": [1, 2]}", config), "integer set list in");
    mu_assert(test_same("1 not in il", "{\"il\": [2, 3]}", config), "integer set list not in");
    mu_assert(test_diff("1 in il", "2 in il", "{\"il\": [1, 2]}", config), "integer set list diff");

    free_config(config);
    return 0;
}

int test_set_list_string()
{
    struct config* config = make_default_config();
    add_attr_domain_sl(config, "sl", false);

    mu_assert(test_same("\"1\" in sl", "{\"sl\": [\"1\", \"2\"]}", config), "string set list in");
    mu_assert(test_same("\"1\" not in sl", "{\"sl\": [\"2\", \"3\"]}", config), "string set list not in");
    mu_assert(test_diff("\"1\" in sl", "\"2\" in sl", "{\"sl\": [\"1\", \"2\"]}", config), "string set list diff");

    free_config(config);
    return 0;
}

int test_list_integer()
{
    struct config* config = make_default_config();
    add_attr_domain_il(config, "il", false);

    mu_assert(test_same("il one of (1, 2)", "{\"il\": [1, 2]}", config), "integer list one of");
    mu_assert(test_same("il none of (1, 2)", "{\"il\": [3, 4]}", config), "integer list none of");
    mu_assert(test_same("il all of (1, 2)", "{\"il\": [1, 2]}", config), "integer list all of");
    mu_assert(test_diff("il one of (1, 2)", "il one of (1, 3)", "{\"il\": [1, 2]}", config), "integer list diff");

    free_config(config);
    return 0;
}

int test_list_string()
{
    struct config* config = make_default_config();
    add_attr_domain_sl(config, "sl", false);

    mu_assert(test_same("sl one of (\"1\", \"2\")", "{\"sl\": [\"1\", \"2\"]}", config), "string list one of");
    mu_assert(test_same("sl none of (\"1\", \"2\")", "{\"sl\": [\"3\", \"4\"]}", config), "string list none of");
    mu_assert(test_same("sl all of (\"1\", \"2\")", "{\"sl\": [\"1\", \"2\"]}", config), "string list all of");
    mu_assert(test_diff("sl one of (\"1\", \"2\")", "sl one of (\"1\", \"3\")", "{\"sl\": [\"1\", \"2\"]}", config), "string list diff");

    free_config(config);
    return 0;
}

int test_special_frequency()
{
    //struct config* config = make_default_config();

    // "within_frequency_cap(\"flight\", \"namespace\", 1, 2)"

    return 0;
}

int test_special_segment()
{
    //struct config* config = make_default_config();

    // "segment_within(1, 2)"
    // "segment_within(segment, 1, 2)"
    // "segment_before(1, 2)"
    // "segment_before(segment, 1, 2)"

    return 0;
}

int test_special_geo()
{
    //struct config* config = make_default_config();

    // "geo_within_radius(1, 2, 3)"
    // "geo_within_radius(1., 2., 3.)"

    return 0;
}

int test_special_string()
{
    //struct config* config = make_default_config();
    //add_attr_domain_s(config, "s", false);
    //add_attr_domain_s(config, "s2", false);

    // "contains(s, \"abc\")"
    // "starts_with(s, \"abc\")"
    // "ends_with(s, \"abc\")"

    return 0;
}

int test_bool()
{
    struct config* config = make_default_config();
    add_attr_domain_b(config, "b", false, true, true);
    add_attr_domain_i(config, "i", 0, 10, true);

    mu_assert(test_same("b", "{\"b\": true}", config), "bool var");
    mu_assert(test_same("not b", "{\"b\": false}", config), "bool not");
    mu_assert(test_same("b and b", "{\"b\": true}", config), "bool and");
    mu_assert(test_same("b or b", "{\"b\": true}", config), "bool or");
    mu_assert(test_same("not (i = 0)", "{\"i\": 1}", config), "bool not complex");
    mu_assert(test_same("(i = 0) and (i = 0)", "{\"i\": 0}", config), "bool and complex");
    mu_assert(test_diff("(i = 0) or (i = 1)", "(i = 0) or (i = 2)", "{\"i\": 0}", config), "bool diff");

    free_config(config);
    return 0;
}

int test_sub()
{
    struct config* config = make_default_config();
    add_attr_domain_i(config, "i", 0, 10, true);
    add_attr_domain_b(config, "b", false, true, true);
    add_attr_domain_il(config, "il", true);
    add_attr_domain_sl(config, "sl", true);

    mu_assert(test_same_sub("(i = 0) or (i = 1)", "(i = 0) or (i = 2)", "{\"i\": 0}", 1, config), "");
    mu_assert(test_same_sub(
        "((((not b) or (i = 6 and (\"s2\" in sl)))) and (il one of (2, 3)))",
        "((((not b) or (i = 6 and (\"s2\" in sl)))) and (il one of (2, 4)))",
        "{\"b\": false, \"i\": 6, \"sl\": [\"s1\",\"s2\"], \"il\": [1, 2]}",
        1, config), "whole left side of and");

    free_config(config);
    return 0;
}

int test_bit_logic()
{
    enum { pred_count = 250 };
    struct memoize memoize = make_memoize(pred_count);
    for(size_t i = 0; i < pred_count; i++) {
        if(i % 2 == 0) {
            set_bit(memoize.pass, i);
        }
        else {
            set_bit(memoize.fail, i);
        }
        for(size_t j = 0; j < pred_count; j++) {
            if(j <= i) {
                if(j % 2 == 0) {
                    mu_assert(test_bit(memoize.pass, j), "even, pass for %zu, at %zu", j, i);
                    mu_assert(!test_bit(memoize.fail, j), "even, fail for %zu, at %zu", j, i);
                }
                else {
                    mu_assert(!test_bit(memoize.pass, j), "odd, pass for %zu, at %zu", j, i);
                    mu_assert(test_bit(memoize.fail, j), "odd, fail for %zu, at %zu", j, i);
                }
            }
            else {
                mu_assert(!test_bit(memoize.pass, j), "new, pass for %zu, at %zu", j, i);
                mu_assert(!test_bit(memoize.fail, j), "new, fail for %zu, at %zu", j, i);
            }
        }
    }
    free_memoize(memoize);
    return 0;
}

int all_tests() 
{
    mu_run_test(test_numeric_compare_integer);
    mu_run_test(test_numeric_compare_float);
    mu_run_test(test_equality_integer);
    mu_run_test(test_equality_float);
    mu_run_test(test_equality_string);
    mu_run_test(test_set_var_integer);
    mu_run_test(test_set_var_string);
    mu_run_test(test_set_list_integer);
    mu_run_test(test_set_list_string);
    mu_run_test(test_list_integer);
    mu_run_test(test_list_string);
    mu_run_test(test_special_frequency);
    mu_run_test(test_special_segment);
    mu_run_test(test_special_geo);
    mu_run_test(test_special_string);
    mu_run_test(test_bool);
    mu_run_test(test_sub);
    mu_run_test(test_bit_logic);

    return 0;
}

RUN_TESTS()

