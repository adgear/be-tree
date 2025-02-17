#include <ctype.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "alloc.h"
#include "ast.h"
#include "betree.h"
#include "betree_err.h"
#include "error.h"
#include "hashmap.h"
#include "memoize.h"
#include "printer.h"
#include "special.h"
#include "utils.h"
#include "value.h"
#include "var.h"

static bool match_not_all_of_string(struct value variable, struct ast_list_expr list_expr)
{
    struct string_value* xs;
    size_t x_count;
    struct string_value* ys;
    size_t y_count;
    if(variable.string_list_value->count < list_expr.value.integer_list_value->count) {
        xs = variable.string_list_value->strings;
        x_count = variable.string_list_value->count;
        ys = list_expr.value.string_list_value->strings;
        y_count = list_expr.value.integer_list_value->count;
    }
    else {
        ys = variable.string_list_value->strings;
        y_count = variable.string_list_value->count;
        xs = list_expr.value.string_list_value->strings;
        x_count = list_expr.value.integer_list_value->count;
    }
    size_t i = 0, j = 0;
    while(i < x_count && j < y_count) {
        struct string_value* x = &xs[i];
        struct string_value* y = &ys[j];
        if(x->str == y->str) {
            return true;
        }
        if(y->str < x->str) {
            j++;
        }
        else {
            i++;
        }
    }
    return false;
}

static bool d64binary_search(const int64_t arr[], size_t count, int64_t to_find)
{
    int imin = 0;
    int imax = (int)count - 1;
    while(imax >= imin) {
        int imid = imin + ((imax - imin) / 2);
        if(arr[imid] == to_find) {
            return true;
        }
        if(arr[imid] < to_find) {
            imin = imid + 1;
        }
        else {
            imax = imid - 1;
        }
    }
    return false;
}

static bool sbinary_search(struct string_value arr[], size_t count, betree_str_t to_find)
{
    int imin = 0;
    int imax = (int)count - 1;
    while(imax >= imin) {
        int imid = imin + ((imax - imin) / 2);
        if(arr[imid].str == to_find) {
            return true;
        }
        if(arr[imid].str < to_find) {
            imin = imid + 1;
        }
        else {
            imax = imid - 1;
        }
    }
    return false;
}

static bool integer_in_integer_list(int64_t integer, struct betree_integer_list* list)
{
    return d64binary_search(list->integers, list->count, integer);
}

static bool string_in_string_list(struct string_value string, struct betree_string_list* list)
{
    return sbinary_search(list->strings, list->count, string.str);
}


static bool match_all_of_int(struct value variable, struct ast_list_expr list_expr)
{
    int64_t* xs = list_expr.value.integer_list_value->integers;
    size_t x_count = list_expr.value.integer_list_value->count;
    int64_t* ys = variable.integer_list_value->integers;
    size_t y_count = variable.integer_list_value->count;
    if(x_count <= y_count) {
        size_t from = 0, j = 0;
        while(from < y_count && j < x_count) {
            int64_t x = xs[j];
            from = next_low(ys, from, y_count, x);
            if(from < y_count && ys[from] == x) {
                j++;
            }
            else {
                return false;
            }
        }
        if(j < x_count) {
            return false;
        }
        return true;
    }
    return false;
}

static bool match_all_of_string(struct value variable, struct ast_list_expr list_expr)
{
    struct string_value* xs = list_expr.value.string_list_value->strings;
    size_t x_count = list_expr.value.string_list_value->count;
    struct string_value* ys = variable.string_list_value->strings;
    size_t y_count = variable.string_list_value->count;
    if(x_count <= y_count) {
        size_t i = 0, j = 0;
        while(i < y_count && j < x_count) {
            struct string_value* x = &xs[j];
            struct string_value* y = &ys[i];
            if(y->str < x->str) {
                i++;
            }
            else if(x->str == y->str) {
                i++;
                j++;
            }
            else {
                return false;
            }
        }
        if(j < x_count) {
            return false;
        }
        return true;
    }
    return false;
}

void set_reason_sub_id_list(char* last_reason, const char* variable_name)
{
    sprintf(last_reason, "%s", variable_name);
}

static bool match_node_inner_err(const struct betree_variable** preds,
    const struct ast_node* node,
    struct memoize* memoize,
    struct report_err* report,
    betree_var_t* last_reason,
    betree_var_t* memoize_reason,
    size_t attr_domain_count);

