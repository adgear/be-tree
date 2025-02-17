#pragma once

#include <stdbool.h>

#include "betree.h"
#include "betree_err.h"
#include "config.h"
#include "memoize.h"
#include "value.h"
#include "var.h"

bool match_node_err(const struct betree_variable** preds,
    const struct ast_node* node,
    struct memoize* memoize,
    struct report_err* report,
    betree_var_t* last_reason,
    betree_var_t* memoize_reason,
    size_t attr_domains_count);

void set_reason_sub_id_list(char* last_reason, const char* variable_name);
