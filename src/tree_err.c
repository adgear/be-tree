#include <ctype.h>
#include <float.h>
#include <inttypes.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "alloc.h"
#include "ast.h"
#include "betree.h"
#include "error.h"
#include "hashmap.h"
#include "memoize.h"
#include "printer.h"
#include "tree.h"
#include "utils.h"
#include "betree_err.h"
#include "tree_err.h"
#include "ast_err.h"

static const char* find_pnode_attr_name(struct cdir_err* cdir);

static bool is_id_in(uint64_t id, const uint64_t* ids, size_t sz) {
    if (sz == 0) {
        return false;
    }
    size_t first = 0;
    size_t last = sz - 1;
    if (id < ids[first] || id > ids[last]) {
        return false;
    }
    size_t middle = (first + last)/2;
    while (first <= last) {
        if (id == ids[middle]) {
            return true;
        }
        if (ids[middle] < id) {
            first = middle + 1;
        } else {
            last = middle - 1;
        }
        middle = (first + last)/2;
    }
    return false;
}

static void add_reason_sub_id_list(
    struct report_err* report,
    const char* variable_name,
    betree_sub_t sub_id
)
{
    if(variable_name == NULL)
        variable_name = "NULL";    
    if(hashtable_get(report->reason_sub_id_list, variable_name) == NULL)
    {
        arraylist* sub_id_list = arraylist_create();
        hashtable_set(report->reason_sub_id_list, variable_name, sub_id_list);
    }    
    arraylist_add(hashtable_get(report->reason_sub_id_list, variable_name), sub_id);
}

static void set_reason_sub_id_lists(
    struct report_err* report,
    const char* variable_name,
    struct arraylist* sub_ids
)
{
    if(!sub_ids || sub_ids->size <= 0) return;
    if(variable_name == NULL)
        variable_name = "NULL";
    if(hashtable_get(report->reason_sub_id_list, variable_name) == NULL)
    {
        arraylist* sub_id_list = arraylist_create();
        hashtable_set(report->reason_sub_id_list, variable_name, sub_id_list);
    }    
    if(sub_ids->size > 0)
        arraylist_join(hashtable_get(report->reason_sub_id_list, variable_name), sub_ids);
}

static void search_cdir_ids_err(const struct attr_domain** attr_domains,
    const struct betree_variable** preds,
    struct cdir_err* cdir,
    struct subs_to_eval* subs, bool open_left, bool open_right,
    const uint64_t* ids,
    size_t sz,
    struct report_err* report);

static void add_sub_to_eval(struct betree_sub* sub, struct subs_to_eval* subs)
{
    if(subs->capacity == subs->count) {
        subs->capacity *= 2;
        subs->subs = brealloc(subs->subs, sizeof(*subs->subs) * subs->capacity);
    }

    subs->subs[subs->count] = sub;
    subs->count++;
}

enum short_circuit_e { SHORT_CIRCUIT_PASS, SHORT_CIRCUIT_FAIL, SHORT_CIRCUIT_NONE };

static enum short_circuit_e try_short_circuit_err(size_t attr_domains_count,
    const struct short_circuit* short_circuit, const uint64_t* undefined, struct attr_domain** attr_domains,
    struct report_err* report, betree_sub_t sub_id)
{
    size_t count = attr_domains_count / 64 + 1;
    for(size_t i = 0; i < count; i++) {
        bool pass = short_circuit->pass[i] & undefined[i];
        if(pass) {
            return SHORT_CIRCUIT_PASS;
        }
        bool fail = short_circuit->fail[i] & undefined[i];
        if(fail) {
            for (size_t j=0; j < attr_domains_count; j++)
            {
                if((1<<(j%64) & short_circuit->fail[i]) && (1<<(j%64) & undefined[i])){
                    add_reason_sub_id_list(report, attr_domains[j]->attr_var.attr, sub_id);
                    break;
                }
            }
            return SHORT_CIRCUIT_FAIL;
        }
    }
    return SHORT_CIRCUIT_NONE;
}

bool match_sub_err(size_t attr_domains_count,
    const struct betree_variable** preds,
    const struct betree_sub* sub,
    struct report_err* report,
    struct memoize* memoize,
    const uint64_t* undefined,
    char* last_reason,
    struct attr_domain** attr_domains)
{
    enum short_circuit_e short_circuit = try_short_circuit_err(attr_domains_count, &sub->short_circuit, undefined, attr_domains, report, sub->id);
    if(short_circuit != SHORT_CIRCUIT_NONE) {
        if(report != NULL) {
            report->shorted++;
        }
        if(short_circuit == SHORT_CIRCUIT_PASS) {
            return true;
        }
        if(short_circuit == SHORT_CIRCUIT_FAIL) {
            return false;
        }
    }
    hashtable* memoize_table = hashtable_create();
    bool result = match_node_err(preds, sub->expr, memoize, report, last_reason, memoize_table);
    return result;
}

static void check_sub_err(const struct lnode_err* lnode, struct subs_to_eval* subs)
{
    for(size_t i = 0; i < lnode->sub_count; i++) {
        struct betree_sub* sub = lnode->subs[i];
        add_sub_to_eval(sub, subs);
    }
}

static void check_sub_ids_err(const struct lnode_err* lnode, struct subs_to_eval* subs,
    const uint64_t* ids, size_t sz)
{
    for(size_t i = 0; i < lnode->sub_count; i++) {
        struct betree_sub* sub = lnode->subs[i];
        if(is_id_in(sub->id, ids, sz)) {
            add_sub_to_eval(sub, subs);
        }
    }
}

static void check_sub_node_counting_err(const struct lnode_err* lnode, struct subs_to_eval* subs, int* node_count)
{
    for(size_t i = 0; i < lnode->sub_count; i++) {
        struct betree_sub* sub = lnode->subs[i];
        add_sub_to_eval(sub, subs);
        ++*node_count;
    }
}

static struct pnode_err* search_pdir_err(betree_var_t variable_id, const struct pdir_err* pdir)
{
    if(pdir == NULL) {
        return NULL;
    }
    for(size_t i = 0; i < pdir->pnode_count; i++) {
        struct pnode_err* pnode = pdir->pnodes[i];
        if(variable_id == pnode->attr_var.var) {
            return pnode;
        }
    }
    return NULL;
}

static void search_cdir_err(const struct attr_domain** attr_domains,
    const struct betree_variable** preds,
    struct cdir_err* cdir,
    struct subs_to_eval* subs, bool open_left, bool open_right,
    struct report_err* report);

static void search_cdir_node_counting_err(const struct attr_domain** attr_domains,
    const struct betree_variable** preds,
    struct cdir_err* cdir,
    struct subs_to_eval* subs, bool open_left, bool open_right, int* node_count);

static bool event_contains_variable(const struct betree_variable** preds, betree_var_t variable_id)
{
    return preds[variable_id] != NULL;
}

void match_be_tree_err(const struct attr_domain** attr_domains,
    const struct betree_variable** preds,
    const struct cnode_err* cnode,
    struct subs_to_eval* subs,
    struct report_err* report)
{
    check_sub_err(cnode->lnode, subs);
    if(cnode->pdir != NULL) {
        for(size_t i = 0; i < cnode->pdir->pnode_count; i++) {
            struct pnode_err* pnode = cnode->pdir->pnodes[i];
            const struct attr_domain* attr_domain
                = get_attr_domain(attr_domains, pnode->attr_var.var);
            if(attr_domain->allow_undefined
                || event_contains_variable(preds, pnode->attr_var.var)) {
                search_cdir_err(attr_domains, preds, pnode->cdir, subs, true, true, report);
            }
        }
    }
}