static bool match_special_expr_err(const struct betree_variable** preds,
    const struct ast_special_expr special_expr,
    betree_var_t* last_reason,
    size_t attr_domain_count)
{
    switch(special_expr.type) {
        case AST_SPECIAL_FREQUENCY: {
            switch(special_expr.frequency.op) {
                case AST_SPECIAL_WITHINFREQUENCYCAP: {
                    const struct ast_special_frequency* f = &special_expr.frequency;
                    struct betree_frequency_caps* caps;
                    bool is_caps_defined = get_frequency_var(f->attr_var.var, preds, &caps);
                    *last_reason = special_expr.frequency.attr_var.var;

                    if(is_caps_defined == false) {
                        return false;
                    }
                    if(caps->size == 0) {
                        // Optimization from looking at what within_frequency_caps does
                        return true;
                    }
                    int64_t now;
                    bool is_now_defined = get_integer_var(f->now.var, preds, &now);
                    if(is_now_defined == false) {
                        return false;
                    }
                    return within_frequency_caps(
                        caps, f->type, f->id, f->ns, f->value, f->length, now);
                }
                default:
                    abort();
            }
        }
        case AST_SPECIAL_SEGMENT: {
            *last_reason = special_expr.segment.attr_var.var;
            const struct ast_special_segment* s = &special_expr.segment;
            struct betree_segments* segments;
            bool is_segment_defined = get_segments_var(s->attr_var.var, preds, &segments);
            if(is_segment_defined == false) {
                return false;
            }
            int64_t now;
            bool is_now_defined = get_integer_var(s->now.var, preds, &now);
            if(is_now_defined == false) {
                return false;
            }
            switch(special_expr.segment.op) {
                case AST_SPECIAL_SEGMENTWITHIN:
                    return segment_within(s->segment_id, s->seconds, segments, now);
                case AST_SPECIAL_SEGMENTBEFORE:
                    return segment_before(s->segment_id, s->seconds, segments, now);
                default:
                    abort();
            }
        }
        case AST_SPECIAL_GEO: {
            switch(special_expr.geo.op) {
                case AST_SPECIAL_GEOWITHINRADIUS: {
                    *last_reason = ADDITIONAL_REASON(attr_domain_count, REASON_GEO);
                    const struct ast_special_geo* g = &special_expr.geo;
                    double latitude_var, longitude_var;
                    bool is_latitude_defined
                        = get_float_var(g->latitude_var.var, preds, &latitude_var);
                    bool is_longitude_defined
                        = get_float_var(g->longitude_var.var, preds, &longitude_var);
                    if(is_latitude_defined == false || is_longitude_defined == false) {
                        return false;
                    }
                    return geo_within_radius(
                        g->latitude, g->longitude, latitude_var, longitude_var, g->radius);
                }
                default:
                    abort();
            }
            return false;
        }
        case AST_SPECIAL_STRING: {
            *last_reason = special_expr.string.attr_var.var;
            const struct ast_special_string* s = &special_expr.string;
            struct string_value value;
            bool is_string_defined = get_string_var(s->attr_var.var, preds, &value);
            if(is_string_defined == false) {
                return false;
            }
            switch(s->op) {
                case AST_SPECIAL_CONTAINS:
                    return contains(value.string, s->pattern);
                case AST_SPECIAL_STARTSWITH:
                    return starts_with(value.string, s->pattern);
                case AST_SPECIAL_ENDSWITH:
                    return ends_with(value.string, s->pattern);
                default:
                    abort();
            }
            return false;
        }
        default:
            abort();
    }
}

static bool match_is_null_expr_err(const struct betree_variable** preds,
    const struct ast_is_null_expr is_null_expr,
    betree_var_t* last_reason)
{
    struct value variable;
    bool is_variable_defined = get_variable(is_null_expr.attr_var.var, preds, &variable);
    *last_reason = is_null_expr.attr_var.var;

    switch(is_null_expr.type) {
        case AST_IS_NULL:
            return !is_variable_defined;
        case AST_IS_NOT_NULL:
            return is_variable_defined;
        case AST_IS_EMPTY:
            return is_variable_defined && is_empty_list(variable);
        default:
            abort();
    }
}

static bool match_not_all_of_int(struct value variable, struct ast_list_expr list_expr)
{
    int64_t* xs;
    size_t x_count;
    int64_t* ys;
    size_t y_count;
    if(variable.integer_list_value->count < list_expr.value.integer_list_value->count) {
        xs = variable.integer_list_value->integers;
        x_count = variable.integer_list_value->count;
        ys = list_expr.value.integer_list_value->integers;
        y_count = list_expr.value.integer_list_value->count;
    }
    else {
        ys = variable.integer_list_value->integers;
        y_count = variable.integer_list_value->count;
        xs = list_expr.value.integer_list_value->integers;
        x_count = list_expr.value.integer_list_value->count;
    }
    size_t i = 0, from = 0;
    while(i < x_count && from < y_count) {
        int64_t x = xs[i];
        from = next_low(ys, from, y_count, x);
        // first check that new index is in array
        if(from < y_count && ys[from] == x) {
            return true;
        }
        else {
            i++;
        }
    }
    return false;
}

