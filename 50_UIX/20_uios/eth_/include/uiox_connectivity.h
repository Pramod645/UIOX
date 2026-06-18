/**
 * @file    uiox_connectivity.h
 * @brief   UIOX high-level connectivity feature API.
 *
 * This is the top-most layer of the UIOX networking stack. It exposes
 * high-level network features to application code:
 *
 *   - Stack bring-up / tear-down lifecycle
 *   - DHCP client
 *   - DNS resolver
 *   - NTP time synchronisation
 *   - HTTP client helper
 *   - Ping / reachability test
 *   - Network event callbacks
 *
 * Applications should only need to include this single header and call
 * uiox_net_init() at boot. All lower layers are initialised internally.
 *
 * @date    2026-05-25
 */
//Layer 7 — Connectivity Feature API
 #ifndef UIOX_CONNECTIVITY_H
 #define UIOX_CONNECTIVITY_H
 
 #include "uiox_netif.h"
 #include "uiox_socket.h"
 #include <stdint.h>
 #include <stdbool.h>
 #include <stddef.h>
 
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
     UIOX_NET_EVT_LINK_UP = 0,   /**< Physical link established              */
     UIOX_NET_EVT_LINK_DOWN,     /**< Physical link lost                     */
     UIOX_NET_EVT_IP_ACQUIRED,   /**< IP address acquired (DHCP or static)   */
     UIOX_NET_EVT_IP_LOST,       /**< IP address lost (DHCP lease expired)   */
     UIOX_NET_EVT_DNS_READY,     /**< DNS servers configured                 */
     UIOX_NET_EVT_NTP_SYNCED,    /**< System clock synchronised via NTP      */
     UIOX_NET_EVT_ERROR,         /**< Unrecoverable error — check err_code   */
 } uiox_net_evt_t;
 
 /** Network event callback — invoked from the network task context. */
 typedef void (*uiox_net_evt_cb_t)(uiox_net_evt_t event,
                                    uiox_netif_t  *netif,
                                    void          *user_ctx);
 
 /* =========================================================================
  * Stack configuration
  * ====================================================================== */
 
 typedef enum {
     UIOX_ADDR_MODE_STATIC = 0,  /**< Manually configured IP address         */
     UIOX_ADDR_MODE_DHCP,        /**< DHCP client (recommended for Ethernet) */
 } uiox_addr_mode_t;
 
 typedef struct {
     uiox_hw_dev_t      *hw;             /**< Hardware device (HAL layer)      */
     const uiox_hw_ops_t *hw_ops;        /**< Hardware ops vtable              */
     char                ifname[UIOX_NETIF_NAME_LEN]; /**< e.g. "eth0"        */
     uint8_t             mac[UIOX_HW_MAC_ADDR_LEN];   /**< MAC (0=use hw)     */
     uint16_t            mtu;            /**< MTU (0 = UIOX_HW_MTU_ETHERNET)  */
 
     /* IP addressing */
     uiox_addr_mode_t    addr_mode;
     uint32_t            static_ip;      /**< Used when mode == STATIC         */
     uint32_t            static_mask;
     uint32_t            static_gw;
 
     /* DNS */
     uint32_t            dns_servers[UIOX_DNS_MAX_SERVERS];
     uint8_t             dns_server_count;
 
     /* NTP */
     uint32_t            ntp_server_ip;  /**< 0 = skip NTP sync               */
 
     /* Event callback */
     uiox_net_evt_cb_t   evt_cb;
     void               *evt_ctx;
 } uiox_net_config_t;
 
 /* =========================================================================
  * DHCP lease information
  * ====================================================================== */
 
 typedef struct {
     uint32_t    ip;             /**< Assigned IP address (host order)        */
     uint32_t    mask;           /**< Subnet mask (host order)                */
     uint32_t    gateway;        /**< Default gateway (host order)            */
     uint32_t    dns[UIOX_DNS_MAX_SERVERS];
     uint8_t     dns_count;
     uint32_t    lease_time_s;   /**< Lease duration in seconds               */
     uint32_t    renew_time_s;   /**< T1: renewal time                        */
     uint32_t    rebind_time_s;  /**< T2: rebind time                         */
     uint32_t    server_ip;      /**< DHCP server IP                          */
 } uiox_dhcp_lease_t;
 
 /* =========================================================================
  * DNS resolved result
  * ====================================================================== */
 
 typedef struct {
     uint32_t    addrs[4];       /**< Resolved IPv4 addresses                 */
     uint8_t     count;          /**< Number of addresses returned            */
     uint32_t    ttl;            /**< Record TTL in seconds                   */
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
     uint8_t             header_count;
     const void         *body;           /**< Request body (may be NULL)      */
     size_t              body_len;
     uint32_t            timeout_ms;     /**< 0 = default 10 000 ms           */
 } uiox_http_request_t;
 
 typedef struct {
     int         status_code;            /**< HTTP status (200, 404, etc.)    */
     uiox_http_header_t headers[UIOX_HTTP_MAX_HEADERS];
     uint8_t     header_count;
     uint8_t    *body;                   /**< Caller-provided buffer          */
     size_t      body_len;               /**< Bytes written into body buffer  */
     size_t      body_capacity;          /**< Size of caller's body buffer    */
 } uiox_http_response_t;
 
 /* =========================================================================
  * NTP result
  * ====================================================================== */
 
 typedef struct {
     uint32_t    unix_time;      /**< Seconds since Unix epoch (UTC)          */
     int32_t     offset_ms;      /**< Estimated offset from local clock (ms)  */
     uint32_t    round_trip_ms;  /**< Round-trip delay measurement            */
 } uiox_ntp_result_t;
 
 /* =========================================================================
  * Ping result
  * ====================================================================== */
 
 typedef struct {
     uint32_t    target_ip;
     uint32_t    rtt_ms;         /**< Round-trip time (0 if timed out)        */
     bool        reachable;
     uint8_t     ttl;            /**< TTL from reply                          */
 } uiox_ping_result_t;
 
 /* =========================================================================
  * Lifecycle API
  * ====================================================================== */
 
 /**
  * @brief  Bring up the entire UIOX networking stack.
  *
  * Performs in order:
  *   1. HAL init + PHY auto-negotiation
  *   2. Netif registration and link-up
  *   3. DHCP or static IP assignment
  *   4. DNS server configuration
  *   5. Optional NTP synchronisation
  *   6. Fires UIOX_NET_EVT_IP_ACQUIRED on success
  *
  * @param  cfg   Stack configuration (must remain valid for stack lifetime).
  * @return 0 on success, negative errno on failure.
  */
 int  uiox_net_init   (const uiox_net_config_t *cfg);
 
 /**
  * @brief  Tear down the network stack gracefully.
  *
  * Closes all open sockets, releases DHCP lease, brings link down,
  * and deinitialises hardware.
  */
 void uiox_net_deinit (void);
 
 /**
  * @brief  Return the primary active network interface.
  */
 uiox_netif_t *uiox_net_primary_if(void);
 
 /**
  * @brief  Query current IP address of the primary interface.
  * @return IPv4 address in host byte order, 0 if not configured.
  */
 uint32_t uiox_net_local_ip(void);
 
 /**
  * @brief  Periodic tick — call from a timer or task at ~100 ms intervals.
  *
  * Drives DHCP lease renewal, ARP cache GC, TCP keepalive, and NTP
  * resync logic.
  *
  * @param  now_ms  Current monotonic time in milliseconds.
  */
 void uiox_net_tick(uint32_t now_ms);
 
 /* =========================================================================
  * DHCP client API
  * ====================================================================== */
 
 /**
  * @brief  Run DHCP DISCOVER → OFFER → REQUEST → ACK on the given interface.
  * @param  netif      Interface to configure.
  * @param  lease_out  Filled with lease details on success (may be NULL).
  * @return 0 on success, -ETIMEDOUT if no server responds.
  */
 int  uiox_dhcp_request(uiox_netif_t *netif, uiox_dhcp_lease_t *lease_out);
 
 /**
  * @brief  Send DHCP RELEASE and clear the interface IP address.
  */
 void uiox_dhcp_release(uiox_netif_t *netif);
 
 /**
  * @brief  Renew an existing DHCP lease (DHCPREQUEST to the known server).
  * @return 0 on success, negative errno on failure.
  */
 int  uiox_dhcp_renew  (uiox_netif_t *netif, uiox_dhcp_lease_t *lease_out);
 
 /**
  * @brief  Return a copy of the current active lease.
  * @return false if no active lease.
  */
 bool uiox_dhcp_lease  (uiox_netif_t *netif, uiox_dhcp_lease_t *out);
 
 /* =========================================================================
  * DNS resolver API
  * ====================================================================== */
 
 /**
  * @brief  Configure DNS servers (overrides DHCP-supplied servers).
  */
 void uiox_dns_set_servers(const uint32_t *servers, uint8_t count);
 
 /**
  * @brief  Resolve a hostname to IPv4 addresses.
  * @param  hostname  Null-terminated hostname string.
  * @param  result    Filled with resolved addresses and TTL.
  * @return 0 on success, -ENOENT if name not found, -ETIMEDOUT on timeout.
  */
 int  uiox_dns_resolve(const char *hostname, uiox_dns_result_t *result);
 
 /**
  * @brief  Convenience: resolve hostname and return first IPv4 address.
  * @return Resolved IPv4 (host order), 0 on failure.
  */
 uint32_t uiox_dns_resolve_first(const char *hostname);
 
 /**
  * @brief  Flush the internal DNS cache.
  */
 void uiox_dns_cache_flush(void);
 
 /* =========================================================================
  * NTP client API
  * ====================================================================== */
 
 /**
  * @brief  Query an NTP server and return time information.
  * @param  server_ip  NTP server IPv4 (host order).
  * @param  result     Filled with time data on success (may be NULL).
  * @return 0 on success, negative errno on failure.
  */
 int  uiox_ntp_sync(uint32_t server_ip, uiox_ntp_result_t *result);
 
 /**
  * @brief  Return the last successfully synchronised Unix timestamp.
  * @return Seconds since Unix epoch, 0 if never synced.
  */
 uint32_t uiox_ntp_last_sync(void);
 
 /* =========================================================================
  * HTTP client API
  * ====================================================================== */
 
 /**
  * @brief  Execute an HTTP request (synchronous, blocking).
  *
  * Resolves the hostname in the URL via DNS, opens a TCP connection,
  * sends the request, and reads the response into resp->body.
  *
  * @param  req   Populated request descriptor.
  * @param  resp  Response descriptor; caller must set body + body_capacity.
  * @return 0 on success (check resp->status_code), negative errno on error.
  */
 int  uiox_http_request(const uiox_http_request_t *req,
                         uiox_http_response_t      *resp);
 
 /**
  * @brief  Convenience GET — resolves, connects, fetches, closes.
  * @param  url          Null-terminated URL string.
  * @param  buf          Caller-supplied response body buffer.
  * @param  buf_len      Size of buf in bytes.
  * @param  status_out   HTTP status code (may be NULL).
  * @return Bytes written into buf, or negative errno.
  */
 ssize_t uiox_http_get(const char *url, void *buf, size_t buf_len,
                        int *status_out);
 
 /**
  * @brief  Convenience POST — posts body, returns response.
  */
 ssize_t uiox_http_post(const char *url,
                         const void *body,    size_t body_len,
                         void       *resp_buf, size_t resp_buf_len,
                         int        *status_out);
 
 /* =========================================================================
  * Ping / reachability API
  * ====================================================================== */
 
 /**
  * @brief  Send a single ICMP echo request and wait for reply.
  * @param  target_ip   Destination IPv4 (host order).
  * @param  timeout_ms  How long to wait for reply.
  * @param  result      Filled with RTT and reachability on success.
  * @return 0 on success (target reachable), -ETIMEDOUT if no reply.
  */
 int  uiox_ping(uint32_t target_ip, uint32_t timeout_ms,
                 uiox_ping_result_t *result);
 
 /**
  * @brief  Convenience: check if a host is reachable.
  * @return true if ping succeeded within timeout.
  */
 bool uiox_reachable(uint32_t target_ip, uint32_t timeout_ms);
 
 /* =========================================================================
  * Utility: IP address helpers
  * ====================================================================== */
 
 /**
  * @brief  Convert dotted-decimal string to host-order uint32_t.
  * @return Parsed IPv4, or 0 on parse error.
  */
 uint32_t uiox_ip4_from_str(const char *str);
 
 /**
  * @brief  Convert host-order uint32_t to dotted-decimal string.
  * @param  buf   Caller buffer, must be at least 16 bytes.
  */
 void uiox_ip4_to_str(uint32_t ip, char *buf, size_t buf_len);
 
 #ifdef __cplusplus
 }
 #endif
 
 #endif /* UIOX_CONNECTIVITY_H */
 