static void match_be_tree_ids_err(const struct attr_domain** attr_domains,
    const struct betree_variable** preds,
    const struct cnode_err* cnode,
    struct subs_to_eval* subs,
    const uint64_t* ids,
    size_t sz,
    struct report_err* report)
{
    check_sub_ids_err(cnode->lnode, subs, ids, sz);
    if(cnode->pdir != NULL) {
        for(size_t i = 0; i < cnode->pdir->pnode_count; i++) {
            struct pnode_err* pnode = cnode->pdir->pnodes[i];
            const struct attr_domain* attr_domain
                = get_attr_domain(attr_domains, pnode->attr_var.var);
            if(attr_domain->allow_undefined
                || event_contains_variable(preds, pnode->attr_var.var)) {
                search_cdir_ids_err(attr_domains, preds, pnode->cdir, subs, true, true, ids, sz, report);
            }
        }
    }
}

void match_be_tree_node_counting_err(const struct attr_domain** attr_domains,
    const struct betree_variable** preds,
    const struct cnode_err* cnode,
    struct subs_to_eval* subs, int* node_count)
{
    check_sub_node_counting_err(cnode->lnode, subs, node_count);
    if(cnode->pdir != NULL) {
        for(size_t i = 0; i < cnode->pdir->pnode_count; i++) {
            struct pnode_err* pnode = cnode->pdir->pnodes[i];
            const struct attr_domain* attr_domain
                = get_attr_domain(attr_domains, pnode->attr_var.var);
            if(attr_domain->allow_undefined
                || event_contains_variable(preds, pnode->attr_var.var)) {
                search_cdir_node_counting_err(attr_domains, preds, pnode->cdir, subs, true, true, node_count);
            }
            ++*node_count;
        }
        ++*node_count;
    }
}

static bool is_event_enclosed_err(const struct betree_variable** preds, const struct cdir_err* cdir, bool open_left, bool open_right)
{
    if(cdir == NULL) {
        return false;
    }
    const struct betree_variable* pred = preds[cdir->attr_var.var];
    if(pred == NULL) {
        return true;
    }
    // No open_left for smin because it's always 0
    switch(pred->value.value_type) {
        case BETREE_BOOLEAN:
            return (cdir->bound.bmin <= pred->value.boolean_value) && (cdir->bound.bmax >= pred->value.boolean_value);
        case BETREE_INTEGER:
            return (open_left || cdir->bound.imin <= pred->value.integer_value) && (open_right || cdir->bound.imax >= pred->value.integer_value);
        case BETREE_FLOAT:
            return (open_left || cdir->bound.fmin <= pred->value.float_value) && (open_right || cdir->bound.fmax >= pred->value.float_value);
        case BETREE_STRING:
            return (cdir->bound.smin <= pred->value.string_value.str) && (open_right || cdir->bound.smax >= pred->value.string_value.str);
        case BETREE_INTEGER_ENUM:
            return (cdir->bound.smin <= pred->value.integer_enum_value.ienum) && (open_right || cdir->bound.smax >= pred->value.integer_enum_value.ienum);
        case BETREE_INTEGER_LIST:
            if(pred->value.integer_list_value->count != 0) {
                int64_t min = pred->value.integer_list_value->integers[0];
                int64_t max = pred->value.integer_list_value->integers[pred->value.integer_list_value->count - 1];
                int64_t bound_min = open_left ? INT64_MIN : cdir->bound.imin;
                int64_t bound_max = open_right ? INT64_MAX : cdir->bound.imax;
                return min <= bound_max && bound_min <= max;
            }
            else {
                return true;
            }
        case BETREE_STRING_LIST:
            if(pred->value.string_list_value->count != 0) {
                size_t min = pred->value.string_list_value->strings[0].str;
                size_t max = pred->value.string_list_value->strings[pred->value.string_list_value->count - 1].str;
                size_t bound_min = cdir->bound.smin;
                size_t bound_max = open_right ? SIZE_MAX : cdir->bound.smax;
                return min <= bound_max && bound_min <= max;
            }
            else {
                return true;
            }
        case BETREE_SEGMENTS:
        case BETREE_FREQUENCY_CAPS:
            return true;
        default: abort();
    }
    return false;
}

bool sub_is_enclosed_err(const struct attr_domain** attr_domains, const struct betree_sub* sub, const struct cdir_err* cdir)
{
    if(cdir == NULL) {
        return false;
    }
    if(test_bit(sub->attr_vars, cdir->attr_var.var) == true) {
        const struct attr_domain* attr_domain = get_attr_domain(attr_domains, cdir->attr_var.var);
        struct value_bound bound = get_variable_bound(attr_domain, sub->expr);
        switch(attr_domain->bound.value_type) {
            case(BETREE_INTEGER):
            case(BETREE_INTEGER_LIST):
                return cdir->bound.imin <= bound.imin && cdir->bound.imax >= bound.imax;
            case(BETREE_FLOAT): {
                return cdir->bound.fmin <= bound.fmin && cdir->bound.fmax >= bound.fmax;
            }
            case(BETREE_BOOLEAN): {
                return cdir->bound.bmin <= bound.bmin && cdir->bound.bmax >= bound.bmax;
            }
            case(BETREE_STRING):
            case(BETREE_STRING_LIST):
            case(BETREE_INTEGER_ENUM):
                return cdir->bound.smin <= bound.smin && cdir->bound.smax >= bound.smax;
            case(BETREE_SEGMENTS): {
                fprintf(
                    stderr, "%s a segments value cdir should never happen for now\n", __func__);
                abort();
            }
            case(BETREE_FREQUENCY_CAPS): {
                fprintf(stderr,
                    "%s a frequency value cdir should never happen for now\n",
                    __func__);
                abort();
            }
            default: abort();
        }
    }
    return false;
}

static void search_cdir_err(const struct attr_domain** attr_domains,
    const struct betree_variable** preds,
    struct cdir_err* cdir,
    struct subs_to_eval* subs,
    bool open_left,
    bool open_right,
    struct report_err* report)
{
    match_be_tree_err(attr_domains, preds, cdir->cnode, subs, report);
    if(is_event_enclosed_err(preds, cdir->lchild, open_left, false)) {
        search_cdir_err(attr_domains, preds, cdir->lchild, subs, open_left, false, report);
    }
    else {
        if(cdir->lchild && cdir->lchild->sub_ids->size > 0) {
            const char* attr_name = find_pnode_attr_name(cdir);
            if(attr_name != NULL)
                set_reason_sub_id_lists(report, attr_name, cdir->lchild->sub_ids);
            else
                set_reason_sub_id_lists(report, "no_reason", cdir->lchild->sub_ids);
        }
    }
    if(is_event_enclosed_err(preds, cdir->rchild, false, open_right)) {
        search_cdir_err(attr_domains, preds, cdir->rchild, subs, false, open_right, report);
    }
    else {
        if(cdir->rchild && cdir->rchild->sub_ids->size > 0) {
            const char* attr_name = find_pnode_attr_name(cdir);
            if(attr_name != NULL)
                set_reason_sub_id_lists(report, attr_name, cdir->lchild->sub_ids);
            else
                set_reason_sub_id_lists(report, "no_reason", cdir->lchild->sub_ids);
        }
    }
}