static void invalid_expr(const char* msg)
{
    fprintf(stderr, "%s", msg);
    abort();
}


static bool match_list_expr_err(const struct betree_variable** preds,
    const struct ast_list_expr list_expr,
    betree_var_t* last_reason)
{
    *last_reason = list_expr.attr_var.var;
    struct value variable;
    bool is_variable_defined = get_variable(list_expr.attr_var.var, preds, &variable);
    if(is_variable_defined == false) {
        return false;
    }
    switch(list_expr.op) {
        case AST_LIST_ONE_OF:
        case AST_LIST_NONE_OF: {
            bool result = false;
            switch(list_expr.value.value_type) {
                case AST_LIST_VALUE_INTEGER_LIST:
                    result = match_not_all_of_int(variable, list_expr);
                    break;
                case AST_LIST_VALUE_STRING_LIST: {
                    result = match_not_all_of_string(variable, list_expr);
                    break;
                }
                default:
                    abort();
            }
            switch(list_expr.op) {
                case AST_LIST_ONE_OF:
                    return result;
                case AST_LIST_NONE_OF:
                    return !result;
                case AST_LIST_ALL_OF:
                    invalid_expr("Should never happen");
                    return false;
                default:
                    abort();
            }
        }
        case AST_LIST_ALL_OF: {
            switch(list_expr.value.value_type) {
                case AST_LIST_VALUE_INTEGER_LIST:
                    if(match_all_of_int(variable, list_expr) == false) {
                        *last_reason = list_expr.attr_var.var;
                    }
                    return match_all_of_int(variable, list_expr);
                case AST_LIST_VALUE_STRING_LIST:
                    if(match_all_of_string(variable, list_expr) == false) {
                        *last_reason = list_expr.attr_var.var;
                    }
                    return match_all_of_string(variable, list_expr);
                default:
                    abort();
            }
        }
        default:
            abort();
    }
}

static bool match_set_expr_err(const struct betree_variable** preds,
    const struct ast_set_expr set_expr,
    betree_var_t* last_reason)
{
    struct set_left_value left = set_expr.left_value;
    struct set_right_value right = set_expr.right_value;
    bool is_in;

    if(left.value_type == AST_SET_LEFT_VALUE_INTEGER
        && right.value_type == AST_SET_RIGHT_VALUE_VARIABLE) {
        *last_reason = set_expr.right_value.variable_value.var;
        struct betree_integer_list* variable;
        bool is_variable_defined = get_integer_list_var(right.variable_value.var, preds, &variable);
        if(is_variable_defined == false) {
            return false;
        }
        is_in = integer_in_integer_list(left.integer_value, variable);
    }
    else if(left.value_type == AST_SET_LEFT_VALUE_STRING
        && right.value_type == AST_SET_RIGHT_VALUE_VARIABLE) {
        *last_reason = set_expr.right_value.variable_value.var;
        struct betree_string_list* variable;
        bool is_variable_defined = get_string_list_var(right.variable_value.var, preds, &variable);
        if(is_variable_defined == false) {
            return false;
        }
        is_in = string_in_string_list(left.string_value, variable);
    }
    else if(left.value_type == AST_SET_LEFT_VALUE_VARIABLE
        && right.value_type == AST_SET_RIGHT_VALUE_INTEGER_LIST) {
        *last_reason = set_expr.left_value.variable_value.var;
        int64_t variable;
        bool is_variable_defined = get_integer_var(left.variable_value.var, preds, &variable);
        if(is_variable_defined == false) {
            return false;
        }
        is_in = integer_in_integer_list(variable, right.integer_list_value);
    }
    else if(left.value_type == AST_SET_LEFT_VALUE_VARIABLE
        && right.value_type == AST_SET_RIGHT_VALUE_STRING_LIST) {
        *last_reason = set_expr.left_value.variable_value.var;
        struct string_value variable;
        bool is_variable_defined = get_string_var(left.variable_value.var, preds, &variable);
        if(is_variable_defined == false) {
            return false;
        }
        is_in = string_in_string_list(variable, right.string_list_value);
    }
    else {
        invalid_expr("invalid set expression");
        return false;
    }
    switch(set_expr.op) {
        case AST_SET_NOT_IN: {
            return !is_in;
        }
        case AST_SET_IN: {
            return is_in;
        }
        default:
            abort();
    }
}

