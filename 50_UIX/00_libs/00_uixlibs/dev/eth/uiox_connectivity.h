/*
 * 50_UIX/00_libs/00_uixlibs/dev/eth/uiox_connectivity.h
 *
 * UIOX high-level connectivity feature API — EL0.
 *
 *   21_apps/app_eth.c → dev/eth/uiox_connectivity.c
 *     → dev/uiox_dev_stream.c → sys_open/sys_read/sys_write/uix_poll
 *       → 40_SCIX → 30_KIX
 *
 * No 30_KIX header.
 *
 * @date    2026-05-25
 */
#ifndef UIOX_CONNECTIVITY_H
#define UIOX_CONNECTIVITY_H

#include "../sys/uix_types.h"    /* uix_uint8_t, uix_uint32_t, uix_size_t, uix_ssize_t, uix_bool_t */

#ifdef __cplusplus
extern "C" {
#endif

/* =========================================================================
 * Constants
 * ====================================================================== */

#define UIOX_DNS_MAX_SERVERS        4       /**< Max DNS server entries        */
#define UIOX_DNS_NAME_MAX           253     /**< Max DNS hostname length       */
#define UIOX_DNS_TIMEOUT_MS         3000    /**< DNS query timeout             */
#define UIOX_DNS_RETRIES            3       /**< DNS query retry count         */
#define UIOX_DHCP_TIMEOUT_MS        10000   /**< DHCP discover timeout         */
#define UIOX_DHCP_RETRY_COUNT       3       /**< DHCP retry count              */
#define UIOX_NTP_PORT               123     /**< NTP UDP port                  */
#define UIOX_NTP_TIMEOUT_MS         5000    /**< NTP request timeout           */
#define UIOX_HTTP_MAX_HEADERS       16      /**< Max HTTP headers per request  */
#define UIOX_HTTP_MAX_URL_LEN       512     /**< Max HTTP URL length           */
#define UIOX_HTTP_MAX_HDR_LEN       128     /**< Max single HTTP header length */
#define UIOX_HTTP_RESP_BUF_MIN      256     /**< Min response buffer size      */

/* =========================================================================
 * Network event types
 * ====================================================================== */

typedef enum {
    UIOX_NET_EVT_LINK_UP = 0,
    UIOX_NET_EVT_LINK_DOWN,
    UIOX_NET_EVT_IP_ACQUIRED,
    UIOX_NET_EVT_IP_LOST,
    UIOX_NET_EVT_DNS_READY,
    UIOX_NET_EVT_NTP_SYNCED,
    UIOX_NET_EVT_ERROR,
} uiox_net_evt_t;

typedef void (*uiox_net_evt_cb_t)(uiox_net_evt_t event,
                                   void          *iface,
                                   void          *user_ctx);

/* =========================================================================
 * Stack configuration
 * ====================================================================== */

typedef enum {
    UIOX_ADDR_MODE_STATIC = 0,
    UIOX_ADDR_MODE_DHCP,
} uiox_addr_mode_t;

#define UIOX_IFNAME_MAX  16
#define UIOX_MAC_LEN      6

typedef struct {
    char           ifname[UIOX_IFNAME_MAX];  /**< e.g. "eth0"                 */
    uix_uint8_t    mac[UIOX_MAC_LEN];        /**< MAC (0=use hw)              */
    uix_uint16_t   mtu;

    uiox_addr_mode_t addr_mode;
    uix_uint32_t   static_ip;
    uix_uint32_t   static_mask;
    uix_uint32_t   static_gw;

    uix_uint32_t   dns_servers[UIOX_DNS_MAX_SERVERS];
    uix_uint8_t    dns_server_count;

    uix_uint32_t   ntp_server_ip;

    uiox_net_evt_cb_t evt_cb;
    void             *evt_ctx;
} uiox_net_config_t;

/* =========================================================================
 * DHCP lease information
 * ====================================================================== */

typedef struct {
    uix_uint32_t  ip;
    uix_uint32_t  mask;
    uix_uint32_t  gateway;
    uix_uint32_t  dns[UIOX_DNS_MAX_SERVERS];
    uix_uint8_t   dns_count;
    uix_uint32_t  lease_time_s;
    uix_uint32_t  renew_time_s;
    uix_uint32_t  rebind_time_s;
    uix_uint32_t  server_ip;
} uiox_dhcp_lease_t;

/* =========================================================================
 * DNS resolved result
 * ====================================================================== */

typedef struct {
    uix_uint32_t  addrs[4];
    uix_uint8_t   count;
    uix_uint32_t  ttl;
} uiox_dns_result_t;

/* =========================================================================
 * HTTP request / response
 * ====================================================================== */

typedef enum {
    UIOX_HTTP_GET = 0,
    UIOX_HTTP_POST,
    UIOX_HTTP_PUT,
    UIOX_HTTP_DELETE,
    UIOX_HTTP_HEAD,
} uiox_http_method_t;

typedef struct {
    char key  [UIOX_HTTP_MAX_HDR_LEN];
    char value[UIOX_HTTP_MAX_HDR_LEN];
} uiox_http_header_t;

typedef struct {
    uiox_http_method_t  method;
    char                url[UIOX_HTTP_MAX_URL_LEN];
    uiox_http_header_t  headers[UIOX_HTTP_MAX_HEADERS];
    uix_uint8_t         header_count;
    const void         *body;
    uix_size_t          body_len;
    uix_uint32_t        timeout_ms;
} uiox_http_request_t;

typedef struct {
    int                 status_code;
    uiox_http_header_t  headers[UIOX_HTTP_MAX_HEADERS];
    uix_uint8_t         header_count;
    uix_uint8_t        *body;
    uix_size_t          body_len;
    uix_size_t          body_capacity;
} uiox_http_response_t;

/* =========================================================================
 * NTP result
 * ====================================================================== */

typedef struct {
    uix_uint32_t  unix_time;
    uix_int32_t   offset_ms;
    uix_uint32_t  round_trip_ms;
} uiox_ntp_result_t;

/* =========================================================================
 * Ping result
 * ====================================================================== */

typedef struct {
    uix_uint32_t  target_ip;
    uix_uint32_t  rtt_ms;
    uix_bool_t    reachable;
    uix_uint8_t   ttl;
} uiox_ping_result_t;

/* =========================================================================
 * Interface statistics
 * ====================================================================== */

typedef struct {
    uix_uint64_t  rx_packets;
    uix_uint64_t  rx_bytes;
    uix_uint64_t  rx_errors;
    uix_uint64_t  rx_dropped;
    uix_uint64_t  tx_packets;
    uix_uint64_t  tx_bytes;
    uix_uint64_t  tx_errors;
} uiox_net_stats_t;

/* =========================================================================
 * Lifecycle
 * ====================================================================== */

uix_int32_t  uiox_net_init  (const uiox_net_config_t *cfg);
void uiox_net_deinit(void);
uix_uint32_t uiox_net_local_ip(void);
void uiox_net_tick  (uix_uint32_t now_ms);
int  uiox_net_stats (uiox_net_stats_t *out);

/* =========================================================================
 * DHCP client
 * ====================================================================== */

int  uiox_dhcp_request(uiox_dhcp_lease_t *lease_out);
void uiox_dhcp_release(void);
int  uiox_dhcp_renew  (uiox_dhcp_lease_t *lease_out);
uix_bool_t uiox_dhcp_lease(uiox_dhcp_lease_t *out);

/* =========================================================================
 * DNS resolver
 * ====================================================================== */

void         uiox_dns_set_servers(const uix_uint32_t *servers, uix_uint8_t count);
int          uiox_dns_resolve(const char *hostname, uiox_dns_result_t *result);
uix_uint32_t uiox_dns_resolve_first(const char *hostname);
void         uiox_dns_cache_flush(void);

/* =========================================================================
 * NTP client
 * ====================================================================== */

int          uiox_ntp_sync(uix_uint32_t server_ip, uiox_ntp_result_t *result);
uix_uint32_t uiox_ntp_last_sync(void);

/* =========================================================================
 * HTTP client
 * ====================================================================== */

int uiox_http_request(const uiox_http_request_t *req,
                       uiox_http_response_t      *resp);

uix_ssize_t uiox_http_get(const char *url, void *buf, uix_size_t buf_len,
                           int *status_out);

uix_ssize_t uiox_http_post(const char *url,
                            const void *body,     uix_size_t body_len,
                            void       *resp_buf, uix_size_t resp_buf_len,
                            int        *status_out);

/* =========================================================================
 * Ping / reachability
 * ====================================================================== */

int  uiox_ping(uix_uint32_t target_ip, uix_uint32_t timeout_ms,
                uiox_ping_result_t *result);
uix_bool_t uiox_reachable(uix_uint32_t target_ip, uix_uint32_t timeout_ms);

/* =========================================================================
 * IP address helpers
 * ====================================================================== */

uix_uint32_t uiox_ip4_from_str(const char *str);
void         uiox_ip4_to_str(uix_uint32_t ip, char *buf, uix_size_t buf_len);

#ifdef __cplusplus
}
#endif

#endif /* UIOX_CONNECTIVITY_H */
