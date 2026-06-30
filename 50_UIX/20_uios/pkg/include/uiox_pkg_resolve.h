/**
 * @file  uiox_pkg_resolve.h
 * @brief UIOX Package Manager — dependency resolver (DAG, topo-sort).
 *
 * Builds a directed acyclic graph (DAG) of package dependencies,
 * performs topological sort (Kahn's algorithm), and detects conflicts.
 *
 * @version 1.0.0
 * @date    2026-06-29
 */

 #ifndef UIOX_PKG_RESOLVE_H
 #define UIOX_PKG_RESOLVE_H
 
 #include "uiox_pkg_store.h"
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 /* =========================================================================
  * Resolution plan
  * A sorted list of operations to perform in order.
  * ====================================================================== */
 
 typedef enum {
     UIOX_PKG_OP_INSTALL  = 0,
     UIOX_PKG_OP_UPGRADE  = 1,
     UIOX_PKG_OP_REMOVE   = 2,
     UIOX_PKG_OP_KEEP     = 3,   /**< Already satisfied — no action      */
 } uiox_pkg_op_t;
 
 typedef struct {
     char          name   [UIOX_PKG_NAME_MAX];
     uint32_t      version;
     uiox_pkg_op_t op;
     bool          is_auto;  /**< Auto-installed as dependency            */
 } uiox_pkg_plan_entry_t;
 
 #define UIOX_PKG_PLAN_MAX   128u
 
 typedef struct {
     uiox_pkg_plan_entry_t entries[UIOX_PKG_PLAN_MAX];
     uint32_t              count;
     bool                  has_conflicts;
     char                  conflict_msg[256];
 } uiox_pkg_plan_t;
 
 /* =========================================================================
  * Resolver context
  * ====================================================================== */
 
 #define UIOX_PKG_GRAPH_MAX   UIOX_PKG_REC_POOL_SIZE
 
 typedef struct {
     uiox_pkg_store_t *store;
     /* Adjacency: node[i].name depends on adj[i][0..nadj[i]-1] */
     char     node_name[UIOX_PKG_GRAPH_MAX][UIOX_PKG_NAME_MAX];
     uint32_t node_ver [UIOX_PKG_GRAPH_MAX];
     uint8_t  adj      [UIOX_PKG_GRAPH_MAX][UIOX_PKG_MAX_DEPS];
     uint8_t  nadj     [UIOX_PKG_GRAPH_MAX];
     uint8_t  in_degree[UIOX_PKG_GRAPH_MAX];
     uint32_t node_count;
 } uiox_pkg_resolver_t;
 
 /* =========================================================================
  * Resolver API
  * ====================================================================== */
 
 uiox_pkg_err_t uiox_pkg_resolve_init  (uiox_pkg_resolver_t *res,
                                         uiox_pkg_store_t *store);
 
 /**
  * Build an install plan for @name.
  * Walks dependency tree, checks conflicts, and produces an ordered plan.
  */
 uiox_pkg_err_t uiox_pkg_resolve_install(uiox_pkg_resolver_t *res,
                                          const char *name,
                                          uint32_t version,
                                          uiox_pkg_plan_t *plan);
 
 /**
  * Build a removal plan for @name.
  * Checks reverse dependencies (nothing else needs it).
  */
 uiox_pkg_err_t uiox_pkg_resolve_remove (uiox_pkg_resolver_t *res,
                                          const char *name,
                                          uiox_pkg_plan_t *plan);
 
 /**
  * Build an upgrade plan: remove old, install new.
  */
 uiox_pkg_err_t uiox_pkg_resolve_upgrade(uiox_pkg_resolver_t *res,
                                          const char *name,
                                          uint32_t new_version,
                                          uiox_pkg_plan_t *plan);
 
 /** Print a plan to kernel console. */
 void           uiox_pkg_resolve_print  (const uiox_pkg_plan_t *plan);
 
 #ifdef __cplusplus
 }
 #endif
 #endif /* UIOX_PKG_RESOLVE_H */
 