static void search_cdir_ids_err(const struct attr_domain** attr_domains,
    const struct betree_variable** preds,
    struct cdir_err* cdir,
    struct subs_to_eval* subs,
    bool open_left,
    bool open_right,
    const uint64_t* ids,
    size_t sz,
    struct report_err* report)
{
    match_be_tree_ids_err(attr_domains, preds, cdir->cnode, subs, ids, sz, report);
    if(is_event_enclosed_err(preds, cdir->lchild, open_left, false)) {
        search_cdir_ids_err(attr_domains, preds, cdir->lchild, subs, open_left, false, ids, sz, report);
    }
    else {
        if(cdir->lchild && cdir->lchild->sub_ids->size > 0) {
            const char* attr_name = find_pnode_attr_name(cdir);
            if(attr_name != NULL)
                set_reason_sub_id_lists(report, attr_name, cdir->lchild->sub_ids);
            else
                set_reason_sub_id_lists(report, "no_reason", cdir->lchild->sub_ids);
        }
    }
    if(is_event_enclosed_err(preds, cdir->rchild, false, open_right)) {
        search_cdir_ids_err(attr_domains, preds, cdir->rchild, subs, false, open_right, ids, sz, report);
    }
    else {
        if(cdir->rchild && cdir->rchild->sub_ids->size > 0) {
            const char* attr_name = find_pnode_attr_name(cdir);
            if(attr_name != NULL)
                set_reason_sub_id_lists(report, attr_name, cdir->lchild->sub_ids);
            else
                set_reason_sub_id_lists(report, "no_reason", cdir->lchild->sub_ids);
        }
    }
}

static void search_cdir_node_counting_err(const struct attr_domain** attr_domains,
    const struct betree_variable** preds,
    struct cdir_err* cdir,
    struct subs_to_eval* subs, bool open_left, bool open_right, int* node_count)
{
    match_be_tree_node_counting_err(attr_domains, preds, cdir->cnode, subs, node_count);
    if(is_event_enclosed_err(preds, cdir->lchild, open_left, false)) {
        search_cdir_node_counting_err(attr_domains, preds, cdir->lchild, subs, open_left, false, node_count);
    }
    if(is_event_enclosed_err(preds, cdir->rchild, false, open_right)) {
        search_cdir_node_counting_err(attr_domains, preds, cdir->rchild, subs, false, open_right, node_count);
    }
}

static bool is_used_cnode_err(betree_var_t variable_id, const struct cnode_err* cnode);

static bool is_used_pdir_err(betree_var_t variable_id, const struct pdir_err* pdir)
{
    if(pdir == NULL || pdir->parent == NULL) {
        return false;
    }
    return is_used_cnode_err(variable_id, pdir->parent);
}

static bool is_used_pnode_err(betree_var_t variable_id, const struct pnode_err* pnode)
{
    if(pnode == NULL || pnode->parent == NULL) {
        return false;
    }
    if(pnode->attr_var.var == variable_id) {
        return true;
    }
    return is_used_pdir_err(variable_id, pnode->parent);
}

static bool is_used_cdir_err(betree_var_t variable_id, const struct cdir_err* cdir)
{
    if(cdir == NULL) {
        return false;
    }
    if(cdir->attr_var.var == variable_id) {
        return true;
    }
    switch(cdir->parent_type) {
        case CNODE_PARENT_PNODE: {
            return is_used_pnode_err(variable_id, cdir->pnode_parent);
        }
        case CNODE_PARENT_CDIR: {
            return is_used_cdir_err(variable_id, cdir->cdir_parent);
        }
        default: abort();
    }
}

static bool is_used_cnode_err(betree_var_t variable_id, const struct cnode_err* cnode)
{
    if(cnode == NULL) {
        return false;
    }
    if(cnode->parent == NULL) {
        return false;
    }
    return is_used_cdir_err(variable_id, cnode->parent);
}

static void insert_sub_err(const struct betree_sub* sub, struct lnode_err* lnode)
{
    if(lnode->sub_count == 0) {
        lnode->subs = bcalloc(sizeof(*lnode->subs));
        if(lnode->subs == NULL) {
            fprintf(stderr, "%s bcalloc failed\n", __func__);
            abort();
        }
    }
    else {
        struct betree_sub** subs = brealloc(lnode->subs, sizeof(*subs) * (lnode->sub_count + 1));
        if(subs == NULL) {
            fprintf(stderr, "%s brealloc failed\n", __func__);
            abort();
        }
        lnode->subs = subs;
    }
    lnode->subs[lnode->sub_count] = (struct betree_sub*)sub;
    lnode->sub_count++;
}

static bool is_root_err(const struct cnode_err* cnode)
{
    if(cnode == NULL) {
        return false;
    }
    return cnode->parent == NULL;
}

static void space_partitioning_err(const struct config* config, struct cnode_err* cnode);
static void space_clustering_err(const struct config* config, struct cdir_err* cdir);
static struct cdir_err* insert_cdir_err(
    const struct config* config, const struct betree_sub* sub, struct cdir_err* cdir);

static size_t count_attr_in_lnode_err(betree_var_t variable_id, const struct lnode_err* lnode);

static size_t count_attr_in_cdir_err(betree_var_t variable_id, const struct cdir_err* cdir)
{
    if(cdir == NULL) {
        return 0;
    }
    size_t count = 0;
    if(cdir->cnode != NULL) {
        count += count_attr_in_lnode_err(variable_id, cdir->cnode->lnode);
    }
    count += count_attr_in_cdir_err(variable_id, cdir->lchild);
    count += count_attr_in_cdir_err(variable_id, cdir->rchild);
    return count;
}

static size_t domain_bound_diff(const struct attr_domain* attr_domain)
{
    const struct value_bound* b = &attr_domain->bound;
    switch(b->value_type) {
        case BETREE_BOOLEAN:
            return 1;
        case BETREE_INTEGER:
        case BETREE_INTEGER_LIST:
            if(b->imin == INT64_MIN && b->imax == INT64_MAX) {
                return SIZE_MAX;
            }
            else {
                return (size_t)(llabs(b->imax - b->imin));
            }
        case BETREE_FLOAT:
            if(feq(b->fmin, -DBL_MAX) && feq(b->fmax, DBL_MAX)) {
                return SIZE_MAX;
            }
            else {
                return (size_t)(fabs(b->fmax - b->fmin));
            }
        case BETREE_STRING:
        case BETREE_STRING_LIST:
        case BETREE_INTEGER_ENUM:
            return b->smax - b->smin;
        case BETREE_SEGMENTS:
        case BETREE_FREQUENCY_CAPS:
        default:
            abort();
    }
}

static double get_attr_domain_score(const struct attr_domain* attr_domain)
{
    size_t diff = domain_bound_diff(attr_domain);
    if(diff == 0) {
        diff = 1;
    }
    double num = attr_domain->allow_undefined ? 1. : 10.;
    double bound_score = num / (double)diff;
    return bound_score;
}

static double get_score(const struct attr_domain** attr_domains, betree_var_t var, size_t count)
{
    const struct attr_domain* attr_domain = get_attr_domain(attr_domains, var);
    double attr_domain_score = get_attr_domain_score(attr_domain);
    double score = (double)count * attr_domain_score;
    return score;
}

static double get_pnode_score_err(const struct attr_domain** attr_domains, struct pnode_err* pnode)
{
    size_t count = count_attr_in_cdir_err(pnode->attr_var.var, pnode->cdir);
    return get_score(attr_domains, pnode->attr_var.var, count);
}

