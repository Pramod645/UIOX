/**
 * @file  uiox_pkg_resolve.c
 * @brief UIOX Package Manager — dependency resolver (Kahn's topo-sort).
 * @date  2026-06-29
 */

 #include "../include/uiox_pkg_device.h"
 #include <string.h>
 #include <stdio.h>
 
 /* =========================================================================
  * Internal helpers
  * ====================================================================== */
 
 static int find_node(uiox_pkg_resolver_t *res, const char *name)
 {
     for (uint32_t i = 0u; i < res->node_count; i++)
         if (strncmp(res->node_name[i], name, UIOX_PKG_NAME_MAX) == 0)
             return (int)i;
     return -1;
 }
 
 static int add_node(uiox_pkg_resolver_t *res,
                      const char *name, uint32_t ver)
 {
     int idx = find_node(res, name);
     if (idx >= 0) return idx;
     if (res->node_count >= UIOX_PKG_GRAPH_MAX) return -1;
     idx = (int)res->node_count++;
     strncpy(res->node_name[idx], name, UIOX_PKG_NAME_MAX - 1u);
     res->node_ver[idx]   = ver;
     res->nadj[idx]       = 0u;
     res->in_degree[idx]  = 0u;
     return idx;
 }
 
 static void add_edge(uiox_pkg_resolver_t *res, int from, int to)
 {
     if (from < 0 || to < 0) return;
     if (res->nadj[from] < UIOX_PKG_MAX_DEPS) {
         res->adj[from][res->nadj[from]++] = (uint8_t)to;
         res->in_degree[to]++;
     }
 }
 
 /* Build dependency graph by DFS into store archives */
 static uiox_pkg_err_t build_graph(uiox_pkg_resolver_t *res,
                                     const char *name,
                                     uint32_t version)
 {
     int idx = add_node(res, name, version);
     if (idx < 0) return UIOX_PKG_ERR_OVERFLOW;
 
     uiox_pkg_rec_t rec;
     uiox_pkg_err_t rc = uiox_pkg_store_load_pkg(res->store, name, &rec);
     if (rc != UIOX_PKG_OK) return UIOX_PKG_ERR_NOTFOUND;
 
     for (uint32_t i = 0u; i < rec.dep_count; i++) {
         const uiox_pkg_dep_t *d = &rec.deps[i];
         if (d->type == UIOX_PKG_DEP_CONFLICT) continue;
         if (d->type != UIOX_PKG_DEP_REQUIRED  &&
             d->type != UIOX_PKG_DEP_OPTIONAL) continue;
 
         /* Check if already installed */
         uiox_pkg_index_entry_t *ie =
             uiox_pkg_store_index_find(res->store, d->name);
         if (ie && (uiox_pkg_state_t)ie->state == UIOX_PKG_STATE_INSTALLED)
             continue;  /* Already satisfied */
 
         /* Recurse */
         int dep_idx = add_node(res, d->name, d->ver_min);
         if (dep_idx < 0) return UIOX_PKG_ERR_OVERFLOW;
         add_edge(res, idx, dep_idx);   /* pkg → dep (must install dep first) */
 
         rc = build_graph(res, d->name, d->ver_min);
         if (rc == UIOX_PKG_ERR_NOTFOUND && d->type == UIOX_PKG_DEP_OPTIONAL)
             rc = UIOX_PKG_OK;
         if (rc != UIOX_PKG_OK) return rc;
     }
     return UIOX_PKG_OK;
 }
 
 /* Kahn's algorithm topological sort → fills plan in install order */
 static uiox_pkg_err_t topo_sort(uiox_pkg_resolver_t *res,
                                   uiox_pkg_plan_t *plan,
                                   uiox_pkg_op_t op)
 {
     /* Queue of nodes with in_degree == 0 */
     uint8_t  queue[UIOX_PKG_GRAPH_MAX];
     uint32_t q_head = 0u, q_tail = 0u;
 
     uint8_t in_deg[UIOX_PKG_GRAPH_MAX];
     memcpy(in_deg, res->in_degree,
            res->node_count * sizeof(uint8_t));
 
     for (uint32_t i = 0u; i < res->node_count; i++)
         if (in_deg[i] == 0u) queue[q_tail++] = (uint8_t)i;
 
     uint32_t processed = 0u;
     while (q_head < q_tail) {
         uint8_t u = queue[q_head++];
         if (plan->count >= UIOX_PKG_PLAN_MAX) return UIOX_PKG_ERR_OVERFLOW;
 
         uiox_pkg_plan_entry_t *e = &plan->entries[plan->count++];
         strncpy(e->name, res->node_name[u], UIOX_PKG_NAME_MAX - 1u);
         e->version = res->node_ver[u];
         e->op      = op;
         e->is_auto = (processed > 0u); /* first is explicit, rest auto */
         processed++;
 
         for (uint8_t j = 0u; j < res->nadj[u]; j++) {
             uint8_t v = res->adj[u][j];
             if (--in_deg[v] == 0u) queue[q_tail++] = v;
         }
     }
 
     if (processed != res->node_count) {
         /* Cycle detected */
         plan->has_conflicts = true;
         strncpy(plan->conflict_msg,
                 "Circular dependency detected",
                 sizeof(plan->conflict_msg) - 1u);
         return UIOX_PKG_ERR_CONFLICT;
     }
     return UIOX_PKG_OK;
 }
 
 /* =========================================================================
  * Public API
  * ====================================================================== */
 
 uiox_pkg_err_t uiox_pkg_resolve_init(uiox_pkg_resolver_t *res,
                                        uiox_pkg_store_t *store)
 {
     if (!res || !store) return UIOX_PKG_ERR_INVAL;
     memset(res, 0, sizeof(*res));
     res->store = store;
     return UIOX_PKG_OK;
 }
 
 uiox_pkg_err_t uiox_pkg_resolve_install(uiox_pkg_resolver_t *res,
                                           const char *name,
                                           uint32_t version,
                                           uiox_pkg_plan_t *plan)
 {
     if (!res || !name || !plan) return UIOX_PKG_ERR_INVAL;
     memset(res->node_name, 0, sizeof(res->node_name));
     res->node_count = 0u;
     memset(plan, 0, sizeof(*plan));
 
     /* Check not already installed */
     uiox_pkg_index_entry_t *ie =
         uiox_pkg_store_index_find(res->store, name);
     if (ie && (uiox_pkg_state_t)ie->state == UIOX_PKG_STATE_INSTALLED)
         return UIOX_PKG_ERR_ALREADY;
 
     uiox_pkg_err_t rc = build_graph(res, name, version);
     if (rc != UIOX_PKG_OK) return rc;
 
     return topo_sort(res, plan, UIOX_PKG_OP_INSTALL);
 }
 
 uiox_pkg_err_t uiox_pkg_resolve_remove(uiox_pkg_resolver_t *res,
                                          const char *name,
                                          uiox_pkg_plan_t *plan)
 {
     if (!res || !name || !plan) return UIOX_PKG_ERR_INVAL;
     memset(plan, 0, sizeof(*plan));
 
     /* Check installed */
     uiox_pkg_index_entry_t *ie =
         uiox_pkg_store_index_find(res->store, name);
     if (!ie || (uiox_pkg_state_t)ie->state != UIOX_PKG_STATE_INSTALLED)
         return UIOX_PKG_ERR_NOTFOUND;
 
     /* Check reverse dependencies (nothing installed depends on us) */
     for (uint32_t i = 0u; i < res->store->index_count; i++) {
         const uiox_pkg_index_entry_t *e = &res->store->index[i];
         if (strncmp(e->name, name, UIOX_PKG_NAME_MAX) == 0) continue;
         if ((uiox_pkg_state_t)e->state != UIOX_PKG_STATE_INSTALLED) continue;
 
         /* Load and check deps */
         uiox_pkg_rec_t rec;
         if (uiox_pkg_store_load_pkg(res->store, e->name, &rec) != UIOX_PKG_OK)
             continue;
         for (uint32_t j = 0u; j < rec.dep_count; j++) {
             if (rec.deps[j].type != UIOX_PKG_DEP_REQUIRED) continue;
             if (strncmp(rec.deps[j].name, name, UIOX_PKG_NAME_MAX) == 0) {
                 plan->has_conflicts = true;
                 snprintf(plan->conflict_msg, sizeof(plan->conflict_msg),
                          "%s requires %s", e->name, name);
                 return UIOX_PKG_ERR_CONFLICT;
             }
         }
     }
 
     if (plan->count >= UIOX_PKG_PLAN_MAX) return UIOX_PKG_ERR_OVERFLOW;
     uiox_pkg_plan_entry_t *pe = &plan->entries[plan->count++];
     strncpy(pe->name, name, UIOX_PKG_NAME_MAX - 1u);
     pe->version = ie->version;
     pe->op      = UIOX_PKG_OP_REMOVE;
     pe->is_auto = false;
     return UIOX_PKG_OK;
 }
 
 uiox_pkg_err_t uiox_pkg_resolve_upgrade(uiox_pkg_resolver_t *res,
                                           const char *name,
                                           uint32_t new_version,
                                           uiox_pkg_plan_t *plan)
 {
     if (!res || !name || !plan) return UIOX_PKG_ERR_INVAL;
     memset(plan, 0, sizeof(*plan));
 
     uiox_pkg_index_entry_t *ie =
         uiox_pkg_store_index_find(res->store, name);
     if (!ie) return UIOX_PKG_ERR_NOTFOUND;
     if (new_version <= ie->version) return UIOX_PKG_ERR_INVAL;
 
     if (plan->count >= UIOX_PKG_PLAN_MAX) return UIOX_PKG_ERR_OVERFLOW;
     uiox_pkg_plan_entry_t *pe = &plan->entries[plan->count++];
     strncpy(pe->name, name, UIOX_PKG_NAME_MAX - 1u);
     pe->version = new_version;
     pe->op      = UIOX_PKG_OP_UPGRADE;
     pe->is_auto = false;
     return UIOX_PKG_OK;
 }
 
 void uiox_pkg_resolve_print(const uiox_pkg_plan_t *plan)
 {
     static const char *op_names[] = { "INSTALL","UPGRADE","REMOVE","KEEP" };
     if (!plan) return;
     printf("  Resolution plan (%u steps):\n", plan->count);
     if (plan->has_conflicts)
         printf("  CONFLICT: %s\n", plan->conflict_msg);
     for (uint32_t i = 0u; i < plan->count; i++) {
         const uiox_pkg_plan_entry_t *e = &plan->entries[i];
         uint8_t t = (uint8_t)e->op;
         printf("  [%2u] %-8s  %-32s  v%u.%u.%u%s\n",
                i, t < 4u ? op_names[t] : "?",
                e->name,
                UIOX_PKG_VER_MAJOR(e->version),
                UIOX_PKG_VER_MINOR(e->version),
                UIOX_PKG_VER_PATCH(e->version),
                e->is_auto ? "  [auto]" : "");
     }
 }
 