static bool match_compare_expr_err(const struct betree_variable** preds,
    const struct ast_compare_expr compare_expr,
    betree_var_t* last_reason)
{
    *last_reason = compare_expr.attr_var.var;
    struct value variable;
    bool is_variable_defined = get_variable(compare_expr.attr_var.var, preds, &variable);
    if(is_variable_defined == false) {
        return false;
    }
    switch(compare_expr.op) {
        case AST_COMPARE_LT: {
            switch(compare_expr.value.value_type) {
                case AST_COMPARE_VALUE_INTEGER: {
                    bool result = variable.integer_value < compare_expr.value.integer_value;
                    return result;
                }
                case AST_COMPARE_VALUE_FLOAT: {
                    bool result = variable.float_value < compare_expr.value.float_value;
                    return result;
                }
                default:
                    abort();
            }
        }
        case AST_COMPARE_LE: {
            switch(compare_expr.value.value_type) {
                case AST_COMPARE_VALUE_INTEGER: {
                    bool result = variable.integer_value <= compare_expr.value.integer_value;
                    return result;
                }
                case AST_COMPARE_VALUE_FLOAT: {
                    bool result = variable.float_value <= compare_expr.value.float_value;
                    return result;
                }
                default:
                    abort();
            }
        }
        case AST_COMPARE_GT: {
            switch(compare_expr.value.value_type) {
                case AST_COMPARE_VALUE_INTEGER: {
                    bool result = variable.integer_value > compare_expr.value.integer_value;
                    return result;
                }
                case AST_COMPARE_VALUE_FLOAT: {
                    bool result = variable.float_value > compare_expr.value.float_value;
                    return result;
                }
                default:
                    abort();
            }
        }
        case AST_COMPARE_GE: {
            switch(compare_expr.value.value_type) {
                case AST_COMPARE_VALUE_INTEGER: {
                    bool result = variable.integer_value >= compare_expr.value.integer_value;
                    return result;
                }
                case AST_COMPARE_VALUE_FLOAT: {
                    bool result = variable.float_value >= compare_expr.value.float_value;
                    return result;
                }
                default:
                    abort();
            }
        }
        default:
            abort();
    }
}

static bool match_equality_expr_err(const struct betree_variable** preds,
    const struct ast_equality_expr equality_expr,
    betree_var_t* last_reason)
{
    struct value variable;
    *last_reason = equality_expr.attr_var.var;
    bool is_variable_defined = get_variable(equality_expr.attr_var.var, preds, &variable);
    if(is_variable_defined == false) {
        return false;
    }
    switch(equality_expr.op) {
        case AST_EQUALITY_EQ: {
            switch(equality_expr.value.value_type) {
                case AST_EQUALITY_VALUE_INTEGER: {
                    bool result = variable.integer_value == equality_expr.value.integer_value;
                    return result;
                }
                case AST_EQUALITY_VALUE_FLOAT: {
                    bool result = feq(variable.float_value, equality_expr.value.float_value);
                    return result;
                }
                case AST_EQUALITY_VALUE_STRING: {
                    bool result = variable.string_value.str == equality_expr.value.string_value.str;
                    if(result == false) return result;
                }
                case AST_EQUALITY_VALUE_INTEGER_ENUM: {
                    bool result = variable.integer_enum_value.ienum
                        == equality_expr.value.integer_enum_value.ienum;
                    return result;
                }
                default:
                    abort();
            }
        }
        case AST_EQUALITY_NE: {
            switch(equality_expr.value.value_type) {
                case AST_EQUALITY_VALUE_INTEGER: {
                    bool result = variable.integer_value != equality_expr.value.integer_value;
                    return result;
                }
                case AST_EQUALITY_VALUE_FLOAT: {
                    bool result = fne(variable.float_value, equality_expr.value.float_value);
                    return result;
                }
                case AST_EQUALITY_VALUE_STRING: {
                    bool result = variable.string_value.str != equality_expr.value.string_value.str;
                    return result;
                }
                case AST_EQUALITY_VALUE_INTEGER_ENUM: {
                    bool result = variable.integer_enum_value.ienum
                        != equality_expr.value.integer_enum_value.ienum;
                    return result;
                }
                default:
                    abort();
            }
        }
        default:
            abort();
    }
}