static double get_lnode_score_err(
    const struct attr_domain** attr_domains, const struct lnode_err* lnode, betree_var_t var)
{
    size_t count = count_attr_in_lnode_err(var, lnode);
    return get_score(attr_domains, var, count);
}

static void update_partition_score_err(const struct attr_domain** attr_domains, struct pnode_err* pnode)
{
    pnode->score = get_pnode_score_err(attr_domains, pnode);
}

bool insert_be_tree_err(
    const struct config* config, const struct betree_sub* sub, struct cnode_err* cnode, struct cdir_err* cdir)
{
    if(config == NULL) {
        fprintf(stderr, "Config is NULL, required to insert in the be tree\n");
        abort();
    }
    bool foundPartition = false;
    struct pnode_err* max_pnode = NULL;
    if(cnode->pdir != NULL) {
        float max_score = -DBL_MAX;
        for(size_t i = 0; i < config->attr_domain_count; i++) {
            if(test_bit(sub->attr_vars, i) == false) {
                continue;
            }
            betree_var_t variable_id = i;
            if(!is_used_cnode_err(variable_id, cnode)) {
                struct pnode_err* pnode = search_pdir_err(variable_id, cnode->pdir);
                if(pnode != NULL) {
                    foundPartition = true;
                    if(max_score < pnode->score) {
                        max_pnode = pnode;
                        max_score = pnode->score;
                    }
                }
            }
        }
    }
    if(!foundPartition) {
        insert_sub_err(sub, cnode->lnode);
        if(is_root_err(cnode)) {
            space_partitioning_err(config, cnode);
        }
        else {
            space_clustering_err(config, cdir);
        }
    }
    else {
        struct cdir_err* maxCdir = insert_cdir_err(config, sub, max_pnode->cdir);
        insert_be_tree_err(config, sub, maxCdir->cnode, maxCdir);
        update_partition_score_err((const struct attr_domain**)config->attr_domains, max_pnode);
    }
    return true;
}

static bool is_leaf_err(const struct cdir_err* cdir)
{
    return cdir->lchild == NULL && cdir->rchild == NULL;
}

static struct cdir_err* insert_cdir_err(
    const struct config* config, const struct betree_sub* sub, struct cdir_err* cdir)
{
    if(is_leaf_err(cdir)) {
        return cdir;
    }
    if(sub_is_enclosed_err((const struct attr_domain**)config->attr_domains, sub, cdir->lchild)) {
        return insert_cdir_err(config, sub, cdir->lchild);
    }
    if(sub_is_enclosed_err((const struct attr_domain**)config->attr_domains, sub, cdir->rchild)) {
        return insert_cdir_err(config, sub, cdir->rchild);
    }
    return cdir;
}

static bool is_overflowed_err(const struct lnode_err* lnode)
{
    return lnode->sub_count > lnode->max;
}

static bool remove_sub_err(betree_sub_t sub, struct lnode_err* lnode)
{
    for(size_t i = 0; i < lnode->sub_count; i++) {
        const struct betree_sub* lnode_sub = lnode->subs[i];
        if(sub == lnode_sub->id) {
            for(size_t j = i; j < lnode->sub_count - 1; j++) {
                lnode->subs[j] = lnode->subs[j + 1];
            }
            lnode->sub_count--;
            if(lnode->sub_count == 0) {
                bfree(lnode->subs);
                lnode->subs = NULL;
            }
            else {
                struct betree_sub** subs = brealloc(lnode->subs, sizeof(*lnode->subs) * lnode->sub_count);
                if(subs == NULL) {
                    fprintf(stderr, "%s brealloc failed\n", __func__);
                    abort();
                }
                lnode->subs = subs;
            }
            return true;
        }
    }
    return false;
}

static void move_err(const struct betree_sub* sub, struct lnode_err* origin, struct lnode_err* destination)
{
    bool isFound = remove_sub_err(sub->id, origin);
    if(!isFound) {
        fprintf(stderr, "Could not find sub %" PRIu64 "\n", sub->id);
        abort();
    }
    if(destination->sub_count == 0) {
        destination->subs = bcalloc(sizeof(*destination->subs));
        if(destination->subs == NULL) {
            fprintf(stderr, "%s bcalloc failed\n", __func__);
            abort();
        }
    }
    else {
        struct betree_sub** subs
            = brealloc(destination->subs, sizeof(*destination->subs) * (destination->sub_count + 1));
        if(subs == NULL) {
            fprintf(stderr, "%s brealloc failed\n", __func__);
            abort();
        }
        destination->subs = subs;
    }
    destination->subs[destination->sub_count] = (struct betree_sub*)sub;
    destination->sub_count++;
}

static struct cdir_err* create_cdir_err(const struct config* config,
    const char* attr,
    betree_var_t variable_id,
    struct value_bound bound)
{
    struct cdir_err* cdir = bcalloc(sizeof(*cdir));
    if(cdir == NULL) {
        fprintf(stderr, "%s bcalloc failed\n", __func__);
        abort();
    }
    cdir->attr_var.attr = bstrdup(attr);
    cdir->attr_var.var = variable_id;
    cdir->bound = bound;
    cdir->cnode = make_cnode(config, cdir);
    cdir->lchild = NULL;
    cdir->rchild = NULL;
    cdir->sub_ids = arraylist_create();
    return cdir;
}

static struct cdir_err* create_cdir_with_cdir_parent_err(
    const struct config* config, struct cdir_err* parent, struct value_bound bound)
{
    struct cdir_err* cdir = create_cdir_err(config, parent->attr_var.attr, parent->attr_var.var, bound);
    cdir->parent_type = CNODE_PARENT_CDIR;
    cdir->cdir_parent = parent;
    return cdir;
}

static struct cdir_err* create_cdir_with_pnode_parent_err(
    const struct config* config, struct pnode_err* parent, struct value_bound bound)
{
    struct cdir_err* cdir = create_cdir_err(config, parent->attr_var.attr, parent->attr_var.var, bound);
    cdir->parent_type = CNODE_PARENT_PNODE;
    cdir->pnode_parent = parent;
    return cdir;
}

struct pnode_err* create_pdir_err(
    const struct config* config, const char* attr, betree_var_t variable_id, struct cnode_err* cnode)
{
    if(cnode == NULL) {
        fprintf(stderr, "cnode is NULL, cannot create a pdir and pnode\n");
        abort();
    }
    struct pdir_err* pdir = cnode->pdir;
    if(cnode->pdir == NULL) {
        pdir = bcalloc(sizeof(*pdir));
        if(pdir == NULL) {
            fprintf(stderr, "%s pdir bcalloc failed\n", __func__);
            abort();
        }
        pdir->parent = cnode;
        pdir->pnode_count = 0;
        pdir->pnodes = NULL;
        cnode->pdir = pdir;
    }

    struct pnode_err* pnode = bcalloc(sizeof(*pnode));
    if(pnode == NULL) {
        fprintf(stderr, "%s pnode bcalloc failed\n", __func__);
        abort();
    }
    pnode->cdir = NULL;
    pnode->parent = pdir;
    pnode->attr_var.attr = bstrdup(attr);
    pnode->attr_var.var = variable_id;
    pnode->score = 0.f;
    struct value_bound bound;
    bool found = false;
    for(size_t i = 0; i < config->attr_domain_count; i++) {
        const struct attr_domain* attr_domain = config->attr_domains[i];
        if(attr_domain->attr_var.var == variable_id) {
            bound = attr_domain->bound;
            found = true;
            break;
        }
    }
    if(!found) {
        fprintf(stderr, "No domain definition for attr %" PRIu64 " in config\n", variable_id);
        abort();
    }
    pnode->cdir = create_cdir_with_pnode_parent_err(config, pnode, bound);

    if(pdir->pnode_count == 0) {
        pdir->pnodes = bcalloc(sizeof(*pdir->pnodes));
        if(pdir->pnodes == NULL) {
            fprintf(stderr, "%s pnodes bcalloc failed\n", __func__);
            abort();
        }
    }
    else {
        struct pnode_err** pnodes = brealloc(pdir->pnodes, sizeof(*pnodes) * (pdir->pnode_count + 1));
        if(pnodes == NULL) {
            fprintf(stderr, "%s brealloc failed\n", __func__);
            abort();
        }
        pdir->pnodes = pnodes;
    }
    pdir->pnodes[pdir->pnode_count] = pnode;
    pdir->pnode_count++;
    return pnode;
}

