#pragma once

#include <stdbool.h>

#include "betree.h"
#include "config.h"
#include "memoize.h"
#include "value.h"
#include "var.h"
#include "betree_err.h"

bool match_node_err(const struct betree_variable** preds,
    const struct ast_node* node,
    struct memoize* memoize,
    struct report_err* report,
    char* last_reason,
    hashtable* memoize_table);
