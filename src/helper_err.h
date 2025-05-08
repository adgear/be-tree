#pragma once

struct betree_err;

void add_variable_from_string_err(struct betree_err* betree, const char* line);
void empty_tree_err(struct betree_err* betree);