static size_t count_attr_in_lnode_err(betree_var_t variable_id, const struct lnode_err* lnode)
{
    size_t count = 0;
    if(lnode == NULL) {
        return count;
    }
    for(size_t i = 0; i < lnode->sub_count; i++) {
        const struct betree_sub* sub = lnode->subs[i];
        if(sub == NULL) {
            fprintf(stderr, "%s, sub is NULL\n", __func__);
            continue;
        }
        if(test_bit(sub->attr_vars, variable_id) == true) {
            count++;
        }
    }
    return count;
}

static bool is_attr_used_in_parent_cnode_err(betree_var_t variable_id, const struct cnode_err* cnode);

static bool is_attr_used_in_parent_pdir_err(betree_var_t variable_id, const struct pdir_err* pdir)
{
    return is_attr_used_in_parent_cnode_err(variable_id, pdir->parent);
}

static bool is_attr_used_in_parent_pnode_err(betree_var_t variable_id, const struct pnode_err* pnode)
{
    if(pnode->attr_var.var == variable_id) {
        return true;
    }
    return is_attr_used_in_parent_pdir_err(variable_id, pnode->parent);
}

static bool is_attr_used_in_parent_cdir_err(betree_var_t variable_id, const struct cdir_err* cdir)
{
    if(cdir->attr_var.var == variable_id) {
        return true;
    }
    switch(cdir->parent_type) {
        case(CNODE_PARENT_CDIR): {
            return is_attr_used_in_parent_cdir_err(variable_id, cdir->cdir_parent);
        }
        case(CNODE_PARENT_PNODE): {
            return is_attr_used_in_parent_pnode_err(variable_id, cdir->pnode_parent);
        }
        default: abort();
    }
}

static bool is_attr_used_in_parent_cnode_err(betree_var_t variable_id, const struct cnode_err* cnode)
{
    if(is_root_err(cnode)) {
        return false;
    }
    return is_attr_used_in_parent_cdir_err(variable_id, cnode->parent);
}

static bool is_attr_used_in_parent_lnode_err(betree_var_t variable_id, const struct lnode_err* lnode)
{
    return is_attr_used_in_parent_cnode_err(variable_id, lnode->parent);
}

static bool splitable_attr_domain(
    const struct config* config, const struct attr_domain* attr_domain)
{
    switch(attr_domain->bound.value_type) {
        case BETREE_INTEGER:
        case BETREE_INTEGER_LIST:
            if(attr_domain->bound.imin == INT64_MIN || attr_domain->bound.imax == INT64_MAX) {
                return false;
            }
            return ((uint64_t)llabs(attr_domain->bound.imax - attr_domain->bound.imin))
                < config->max_domain_for_split;
        case BETREE_FLOAT:
            if(feq(attr_domain->bound.fmin, -DBL_MAX) || feq(attr_domain->bound.fmax, DBL_MAX)) {
                return false;
            }
            return ((uint64_t)fabs(attr_domain->bound.fmax - attr_domain->bound.fmin))
                < config->max_domain_for_split;
        case BETREE_BOOLEAN:
            return true;
        case BETREE_STRING:
        case BETREE_STRING_LIST:
        case BETREE_INTEGER_ENUM:
            if(attr_domain->bound.smax == SIZE_MAX) {
                return false;
            }
            return (attr_domain->bound.smax - attr_domain->bound.smin)
                < config->max_domain_for_split;
        case BETREE_SEGMENTS:
        case BETREE_FREQUENCY_CAPS:
            return false;
        default: abort();
    }
}

static bool get_next_highest_score_unused_attr_err(
    const struct config* config, const struct lnode_err* lnode, betree_var_t* var)
{
    bool found = false;
    double highest_score = 0;
    betree_var_t highest_var;
    for(size_t i = 0; i < lnode->sub_count; i++) {
        const struct betree_sub* sub = lnode->subs[i];
        for(size_t j = 0; j < config->attr_domain_count; j++) {
            if(test_bit(sub->attr_vars, j) == false) {
                continue;
            }
            betree_var_t current_variable_id = j;
            const struct attr_domain* attr_domain = get_attr_domain(
                (const struct attr_domain**)config->attr_domains, current_variable_id);
            if(splitable_attr_domain(config, attr_domain)
                && !is_attr_used_in_parent_lnode_err(current_variable_id, lnode)) {
                double current_score = get_lnode_score_err(
                    (const struct attr_domain**)config->attr_domains, lnode, current_variable_id);
                found = true;
                if(current_score > highest_score) {
                    highest_score = current_score;
                    highest_var = current_variable_id;
                }
            }
        }
    }
    if(found == false) {
        return false;
    }
    *var = highest_var;
    return true;
}

static void update_cluster_capacity_err(const struct config* config, struct lnode_err* lnode)
{
    if(lnode == NULL) {
        return;
    }
    size_t count = lnode->sub_count;
    size_t max = smax(config->lnode_max_cap,
        ceil((double)count / (double)config->lnode_max_cap) * config->lnode_max_cap);
    lnode->max = max;
}

static size_t count_subs_with_variable(
    const struct betree_sub** subs, size_t sub_count, betree_var_t variable_id)
{
    size_t count = 0;
    for(size_t i = 0; i < sub_count; i++) {
        const struct betree_sub* sub = subs[i];
        if(sub_has_attribute(sub, variable_id)) {
            count++;
        }
    }
    return count;
}

static void space_partitioning_err(const struct config* config, struct cnode_err* cnode)
{
    struct lnode_err* lnode = cnode->lnode;
    while(is_overflowed_err(lnode) == true) {
        betree_var_t var;
        bool found = get_next_highest_score_unused_attr_err(config, lnode, &var);
        if(found == false) {
            break;
        }
        size_t target_subs_count = count_subs_with_variable(
            (const struct betree_sub**)lnode->subs, lnode->sub_count, var);
        if(target_subs_count < config->partition_min_size) {
            break;
        }
        const char* attr = config->attr_domains[var]->attr_var.attr;
        struct pnode_err* pnode = create_pdir_err(config, attr, var, cnode);
        for(size_t i = 0; i < lnode->sub_count; i++) {
            const struct betree_sub* sub = lnode->subs[i];
            if(sub_has_attribute(sub, var)) {
                struct cdir_err* cdir = insert_cdir_err(config, sub, pnode->cdir);
                move_err(sub, lnode, cdir->cnode->lnode);
                i--;
            }
        }
        space_clustering_err(config, pnode->cdir);
    }
    update_cluster_capacity_err(config, lnode);
}

