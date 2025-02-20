#pragma once

struct cnode_err;
struct betree_err;

void write_dot_file_err(const struct betree_err* tree);
void write_dot_to_file_err(const struct betree_err* tree, const char* fname);
