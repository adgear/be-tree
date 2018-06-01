#include <stdio.h>

#include "ast.h"
#include "printer.h"
#include "utils.h"

const char* numeric_compare_value_to_string(struct numeric_compare_value value)
{
    char* expr;
    switch(value.value_type) {
        case AST_NUMERIC_COMPARE_VALUE_INTEGER:
            asprintf(&expr, "%ld", value.integer_value);
            break;
        case AST_NUMERIC_COMPARE_VALUE_FLOAT:
            asprintf(&expr, "%.2f", value.float_value);
            break;
        default:
            switch_default_error("Invalid numeric compare value type");
            break;
    }
    return expr;
}

const char* numeric_compare_op_to_string(enum ast_numeric_compare_e op)
{
    switch(op) {
        case AST_NUMERIC_COMPARE_LT:
            return "<";
        case AST_NUMERIC_COMPARE_LE:
            return "<=";
        case AST_NUMERIC_COMPARE_GT:
            return ">";
        case AST_NUMERIC_COMPARE_GE:
            return ">=";
        default:
            switch_default_error("Invalid numeric compare operation");
            return NULL;
    }
}

const char* equality_value_to_string(struct equality_value value)
{
    char* expr;
    switch(value.value_type) {
        case AST_EQUALITY_VALUE_INTEGER:
            asprintf(&expr, "%ld", value.integer_value);
            break;
        case AST_EQUALITY_VALUE_FLOAT:
            asprintf(&expr, "%.2f", value.float_value);
            break;
        case AST_EQUALITY_VALUE_STRING:
            asprintf(&expr, "\"%s\"", value.string_value.string);
            break;
        default:
            switch_default_error("Invalid equality value type");
            break;
    }
    return expr;
}

const char* equality_op_to_string(enum ast_equality_e op)
{
    switch(op) {
        case AST_EQUALITY_EQ:
            return "=";
        case AST_EQUALITY_NE:
            return "<>";
        default:
            switch_default_error("Invalid equality operation");
            return NULL;
    }
}

const char* set_left_value_to_string(struct set_left_value value)
{
    char* expr;
    switch(value.value_type) {
        case AST_SET_LEFT_VALUE_INTEGER:
            asprintf(&expr, "%ld", value.integer_value);
            break;
        case AST_SET_LEFT_VALUE_STRING:
            asprintf(&expr, "\"%s\"", value.string_value.string);
            break;
        case AST_SET_LEFT_VALUE_VARIABLE:
            asprintf(&expr, "%s", value.variable_value.attr);
            break;
        default:
            switch_default_error("Invalid set left value type");
            break;
    }
    return expr;
}

const char* set_right_value_to_string(struct set_right_value value)
{
    char* expr;
    switch(value.value_type) {
        case AST_SET_RIGHT_VALUE_VARIABLE:
            asprintf(&expr, "%s", value.variable_value.attr);
            break;
        case AST_SET_RIGHT_VALUE_INTEGER_LIST: {
            const char* list = integer_list_value_to_string(value.integer_list_value);
            asprintf(&expr, "(%s)", list);
            free((char*)list);
            break;
        }
        case AST_SET_RIGHT_VALUE_STRING_LIST: {
            const char* list = string_list_value_to_string(value.string_list_value);
            asprintf(&expr, "(%s)", list);
            free((char*)list);
            break;
        }
        default:
            switch_default_error("Invalid set right value type");
            break;
    }
    return expr;
}

const char* set_op_to_string(enum ast_set_e op)
{
    switch(op) {
        case AST_SET_IN:
            return "in";
        case AST_SET_NOT_IN:
            return "not in";
        default:
            switch_default_error("Invalid set op");
            return NULL;
    }
}

const char* list_value_to_string(struct list_value value)
{
    char* list;
    switch(value.value_type) {
        case AST_LIST_VALUE_INTEGER_LIST: {
            const char* inner = integer_list_value_to_string(value.integer_list_value);
            asprintf(&list, "(%s)", inner);
            free((char*)inner);
            break;
        }
        case AST_LIST_VALUE_STRING_LIST: {
            const char* inner = string_list_value_to_string(value.string_list_value);
            asprintf(&list, "(%s)", inner);
            free((char*)inner);
            break;
        }
        default: {
            switch_default_error("Invalid list value type");
            break;
        }
    }
    return list;
}

const char* list_op_to_string(enum ast_list_e op)
{
    switch(op) {
        case AST_LIST_ONE_OF: {
            return "one of";
        }
        case AST_LIST_NONE_OF: {
            return "none of";
        }
        case AST_LIST_ALL_OF: {
            return "all of";
        }
        default: {
            switch_default_error("Invalid list operation");
            return NULL;
        }
    }
}

char* ast_to_string(const struct ast_node* node)
{
    char* expr;
    switch(node->type) {
        case(AST_TYPE_SPECIAL_EXPR): {
            // TODO
            abort();
        }
        case(AST_TYPE_BOOL_EXPR): {
            switch(node->bool_expr.op) {
                case AST_BOOL_VARIABLE: {
                    asprintf(&expr, "%s", node->bool_expr.variable.attr);
                    return expr;
                }
                case AST_BOOL_NOT: {
                    const char* a = ast_to_string(node->bool_expr.unary.expr);
                    asprintf(&expr, "not (%s)", a);
                    free((char*)a);
                    return expr;
                }
                case AST_BOOL_OR: {
                    const char* a = ast_to_string(node->bool_expr.binary.lhs);
                    const char* b = ast_to_string(node->bool_expr.binary.rhs);
                    asprintf(&expr, "((%s) or (%s))", a, b);
                    free((char*)a);
                    free((char*)b);
                    return expr;
                }
                case AST_BOOL_AND: {
                    const char* a = ast_to_string(node->bool_expr.binary.lhs);
                    const char* b = ast_to_string(node->bool_expr.binary.rhs);
                    asprintf(&expr, "((%s) and (%s))", a, b);
                    free((char*)a);
                    free((char*)b);
                    return expr;
                }
                default: {
                    switch_default_error("Invalid bool operation");
                    return NULL;
                }
            }
        }
        case(AST_TYPE_SET_EXPR): {
            const char* left = set_left_value_to_string(node->set_expr.left_value);
            const char* right = set_right_value_to_string(node->set_expr.right_value);
            const char* op = set_op_to_string(node->set_expr.op);
            asprintf(&expr, "%s %s %s", left, op, right);
            free((char*)left);
            free((char*)right);
            return expr;
        }
        case(AST_TYPE_LIST_EXPR): {
            const char* value = list_value_to_string(node->list_expr.value);
            const char* op = list_op_to_string(node->list_expr.op);
            asprintf(&expr, "%s %s %s", node->list_expr.attr_var.attr, op, value);
            free((char*)value);
            return expr;
        }
        case(AST_TYPE_EQUALITY_EXPR): {
            const char* value = equality_value_to_string(node->equality_expr.value);
            const char* op = equality_op_to_string(node->equality_expr.op);
            asprintf(&expr, "%s %s %s", node->equality_expr.attr_var.attr, op, value);
            free((char*)value);
            return expr;
        }
        case(AST_TYPE_NUMERIC_COMPARE_EXPR): {
            const char* value = numeric_compare_value_to_string(node->numeric_compare_expr.value);
            const char* op = numeric_compare_op_to_string(node->numeric_compare_expr.op);
            asprintf(&expr, "%s %s %s", node->numeric_compare_expr.attr_var.attr, op, value);
            free((char*)value);
            return expr;
        }
        default: {
            switch_default_error("Invalid expr type");
            return NULL;
        }
    }
}