static bool is_atomic_err(const struct cdir_err* cdir)
{
    switch(cdir->bound.value_type) {
        case(BETREE_INTEGER):
        case(BETREE_INTEGER_LIST):
            return cdir->bound.imin == cdir->bound.imax;
        case(BETREE_FLOAT): {
            return feq(cdir->bound.fmin, cdir->bound.fmax);
        }
        case(BETREE_BOOLEAN): {
            return cdir->bound.bmin == cdir->bound.bmax;
        }
        case(BETREE_STRING):
        case(BETREE_STRING_LIST):
        case(BETREE_INTEGER_ENUM):
            return cdir->bound.smin == cdir->bound.smax;
        case(BETREE_SEGMENTS): {
            fprintf(stderr, "%s a segments value cdir should never happen for now\n", __func__);
            abort();
        }
        case(BETREE_FREQUENCY_CAPS): {
            fprintf(stderr, "%s a frequency value cdir should never happen for now\n", __func__);
            abort();
        }
        default: abort();
    }
}

struct lnode_err* make_lnode_err(const struct config* config, struct cnode_err* parent)
{
    struct lnode_err* lnode = bcalloc(sizeof(*lnode));
    if(lnode == NULL) {
        fprintf(stderr, "%s bcalloc failed\n", __func__);
        abort();
    }
    lnode->parent = parent;
    lnode->sub_count = 0;
    lnode->subs = NULL;
    lnode->max = config->lnode_max_cap;
    return lnode;
}

struct cnode_err* make_cnode_err(const struct config* config, struct cdir_err* parent)
{
    struct cnode_err* cnode = bcalloc(sizeof(*cnode));
    if(cnode == NULL) {
        fprintf(stderr, "%s bcalloc failed\n", __func__);
        abort();
    }
    cnode->parent = parent;
    cnode->pdir = NULL;
    cnode->lnode = make_lnode(config, cnode);
    return cnode;
}

struct value_bounds {
    struct value_bound lbound;
    struct value_bound rbound;
};

static struct value_bounds split_value_bound(struct value_bound bound)
{
    struct value_bound lbound = { .value_type = bound.value_type };
    struct value_bound rbound = { .value_type = bound.value_type };
    switch(bound.value_type) {
        case(BETREE_INTEGER):
        case(BETREE_INTEGER_LIST): {
            int64_t start = bound.imin, end = bound.imax;
            lbound.imin = start;
            rbound.imax = end;
            if(llabs(end - start) > 2) {
                int64_t middle = start + (end - start) / 2;
                lbound.imax = middle;
                rbound.imin = middle;
            }
            else if(llabs(end - start) == 2) {
                int64_t middle = start + 1;
                lbound.imax = middle;
                rbound.imin = middle;
            }
            else if(llabs(end - start) == 1) {
                lbound.imax = start;
                rbound.imin = end;
            }
            else {
                fprintf(stderr, "%s trying to split an unsplitable bound\n", __func__);
                abort();
            }
            break;
        }
        case(BETREE_FLOAT): {
            double start = bound.fmin, end = bound.fmax;
            lbound.fmin = start;
            rbound.fmax = end;
            if(fabs(end - start) > 2) {
                double middle = start + ceil((end - start) / 2);
                lbound.fmax = middle;
                rbound.fmin = middle;
            }
            else if(feq(fabs(end - start), 2)) {
                double middle = start + 1;
                lbound.fmax = middle;
                rbound.fmin = middle;
            }
            else if(feq(fabs(end - start), 1)) {
                lbound.fmax = start;
                rbound.fmin = end;
            }
            else {
                fprintf(stderr, "%s trying to split an unsplitable bound\n", __func__);
                abort();
            }
            break;
        }
        case(BETREE_BOOLEAN): {
            bool start = bound.bmin, end = bound.bmax;
            lbound.bmin = start;
            rbound.bmax = end;
            if(abs(end - start) == 1) {
                lbound.bmax = start;
                rbound.bmin = end;
            }
            else {
                fprintf(stderr, "%s trying to split an unsplitable bound\n", __func__);
                abort();
            }
            break;
        }
        case(BETREE_STRING):
        case(BETREE_STRING_LIST): 
        case(BETREE_INTEGER_ENUM): {
            size_t start = bound.smin, end = bound.smax;
            lbound.smin = start;
            rbound.smax = end;
            if(end - start > 2) {
                size_t middle = start + (end - start) / 2;
                lbound.smax = middle;
                rbound.smin = middle;
            }
            else if(end - start == 2) {
                int64_t middle = start + 1;
                lbound.smax = middle;
                rbound.smin = middle;
            }
            else if(end - start == 1) {
                lbound.smax = start;
                rbound.smin = end;
            }
            else {
                fprintf(stderr, "%s trying to split an unsplitable bound\n", __func__);
                abort();
            }
            break;
        }
        case(BETREE_SEGMENTS): {
            fprintf(stderr, "%s a segment value cdir should never happen for now\n", __func__);
            abort();
        }
        case(BETREE_FREQUENCY_CAPS): {
            fprintf(stderr, "%s a frequency value cdir should never happen for now\n", __func__);
            abort();
        }
        default: abort();
    }
    struct value_bounds bounds = { .lbound = lbound, .rbound = rbound };
    return bounds;
}

static void space_clustering_err(const struct config* config, struct cdir_err* cdir)
{
    if(cdir == NULL || cdir->cnode == NULL) {
        return;
    }
    struct lnode_err* lnode = cdir->cnode->lnode;
    if(!is_overflowed_err(lnode)) {
        return;
    }
    if(!is_leaf_err(cdir) || is_atomic_err(cdir)) {
        space_partitioning_err(config, cdir->cnode);
    }
    else {
        struct value_bounds bounds = split_value_bound(cdir->bound);
        cdir->lchild = create_cdir_with_cdir_parent_err(config, cdir, bounds.lbound);
        cdir->rchild = create_cdir_with_cdir_parent_err(config, cdir, bounds.rbound);
        for(size_t i = 0; i < lnode->sub_count; i++) {
            const struct betree_sub* sub = lnode->subs[i];
            if(sub_is_enclosed_err((const struct attr_domain**)config->attr_domains, sub, cdir->lchild)) {
                move_err(sub, lnode, cdir->lchild->cnode->lnode);
                i--;
            }
            else if(sub_is_enclosed_err((const struct attr_domain**)config->attr_domains, sub, cdir->rchild)) {
                move_err(sub, lnode, cdir->rchild->cnode->lnode);
                i--;
            }
        }
        space_partitioning_err(config, cdir->cnode);
        space_clustering_err(config, cdir->lchild);
        space_clustering_err(config, cdir->rchild);
    }
    update_cluster_capacity_err(config, lnode);
}

static void free_pnode_err(struct pnode_err* pnode);

static void free_pdir_err(struct pdir_err* pdir)
{
    if(pdir == NULL) {
        return;
    }
    for(size_t i = 0; i < pdir->pnode_count; i++) {
        struct pnode_err* pnode = pdir->pnodes[i];
        free_pnode_err(pnode);
    }
    bfree(pdir->pnodes);
    pdir->pnodes = NULL;
    bfree(pdir);
}

static void free_pred(struct betree_variable* pred)
{
    if(pred == NULL) {
        return;
    }
    bfree((char*)pred->attr_var.attr);
    free_value(pred->value);
    bfree(pred);
}

