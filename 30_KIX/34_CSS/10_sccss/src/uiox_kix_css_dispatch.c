/*
 * 34_CSS/10_sccss/src/uiox_kix_css_dispatch.c
 *
 * SCCSS dispatcher — CSS numbers to CSS calls.
 *
 *   called by : 40_SystemCallInterface/uix_archSysCall.c
 *               (through syscall_dispatch in 30_KIX/common/uiox_kix_router.c)
 *   calls     : the uiox_kix_css_* entry points the CSS MODULES own —
 *               one set per class under 01_Com / 02_Sensors /
 *               03_NonSensors, each in its own directory
 *
 * ── numbering ───────────────────────────────────────────────────────
 * The numbers are the BSD ones from 50_UIX's uix_sys.h, NOT numbers
 * invented here.  SCiX decides which subsystem owns a number; this file
 * decides which FUNCTION within the subsystem.
 *
 * That split keeps each layer's table small: SCiX knows only "97 belongs
 * to CSS", and this file knows "97 is socket".  A number added to a
 * class is one line here and one line in SCiX's ownership test, and
 * neither has to learn the other's table.
 *
 * ── what this file is, and is NOT ───────────────────────────────────
 * It is a ROUTER.  It knows a number and a function pointer, and it
 * holds no behaviour of its own.
 *
 * It is NOT where a class lives.  A socket, a Bluetooth ACL send, a
 * camera frame dequeue, a speaker stream — each of those is a CLASS
 * with its own state, its own structs, its own flags and its own header,
 * and each belongs in its own module directory beside the driver it
 * drives:
 *
 *   01_Com/eth/            socket, protocol, netif bindings
 *   01_Com/bt/             HCI, ACL, GATT
 *   01_Com/usb/            USB class registration
 *   02_Sensors/camera/     frame queues, CSI-2
 *   02_Sensors/mic/        PCM capture
 *   03_NonSensors/speaker/ playback streams
 *
 * A struct in this file would be a struct every other class had to
 * share a header with, and a flag here would be a flag that meant one
 * thing to one class and something else to another.  That is the
 * boundary this file exists to keep: ONE table, N class headers.
 *
 * ── the bad-address/unsupported rules, held in one place ─────────────
 * The two codes this file can produce on its own are EINVAL and ENOSYS,
 * and they mean different things:
 *
 *   EINVAL  the number names no call in this subsystem at all
 *   ENOSYS  the number names a REAL call with no implementation yet
 *
 * Collapsing them would erase the only signal separating "you asked for
 * nothing" from "not finished yet".
 *
 * @version 1.0.0  @date 2026-11-20
 */

 #include "uiox_base_types.h"
 #include "uiox_kix_sccss_numbers.h"  /* the BSD numbers, local         */
 #include "uiox_kix_sccss.h"          /* SCCSS_EINVAL / _ENOSYS, the
                                       * entry struct, and the boundary
                                       * this file sits under          */
 #include "uiox_socket.h"
 #include "uiox_kix_css_wifi.h"    /* WiFi's four entry points */
 #include "uiox_kix_css_bt.h"      /* BT's four   */
 #include "uiox_kix_css_mic.h"
 #include "uiox_kix_css_spk.h"     /* beside the WiFi/BT/MIC includes */
 #include "uiox_kix_css_cam.h"     /* beside the WiFi/BT/MIC/SPK includes */
 #include "uiox_kix_css_hdmi.h"    /* beside the WiFi/BT/MIC/SPK/CAM includes */
 #include "uiox_kix_css_mon.h"     /* beside the HDMI include */
 #include "uiox_kix_css_gpu.h"     /* beside the HDMI/MON includes */


 /* ── Return convention ──────────────────────────────────────────────
  * Negative is an error, matching the file and process sides so SCiX can
  * pass a value through without asking which subsystem produced it.
  *
  * The two values come from uiox_kix_sccss.h, included above, and are
  * commented here only to show where an override would go — 10_scfs does
  * the same with SCFS_ENOSYS / SCFS_EINVAL. */
 //#define SCCSS_ENOSYS   ((uiox_int64_t)-38)
 //#define SCCSS_EINVAL   ((uiox_int64_t)-22)
 
 /* ── ONE ROW PER NUMBER, AND EVERY ROW IS A MODULE'S ─────────────────
  * The rows are added as each class is written.  A class's entry points
  * live in that class's own directory, and its row here names them.  No
  * row's body is in this file, and no row's types are in this file.
  *
  * The table is empty at this version on purpose: it is the frame, not
  * the content.  A number nobody has claimed yet has no name and returns
  * EINVAL — which is correct, because no CSS class owns it yet.
  *
  * The eth socket rows are shown COMMENTED, as the shape a row takes
  * once that class's header exists.  Uncomment them when 01_Com/eth
  * declares the eight entry points and they are implemented; until then
  * a live row would be a link error, and a table that cannot link is
  * worse than a table that answers EINVAL. */
 static const uiox_kix_css_entry_t uiox_kix_css_table[SCCSS_MAX] =
 {
     /* ── 01_Com/eth ─────────────────────────────────────────────────
      * Socket calls.  Entry points are declared by the eth module's own
      * header; this file only takes their address.
      */
     [SYS_SOCKET]   = { (uiox_kix_css_fn_t)(void *)uiox_socket,   "socket"   },
     [SYS_BIND]     = { (uiox_kix_css_fn_t)(void *)uiox_bind,     "bind"     },
     [SYS_CONNECT]  = { (uiox_kix_css_fn_t)(void *)uiox_connect,  "connect"  },
     [SYS_LISTEN]   = { (uiox_kix_css_fn_t)(void *)uiox_listen,   "listen"   },
     [SYS_ACCEPT]   = { (uiox_kix_css_fn_t)(void *)uiox_accept,   "accept"   },
     [SYS_SENDTO]   = { (uiox_kix_css_fn_t)(void *)uiox_sendto,   "sendto"   },
     [SYS_RECVFROM] = { (uiox_kix_css_fn_t)(void *)uiox_recvfrom, "recvfrom" },
     [SYS_SHUTDOWN] = { (uiox_kix_css_fn_t)(void *)uiox_shutdown, "shutdown" },
     [SYS_SENDMSG]    = { (uiox_kix_css_fn_t)0, "sendmsg"    },
     [SYS_RECVMSG]    = { (uiox_kix_css_fn_t)0, "recvmsg"    },
     [SYS_SOCKETPAIR] = { (uiox_kix_css_fn_t)0, "socketpair" },

     /* ── 02_Sensors/WiFi ───────────────────────────────────────────────────
     * Entry points defined in 02_Sensors/WiFi/uiox_kix_css_wifi.c.
     *
     * The numbers are CSS-PRIVATE (400-403), not BSD's — 802.11 has no BSD
     * syscall vocabulary.  See uiox_kix_sccss_numbers.h for the block and
     * why it sits at 400. */
     [SYS_WIFI_SCAN]       = { (uiox_kix_css_fn_t)(void *)uiox_kix_css_wifi_scan,       "wifi_scan"       },
     [SYS_WIFI_DISCONNECT] = { (uiox_kix_css_fn_t)(void *)uiox_kix_css_wifi_disconnect, "wifi_disconnect" },
     [SYS_WIFI_QUALITY]    = { (uiox_kix_css_fn_t)(void *)uiox_kix_css_wifi_quality,    "wifi_quality"    },
     [SYS_WIFI_STATE]      = { (uiox_kix_css_fn_t)(void *)uiox_kix_css_wifi_state,      "wifi_state"      },
    
     /* ── 01_Com/bt ──────────────────────────────────────────────────
      * Bluetooth.  No vocabulary exists in the kernel for this class —
      * the ACL/HCI path is reached through uiox_bt_if, not a device
      * node.  Rows land here if and when that changes.
      */
     /* ── 01_Com/bt — CSS-PRIVATE numbers (405-408) ─────────────────────────
     * Entry points defined in 01_Com/bt/uiox_kix_css_bt.c.  Bluetooth has
     * no BSD syscall vocabulary, so these are borrowed from the block this
     * kernel marks as its own — see uiox_kix_sccss_numbers.h. */
     [SYS_BT_SCAN_START] = { (uiox_kix_css_fn_t)(void *)uiox_kix_css_bt_scan_start, "bt_scan_start" },
     [SYS_BT_SCAN_STOP]  = { (uiox_kix_css_fn_t)(void *)uiox_kix_css_bt_scan_stop,  "bt_scan_stop"  },
     [SYS_BT_DISCONNECT] = { (uiox_kix_css_fn_t)(void *)uiox_kix_css_bt_disconnect, "bt_disconnect" },
     [SYS_BT_STATE]      = { (uiox_kix_css_fn_t)(void *)uiox_kix_css_bt_state,      "bt_state"      },
     /* ── 02_Sensors/mic — CSS-PRIVATE numbers (409-412) ───────────────────── */
     [SYS_MIC_READ]  = { (uiox_kix_css_fn_t)(void *)uiox_kix_css_mic_read,  "mic_read"  },
     [SYS_MIC_GAIN]  = { (uiox_kix_css_fn_t)(void *)uiox_kix_css_mic_gain,  "mic_gain"  },
     [SYS_MIC_MUTE]  = { (uiox_kix_css_fn_t)(void *)uiox_kix_css_mic_mute,  "mic_mute"  },
     [SYS_MIC_STATE] = { (uiox_kix_css_fn_t)(void *)uiox_kix_css_mic_state, "mic_state" },
     /* ── 03_NonSensors/speaker — CSS-PRIVATE numbers (413-416) ────────────── */
     [SYS_SPK_WRITE]  = { (uiox_kix_css_fn_t)(void *)uiox_kix_css_spk_write,  "spk_write"  },
     [SYS_SPK_VOLUME] = { (uiox_kix_css_fn_t)(void *)uiox_kix_css_spk_volume, "spk_volume" },
     [SYS_SPK_MUTE]   = { (uiox_kix_css_fn_t)(void *)uiox_kix_css_spk_mute,   "spk_mute"   },
     [SYS_SPK_STATE]  = { (uiox_kix_css_fn_t)(void *)uiox_kix_css_spk_state,  "spk_state"  },
     /* ── 02_Sensors/camera — CSS-PRIVATE numbers (417-420) ────────────────── */
     [SYS_CAM_STATE]    = { (uiox_kix_css_fn_t)(void *)uiox_kix_css_cam_state,    "cam_state"    },
     [SYS_CAM_EXPOSURE] = { (uiox_kix_css_fn_t)(void *)uiox_kix_css_cam_exposure, "cam_exposure" },
     [SYS_CAM_GAIN]     = { (uiox_kix_css_fn_t)(void *)uiox_kix_css_cam_gain,     "cam_gain"     },
     [SYS_CAM_STREAM]   = { (uiox_kix_css_fn_t)(void *)uiox_kix_css_cam_stream,   "cam_stream"   },
     /* ── 01_Com/hdmi — CSS-PRIVATE numbers (421-424) ──────────────────────── */
     [SYS_HDMI_STATE]    = { (uiox_kix_css_fn_t)(void *)uiox_kix_css_hdmi_state,    "hdmi_state"    },
     [SYS_HDMI_DPMS]     = { (uiox_kix_css_fn_t)(void *)uiox_kix_css_hdmi_dpms,     "hdmi_dpms"     },
     [SYS_HDMI_ACTIVITY] = { (uiox_kix_css_fn_t)(void *)uiox_kix_css_hdmi_activity, "hdmi_activity" },
     [SYS_HDMI_MODE]     = { (uiox_kix_css_fn_t)(void *)uiox_kix_css_hdmi_mode,     "hdmi_mode"     },
     /* ── 01_Com/monitor — CSS-PRIVATE numbers (425-428) ───────────────────── */
     [SYS_MON_STATE]    = { (uiox_kix_css_fn_t)(void *)uiox_kix_css_mon_state,    "mon_state"    },
     [SYS_MON_DPMS]     = { (uiox_kix_css_fn_t)(void *)uiox_kix_css_mon_dpms,     "mon_dpms"     },
     [SYS_MON_FPS]      = { (uiox_kix_css_fn_t)(void *)uiox_kix_css_mon_fps,      "mon_fps"      },
     [SYS_MON_ACTIVITY] = { (uiox_kix_css_fn_t)(void *)uiox_kix_css_mon_activity, "mon_activity" },
         /* ── 01_Com/gpu — CSS-PRIVATE numbers (429-432) ────────────────────── */
     [SYS_GPU_STATE] = { (uiox_kix_css_fn_t)(void *)uiox_kix_css_gpu_state, "gpu_state" },
     [SYS_GPU_STATS] = { (uiox_kix_css_fn_t)(void *)uiox_kix_css_gpu_stats, "gpu_stats" },
     [SYS_GPU_PSO]   = { (uiox_kix_css_fn_t)(void *)uiox_kix_css_gpu_pso,   "gpu_pso"   },
     [SYS_GPU_FRAME] = { (uiox_kix_css_fn_t)(void *)uiox_kix_css_gpu_frame, "gpu_frame" },

 
     /* ── 01_Com/usb · 02_Sensors/camera, mic · 03_NonSensors/speaker ─
      * Same position: class headers, class directories, rows added per
      * class as each is written.
      */
 };
 
 /* ────────────────────────────────────────────────────────────────────
  * uiox_kix_css_dispatch — one CSS number to one class's call.
  *
  * Returns the raw uiox_int64_t.  Where a register context is supplied
  * the value is written back with the same convention the file and
  * process sides use — positive error number in rc_r0 with rc_carry set —
  * so SCiX can return any subsystem's result without knowing which
  * produced it.
  *
  * The range this table spans is 0..SCCSS_MAX.  A number inside that
  * range with no name belongs to no CSS class and this file answers
  * EINVAL for it.
  * ──────────────────────────────────────────────────────────────────── */
 uiox_int64_t uiox_kix_css_dispatch(uiox_uint64_t  nr,
                                    uiox_uintptr_t a0, uiox_uintptr_t a1,
                                    uiox_uintptr_t a2, uiox_uintptr_t a3,
                                    uiox_uintptr_t a4, uiox_uintptr_t a5,
                                    void          *regs)
 {
     const uiox_kix_css_entry_t *e;
     uiox_int64_t                rv;
 
     /* Out of range: this table cannot speak for it.  EINVAL rather than
      * ENOSYS, because the number is outside the BSD space this table
      * models at all. */
     if (nr >= (uiox_uint64_t)SCCSS_MAX)
         return SCCSS_EINVAL;
 
     e = &uiox_kix_css_table[nr];
 
     /* A slot with no name is a number this SUBSYSTEM does not own.
      * SCiX should have routed it elsewhere, so this is a routing bug
      * rather than a missing implementation — hence EINVAL, and hence
      * the name test rather than the function test. */
     if (e->name == (const char *)0)
         return SCCSS_EINVAL;
 
     /* A named slot with no function is a real BSD syscall this kernel
      * has not implemented.  ENOSYS: the number is valid, the body is
      * not written, and a caller must be able to tell the two apart. */
     if (e->fn == (uiox_kix_css_fn_t)0)
         return SCCSS_ENOSYS;
 
     rv = e->fn(a0, a1, a2, a3, a4, a5);
 
     if (regs) {
         /* The reg context type is 40_psa's.  Referred to through a
          * local mirror so this file needs no 33_PCS include path — the
          * three fields and the flag are all it touches, and they are
          * the same ones the process and file dispatchers write.
          *
          * ⚠ The layout MUST match 40_psa's on EVERY target.  rc_gpr[16]
          * is 128 bytes on the 64-bit arches and 64 on the 32-bit ones,
          * so rc_r0's offset differs per architecture — it follows
          * rc_gpr, and rc_gpr's size follows uiox_uintptr_t.  This holds
          * only while the real struct has the same field order. */
         struct sccss_reg_view {
             uiox_uintptr_t rc_pc;
             uiox_uintptr_t rc_sp;
             uiox_uint32_t  rc_sr;
             uiox_uint32_t  rc_carry;
             uiox_uintptr_t rc_gpr[16];
             uiox_uint32_t  rc_r0;
             uiox_uint32_t  rc_r1;
         } *rc = (struct sccss_reg_view *)regs;
 
         if (rv < 0) {
             rc->rc_r0    = (uiox_uint32_t)(-rv);
             rc->rc_r1    = 0u;
             rc->rc_carry = 1u;
         } else {
             rc->rc_r0    = (uiox_uint32_t)((uiox_uint64_t)rv & 0xFFFFFFFFu);
             rc->rc_r1    = (uiox_uint32_t)((uiox_uint64_t)rv >> 32u);
             rc->rc_carry = 0u;
         }
     }
     return rv;
 }
 
 /* ── Introspection ────────────────────────────────────────────────────
  * Same shape as uiox_kix_scfs_name().  Declared in uiox_kix_sccss.h so
  * a diagnostic can call it without a second extern. */
 const char *uiox_kix_css_name(uiox_uint64_t nr)
 {
     if (nr >= (uiox_uint64_t)SCCSS_MAX) return (const char *)0;
     return uiox_kix_css_table[nr].name;
 }
 