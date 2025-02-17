#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include "betree.h"
#include "betree_err.h"
#include "config.h"
#include "memoize.h"
#include "value.h"

struct cnode_err;

struct lnode_err {
    struct cnode_err* parent;
    struct {
        size_t sub_count;
        struct betree_sub** subs;
    };
    size_t max;
};

struct pdir_err;
struct pnode_err;
struct cdir_err;

struct cnode_err {
    struct cdir_err* parent;
    struct lnode_err* lnode;
    struct pdir_err* pdir;
};

struct pnode_err {
    struct pdir_err* parent;
    struct attr_var attr_var;
    struct cdir_err* cdir;
    float score;
};

struct cdir_err {
    enum c_parent_e parent_type;
    union {
        struct pnode_err* pnode_parent;
        struct cdir_err* cdir_parent;
    };
    struct attr_var attr_var;
    struct value_bound bound;
    struct cnode_err* cnode;
    struct cdir_err* lchild;
    struct cdir_err* rchild;
    struct arraylist* sub_ids;
};

struct pdir_err {
    struct cnode_err* parent;
    struct {
        size_t pnode_count;
        struct pnode_err** pnodes;
    };
};

void match_be_tree_err(const struct config* config,
    const struct betree_variable** preds,
    const struct cnode_err* cnode,
    struct subs_to_eval* subs,
    struct report_err* report);
void match_be_tree_node_counting_err(const struct config* config,
    const struct betree_variable** preds,
    const struct cnode_err* cnode,
    struct subs_to_eval* subs,
    int* node_count);
bool match_sub_err(size_t attr_domains_count,
    const struct betree_variable** preds,
    const struct betree_sub* sub,
    struct report_err* report,
    struct memoize* memoize,
    const uint64_t* undefined,
    betree_var_t* last_reason,
    struct attr_domain** attr_domains,
    betree_var_t* memoize_reason);

void add_sub_err(betree_sub_t id, struct report_err* report);

bool sub_is_enclosed_err(const struct attr_domain** attr_domains,
    const struct betree_sub* sub,
    const struct cdir_err* cdir);

struct lnode_err* make_lnode_err(const struct config* config, struct cnode_err* parent);
void free_lnode_err(struct lnode_err* lnode);
struct cnode_err* make_cnode_err(const struct config* config, struct cdir_err* parent);
void free_cnode_err(struct cnode_err* cnode);

struct betree_event* make_event_from_string_err(
    const struct betree_err* betree, const char* event_str);

struct betree_sub* find_sub_id_err(betree_sub_t id, struct cnode_err* cnode);

bool betree_search_with_preds_err(const struct config* config,
    const struct betree_variable** preds,
    const struct cnode_err* cnode,
    struct report_err* report);

bool betree_search_with_preds_ids_err(const struct config* config,
    const struct betree_variable** preds,
    const struct cnode_err* cnode,
    struct report_err* report,
    const uint64_t* ids,
    size_t sz);

bool insert_be_tree_err(const struct config* config,
    const struct betree_sub* sub,
    struct cnode_err* cnode,
    struct cdir_err* cdir);

void build_sub_ids_cdir(struct cdir_err* cd);
void build_sub_ids_cnode(struct cnode_err* cn);

void set_reason_sub_id_lists(struct report_err* report,
    betree_var_t reason,
    struct arraylist* sub_ids);