void free_lnode_err(struct lnode_err* lnode)
{
    if(lnode == NULL) {
        return;
    }
    for(size_t i = 0; i < lnode->sub_count; i++) {
        const struct betree_sub* sub = lnode->subs[i];
        free_sub((struct betree_sub*)sub);
    }
    bfree(lnode->subs);
    lnode->subs = NULL;
    bfree(lnode);
}

void free_cnode_err(struct cnode_err* cnode)
{
    if(cnode == NULL) {
        return;
    }
    free_lnode_err(cnode->lnode);
    cnode->lnode = NULL;
    free_pdir_err(cnode->pdir);
    cnode->pdir = NULL;
    bfree(cnode);
}

static void free_cdir_err(struct cdir_err* cdir)
{
    if(cdir == NULL) {
        return;
    }
    bfree((char*)cdir->attr_var.attr);
    if(cdir->sub_ids) arraylist_destroy(cdir->sub_ids);
    free_cnode_err(cdir->cnode);
    cdir->cnode = NULL;
    free_cdir_err(cdir->lchild);
    cdir->lchild = NULL;
    free_cdir_err(cdir->rchild);
    cdir->rchild = NULL;
    bfree(cdir);
}

static void free_pnode_err(struct pnode_err* pnode)
{
    if(pnode == NULL) {
        return;
    }
    bfree((char*)pnode->attr_var.attr);
    free_cdir_err(pnode->cdir);
    pnode->cdir = NULL;
    bfree(pnode);
}

static struct betree_sub* find_sub_id_cdir_err(betree_sub_t id, struct cdir_err* cdir)
{
    if(cdir == NULL) {
        return NULL;
    }
    struct betree_sub* in_cnode = find_sub_id_err(id, cdir->cnode);
    if(in_cnode != NULL) {
        return in_cnode;
    }
    struct betree_sub* in_lcdir = find_sub_id_cdir_err(id, cdir->lchild);
    if(in_lcdir != NULL) {
        return in_lcdir;
    }
    struct betree_sub* in_rcdir = find_sub_id_cdir_err(id, cdir->rchild);
    if(in_rcdir != NULL) {
        return in_rcdir;
    }
    return NULL;
}

struct betree_sub* find_sub_id_err(betree_sub_t id, struct cnode_err* cnode)
{
    if(cnode == NULL) {
        return NULL;
    }
    for(size_t i = 0; i < cnode->lnode->sub_count; i++) {
        if(cnode->lnode->subs[i]->id == id) {
            return cnode->lnode->subs[i];
        }
    }
    if(cnode->pdir != NULL) {
        for(size_t i = 0; i < cnode->pdir->pnode_count; i++) {
            struct betree_sub* in_cdir = find_sub_id_cdir_err(id, cnode->pdir->pnodes[i]->cdir);
            if(in_cdir != NULL) {
                return in_cdir;
            }
        }
    }
    return NULL;
}

static void fill_pred_attr_var(struct betree_sub* sub, struct attr_var attr_var)
{
    set_bit(sub->attr_vars, attr_var.var);
}

static enum short_circuit_e short_circuit_for_attr_var(
    betree_var_t id, bool inverted, struct attr_var attr_var)
{
    if(id == attr_var.var) {
        if(inverted) {
            return SHORT_CIRCUIT_PASS;
        }
        return SHORT_CIRCUIT_FAIL;
    }
    return SHORT_CIRCUIT_NONE;
}

static enum short_circuit_e short_circuit_for_node(
    betree_var_t id, bool inverted, const struct ast_node* node)
{
    switch(node->type) {
        case AST_TYPE_IS_NULL_EXPR:
            switch(node->is_null_expr.type) {
                case AST_IS_NULL:
                    return short_circuit_for_attr_var(id, !inverted, node->is_null_expr.attr_var);
                case AST_IS_NOT_NULL:
                    return short_circuit_for_attr_var(id, inverted, node->is_null_expr.attr_var);
                case AST_IS_EMPTY:
                    return short_circuit_for_attr_var(id, inverted, node->is_null_expr.attr_var);
                default:
                    abort();
            }
        case AST_TYPE_COMPARE_EXPR:
            return short_circuit_for_attr_var(id, inverted, node->compare_expr.attr_var);
        case AST_TYPE_EQUALITY_EXPR:
            return short_circuit_for_attr_var(id, inverted, node->equality_expr.attr_var);
        case AST_TYPE_BOOL_EXPR:
            switch(node->bool_expr.op) {
                case AST_BOOL_LITERAL:
                    return SHORT_CIRCUIT_NONE;
                case AST_BOOL_OR: {
                    enum short_circuit_e lhs
                        = short_circuit_for_node(id, inverted, node->bool_expr.binary.lhs);
                    enum short_circuit_e rhs
                        = short_circuit_for_node(id, inverted, node->bool_expr.binary.rhs);
                    if(lhs == SHORT_CIRCUIT_PASS || rhs == SHORT_CIRCUIT_PASS) {
                        return SHORT_CIRCUIT_PASS;
                    }
                    if(lhs == SHORT_CIRCUIT_FAIL && rhs == SHORT_CIRCUIT_FAIL) {
                        return SHORT_CIRCUIT_FAIL;
                    }
                    return SHORT_CIRCUIT_NONE;
                }
                case AST_BOOL_AND: {
                    enum short_circuit_e lhs
                        = short_circuit_for_node(id, inverted, node->bool_expr.binary.lhs);
                    enum short_circuit_e rhs
                        = short_circuit_for_node(id, inverted, node->bool_expr.binary.rhs);
                    if(lhs == SHORT_CIRCUIT_FAIL || rhs == SHORT_CIRCUIT_FAIL) {
                        return SHORT_CIRCUIT_FAIL;
                    }
                    if(lhs == SHORT_CIRCUIT_PASS && rhs == SHORT_CIRCUIT_PASS) {
                        return SHORT_CIRCUIT_PASS;
                    }
                    return SHORT_CIRCUIT_NONE;
                }
                case AST_BOOL_NOT:
                    return short_circuit_for_node(id, !inverted, node->bool_expr.unary.expr);
                case AST_BOOL_VARIABLE:
                    return short_circuit_for_attr_var(id, inverted, node->bool_expr.variable);
                default:
                    abort();
            }
        case AST_TYPE_SET_EXPR:
            if(node->set_expr.left_value.value_type == AST_SET_LEFT_VALUE_VARIABLE) {
                return short_circuit_for_attr_var(
                    id, inverted, node->set_expr.left_value.variable_value);
            }
            else {
                return short_circuit_for_attr_var(
                    id, inverted, node->set_expr.right_value.variable_value);
            }
        case AST_TYPE_LIST_EXPR:
            return short_circuit_for_attr_var(id, inverted, node->list_expr.attr_var);
        case AST_TYPE_SPECIAL_EXPR:
            switch(node->special_expr.type) {
                case AST_SPECIAL_FREQUENCY: {
                    enum short_circuit_e frequency = short_circuit_for_attr_var(id, inverted, node->special_expr.frequency.attr_var);
                    enum short_circuit_e now = short_circuit_for_attr_var(id, inverted, node->special_expr.frequency.now);
                    if(frequency == SHORT_CIRCUIT_FAIL || now == SHORT_CIRCUIT_FAIL) {
                        return SHORT_CIRCUIT_FAIL;
                    }
                    if(frequency == SHORT_CIRCUIT_PASS && now == SHORT_CIRCUIT_PASS) {
                        return SHORT_CIRCUIT_PASS;
                    }
                    return SHORT_CIRCUIT_NONE;
                }
                case AST_SPECIAL_SEGMENT: {
                    enum short_circuit_e frequency = short_circuit_for_attr_var(id, inverted, node->special_expr.segment.attr_var);
                    enum short_circuit_e now = short_circuit_for_attr_var(id, inverted, node->special_expr.segment.now);
                    if(frequency == SHORT_CIRCUIT_FAIL || now == SHORT_CIRCUIT_FAIL) {
                        return SHORT_CIRCUIT_FAIL;
                    }
                    if(frequency == SHORT_CIRCUIT_PASS && now == SHORT_CIRCUIT_PASS) {
                        return SHORT_CIRCUIT_PASS;
                    }
                    return SHORT_CIRCUIT_NONE;
                }
                case AST_SPECIAL_GEO: {
                    enum short_circuit_e latitude = short_circuit_for_attr_var(id, inverted, node->special_expr.geo.latitude_var);
                    enum short_circuit_e longitude = short_circuit_for_attr_var(id, inverted, node->special_expr.geo.longitude_var);
                    if(latitude == SHORT_CIRCUIT_FAIL || longitude == SHORT_CIRCUIT_FAIL) {
                        return SHORT_CIRCUIT_FAIL;
                    }
                    if(latitude == SHORT_CIRCUIT_PASS && longitude == SHORT_CIRCUIT_PASS) {
                        return SHORT_CIRCUIT_PASS;
                    }
                    return SHORT_CIRCUIT_NONE;
                }
                case AST_SPECIAL_STRING:
                    return short_circuit_for_attr_var(
                        id, inverted, node->special_expr.string.attr_var);
                default:
                    abort();
            }
        default:
            abort();
    }
    return SHORT_CIRCUIT_NONE;
}