static bool match_bool_expr_err(const struct betree_variable** preds,
    const struct ast_bool_expr bool_expr,
    struct memoize* memoize,
    struct report_err* report,
    betree_var_t* last_reason,
    betree_var_t* memoize_reason,
    size_t attr_domain_count)
{
    switch(bool_expr.op) {
        case AST_BOOL_LITERAL:
            return bool_expr.literal;
        case AST_BOOL_AND: {
            bool lhs = match_node_inner_err(preds,
                bool_expr.binary.lhs,
                memoize,
                report,
                last_reason,
                memoize_reason,
                attr_domain_count);
            if(lhs == false) {
                return false;
            }
            bool rhs = match_node_inner_err(preds,
                bool_expr.binary.rhs,
                memoize,
                report,
                last_reason,
                memoize_reason,
                attr_domain_count);
            return rhs;
        }
        case AST_BOOL_OR: {
            bool lhs = match_node_inner_err(preds,
                bool_expr.binary.lhs,
                memoize,
                report,
                last_reason,
                memoize_reason,
                attr_domain_count);
            if(lhs == true) {
                return true;
            }
            bool rhs = match_node_inner_err(preds,
                bool_expr.binary.rhs,
                memoize,
                report,
                last_reason,
                memoize_reason,
                attr_domain_count);
            return rhs;
        }
        case AST_BOOL_NOT: {
            bool result = match_node_inner_err(preds,
                bool_expr.unary.expr,
                memoize,
                report,
                last_reason,
                memoize_reason,
                attr_domain_count);
            return !result;
        }
        case AST_BOOL_VARIABLE: {
            bool value;
            bool is_variable_defined = get_bool_var(bool_expr.variable.var, preds, &value);
            *last_reason = bool_expr.variable.var;

            if(is_variable_defined == false) {
                return false;
            }
            return value;
        }
        default:
            abort();
    }
}

static bool match_node_inner_err(const struct betree_variable** preds,
    const struct ast_node* node,
    struct memoize* memoize,
    struct report_err* report,
    betree_var_t* last_reason,
    betree_var_t* memoize_reason,
    size_t attr_domain_count)
{
    if(node->memoize_id != INVALID_PRED) {
        if(test_bit(memoize->pass, node->memoize_id)) {
            if(report != NULL) {
                report->memoized++;
            }
            *last_reason = memoize_reason[node->memoize_id];
            return true;
        }
        if(test_bit(memoize->fail, node->memoize_id)) {
            if(report != NULL) {
                report->memoized++;
            }
            *last_reason = memoize_reason[node->memoize_id];
            return false;
        }
    }
    bool result;
    switch(node->type) {
        case AST_TYPE_IS_NULL_EXPR:
            result = match_is_null_expr_err(preds, node->is_null_expr, last_reason);
            break;
        case AST_TYPE_SPECIAL_EXPR: {
            result
                = match_special_expr_err(preds, node->special_expr, last_reason, attr_domain_count);
            break;
        }
        case AST_TYPE_BOOL_EXPR: {
            result = match_bool_expr_err(preds,
                node->bool_expr,
                memoize,
                report,
                last_reason,
                memoize_reason,
                attr_domain_count);
            break;
        }
        case AST_TYPE_LIST_EXPR: {
            result = match_list_expr_err(preds, node->list_expr, last_reason);
            break;
        }
        case AST_TYPE_SET_EXPR: {
            result = match_set_expr_err(preds, node->set_expr, last_reason);
            break;
        }
        case AST_TYPE_COMPARE_EXPR: {
            result = match_compare_expr_err(preds, node->compare_expr, last_reason);
            break;
        }
        case AST_TYPE_EQUALITY_EXPR: {
            result = match_equality_expr_err(preds, node->equality_expr, last_reason);
            break;
        }
        default:
            abort();
    }
    if(node->memoize_id != INVALID_PRED) {
        if(result) {
            set_bit(memoize->pass, node->memoize_id);
            memoize_reason[node->memoize_id] = *last_reason;
        }
        else {
            set_bit(memoize->fail, node->memoize_id);
            memoize_reason[node->memoize_id] = *last_reason;
        }
    }
    return result;
}

bool match_node_err(const struct betree_variable** preds,
    const struct ast_node* node,
    struct memoize* memoize,
    struct report_err* report,
    betree_var_t* last_reason,
    betree_var_t* memoize_reason,
    size_t attr_domain_count)
{
    return match_node_inner_err(
        preds, node, memoize, report, last_reason, memoize_reason, attr_domain_count);
}
