#pragma once

struct config;
struct cnode;
struct betree;
struct betree_err;

void write_dot_file(const struct betree* tree);
void write_dot_to_file(const struct betree* tree, const char* fname);
void write_dot_to_file_err(const struct betree_err* tree, const char* fname);