static void fill_short_circuit(struct config* config, struct betree_sub* sub)
{
    for(size_t i = 0; i < config->attr_domain_count; i++) {
        struct attr_domain* attr_domain = config->attr_domains[i];
        if(attr_domain->allow_undefined) {
            enum short_circuit_e result
                = short_circuit_for_node(attr_domain->attr_var.var, false, sub->expr);
            if(result == SHORT_CIRCUIT_PASS) {
                set_bit(sub->short_circuit.pass, i);
            }
            else if(result == SHORT_CIRCUIT_FAIL) {
                set_bit(sub->short_circuit.fail, i);
            }
        }
    }
}

int parse(const char* text, struct ast_node** node);
int event_parse(const char* text, struct betree_event** event);

const char* find_pnode_attr_name(struct cdir_err* cdir) {
    if(!cdir) return NULL;
    struct cdir_err* pcd = cdir;
    while(pcd && pcd->parent_type != CNODE_PARENT_PNODE)
        pcd = pcd->cdir_parent;
    if(!pcd || !pcd->pnode_parent) return NULL;
    return pcd->pnode_parent->attr_var.attr;
}


void add_sub_err(betree_sub_t id, struct report_err* report)
{
    if(report->matched == 0) {
        report->subs = bcalloc(sizeof(*report->subs));
        if(report->subs == NULL) {
            fprintf(stderr, "%s bcalloc failed", __func__);
            abort();
        }
    }
    else {
        betree_sub_t* subs
            = brealloc(report->subs, sizeof(*report->subs) * (report->matched + 1));
        if(subs == NULL) {
            fprintf(stderr, "%s brealloc failed", __func__);
            abort();
        }
        report->subs = subs;
    }
    report->subs[report->matched] = id;
    report->matched++;
}

bool betree_search_with_preds_err(const struct config* config,
    const struct betree_variable** preds,
    const struct cnode_err* cnode,
    struct report_err* report
    )
{
    uint64_t* undefined = make_undefined(config->attr_domain_count, preds);
    struct memoize memoize = make_memoize(config->pred_map->memoize_count);
    struct subs_to_eval subs;
    init_subs_to_eval(&subs);
    match_be_tree_err((const struct attr_domain**)config->attr_domains, preds, cnode, &subs, report);
    for(size_t i = 0; i < subs.count; i++) {
        const struct betree_sub* sub = subs.subs[i];
        report->evaluated++;
        char last_reason[512];
        if(match_sub_err(config->attr_domain_count, preds, sub, report, &memoize, undefined, last_reason, 
        (struct attr_domain**)config->attr_domains) == true) {
            add_sub_err(sub->id, report);
        }
        else{
            add_reason_sub_id_list(report, last_reason, sub->id);
        }
    }
    bfree(subs.subs);
    free_memoize(memoize);
    bfree(undefined);
    bfree(preds);
    return true;
}



bool betree_search_with_preds_ids_err(const struct config* config,
    const struct betree_variable** preds,
    const struct cnode_err* cnode,
    struct report_err* report,
    const uint64_t* ids,
    size_t sz
    )
{
    uint64_t* undefined = make_undefined(config->attr_domain_count, preds);
    struct memoize memoize = make_memoize(config->pred_map->memoize_count);
    struct subs_to_eval subs;
    init_subs_to_eval(&subs);
    match_be_tree_ids_err((const struct attr_domain**)config->attr_domains, preds, cnode, &subs, ids, sz, report);
    for(size_t i = 0; i < subs.count; i++) {
        const struct betree_sub* sub = subs.subs[i];
        report->evaluated++;
        char last_reason[512];
        if(match_sub_err(config->attr_domain_count, preds, sub, report, &memoize, undefined, last_reason, (struct attr_domain**)config->attr_domains) == true) {
            add_sub_err(sub->id, report);
        }
    }
    bfree(subs.subs);
    free_memoize(memoize);
    bfree(undefined);
    bfree(preds);
    return true;
}

struct betree_event* make_event_from_string_err(const struct betree_err* betree, const char* event_str)
{
    struct betree_event* event;
    if(likely(event_parse(event_str, &event))) {
        fprintf(stderr, "Failed to parse event: %s\n", event_str);
        abort();
    }
    fill_event(betree->config, event);
    sort_event_lists(event);
    return event;
}

void build_sub_ids_cdir(struct cdir_err* cd)
{
    if(cd->lchild) build_sub_ids_cdir(cd->lchild);
    if(cd->rchild) build_sub_ids_cdir(cd->rchild);
    if(cd->cnode) build_sub_ids_cnode(cd->cnode);
    if(cd->lchild && cd->lchild->sub_ids->size > 0)
        arraylist_join(cd->sub_ids, cd->lchild->sub_ids);
    if(cd->rchild && cd->rchild->sub_ids->size > 0)
        arraylist_join(cd->sub_ids, cd->rchild->sub_ids);
}

void build_sub_ids_cnode(struct cnode_err* cn)
{
    if(!cn) return;
    if(cn->lnode) {
        for(size_t i = 0; i < cn->lnode->sub_count; i++) {
            if(cn->parent) {
                arraylist_add(cn->parent->sub_ids, cn->lnode->subs[i]->id);
            }
        }
    }
    if(cn->pdir) {
        int pnode_count = cn->pdir->pnode_count;
        for(size_t i = 0; i < pnode_count; i++) {
            struct pnode_err* pn = cn->pdir->pnodes[i];
            if(!pn) continue;
            struct cdir_err* cd = pn->cdir;
            if(cd) build_sub_ids_cdir(cd);
            if(cn->parent && cd->sub_ids->size > 0)
                arraylist_join(cn->parent->sub_ids, cd->sub_ids);
        }
    }
}