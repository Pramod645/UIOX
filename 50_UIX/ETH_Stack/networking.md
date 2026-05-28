uiox/
├── include/
│   ├── uiox_net_hw.h          # Layer 1  — HAL: DMA descriptors, ops vtable,
│   │                          #             PHY link management
│   ├── uiox_netbuf.h          # Layer 2a — Zero-copy packet buffer pool
│   ├── uiox_netif.h           # Layer 2b — Netif registration, ARP, Ethernet
│   │                          #             framing, IF statistics
│   ├── uiox_proto.h           # Layer 3/4 — IPv4/IPv6 headers, UDP, TCP,
│   │                          #             ICMP, ARP wire format, routing
│   ├── uiox_socket.h          # Layer 4  — BSD socket API, TCP state machine,
│   │                          #             RX queue, sockaddr types
│   └── uiox_connectivity.h    # Layer 7  — DHCP, DNS, NTP, HTTP, ping,
│                              #             stack lifecycle, event callbacks
└── src/
    ├── uiox_net_hw.c          # HAL lifecycle (init, up, down, link query)
    ├── uiox_netbuf.c          # Buffer pool alloc/free, push/pull/put/trim,
    │                          #   chain helpers
    ├── uiox_netif.c           # Netif register/unregister, RX dispatch,
    │                          #   ARP cache, IF stats
    ├── uiox_proto.c           # IPv4 RX/TX, UDP, TCP, ICMP, ARP DORA,
    │                          #   routing table, Internet checksum
    ├── uiox_socket.c          # socket/bind/listen/accept/connect/
    │                          #   send/recv/setsockopt/close + deliver hook
    ├── uiox_connectivity.c    # DHCP client, DNS resolver, NTP (SNTPv4),
    │                          #   HTTP/1.1 client, ping, IP string utils
    └── uiox_net_demo.c        # End-to-end demo: DHCP→DNS→ping→HTTP→NTP

Key Design Decisions
Decision,Rationale
Static buffer pool,No heap fragmentation — safe for RTOS/bare-metal targets
Zero-copy push/pull,"Headers prepended/stripped by moving data pointer, not copying"
Vtable ops pattern,"Concrete NIC drivers (GMAC, EMAC, WiFi) plug in without modifying upper layers"
Longest-prefix routing,Correct multi-interface route selection with metric tiebreaking
DHCP DORA over raw UDP,Full discover→offer→request→ack with option parsing for mask/GW/DNS/TTL
SNTPv4 (RFC 4330),"Minimal 48-byte NTP packet — no floating point, no kernel clock adjustment"
HTTP/1.1 Connection: close,Stateless — no persistent connection pool needed for embedded use
TCP state machine,SYN/SYN-ACK/ACK/FIN/RST tracked per socket for correct half-close
Event callback system,Application reacts to link/IP/DNS/NTP events without polling
POSIX-compatible socket API,uiox_socket() / uiox_bind() etc. mirror POSIX so porting existing code is minimal



Usage Reference
Command,Effect
make,Native debug build (auto-detects platform)
make BUILD=release,"Optimised, stripped release build"
make PLATFORM=ARM64 CROSS=aarch64-linux-gnu-,Cross-compile for ARM64
make PLATFORM=ARM32 CROSS=arm-linux-gnueabihf-,Cross-compile for ARM32
make lib,Build static library only
make demo,Build demo binary (depends on lib)
make install PREFIX=/opt/uiox,Install headers + lib + binary
make uninstall PREFIX=/opt/uiox,Remove installed files
make size,Print text/data/bss breakdown
make dump,Generate disassembly into build/.../uiox_net_demo.asm
make lint,Run cppcheck static analysis
make format,Auto-format all sources with clang-format
make docs,Generate Doxygen HTML docs
make tags,Generate ctags index for editor navigation
make info,Print full build configuration summary
make clean,Remove current platform/mode build artefacts
make distclean,Remove entire build/ tree
make V=1,Verbose — print full compiler commands