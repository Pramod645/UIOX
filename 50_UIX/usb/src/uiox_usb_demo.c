/**
 * @file    uiox_usb_demo.c
 * @brief   UIOX USB stack end-to-end demonstration.
 *
 * Demonstrates CDC (virtual COM) + HID (keyboard) composite device:
 *   HAL init → EP config → enumeration → SET_CONFIGURATION →
 *   class bind → HID report TX → CDC data TX/RX → suspend/resume.
 *
 * @date    2026-05-28
 */
//Demo Application
 #include "uiox_usb_device.h"
 #include "uiox_usb_class.h"
 #include <stdio.h>
 #include <string.h>
 
 /* =========================================================================
  * USB descriptors — CDC + HID composite
  * ====================================================================== */
 
  static const uiox_usb_dev_desc_t s_dev_desc = {
    .bLength            = sizeof(uiox_usb_dev_desc_t),
    .bDescriptorType    = UIOX_USB_DT_DEVICE,
    .bcdUSB             = 0x0200u,  /* USB 2.0                             */
    .bDeviceClass       = 0xEFu,    /* Misc (IAD composite)                */
    .bDeviceSubClass    = 0x02u,
    .bDeviceProtocol    = 0x01u,
    .bMaxPacketSize0    = 64u,
    .idVendor           = 0x1234u,  /* Demo VID                            */
    .idProduct          = 0x5678u,  /* Demo PID                            */
    .bcdDevice          = 0x0100u,
    .iManufacturer      = 1u,
    .iProduct           = 2u,
    .iSerialNumber      = 3u,
    .bNumConfigurations = 1u,
};

/* HID keyboard report descriptor (boot-compatible, 8-byte report) */
static const uint8_t s_hid_report_desc[] = {
    0x05,0x01,  /* Usage Page: Generic Desktop           */
    0x09,0x06,  /* Usage: Keyboard                       */
    0xA1,0x01,  /* Collection: Application               */
    0x05,0x07,  /* Usage Page: Keyboard/Keypad           */
    0x19,0xE0,  /* Usage Minimum: Left Ctrl              */
    0x29,0xE7,  /* Usage Maximum: Right GUI              */
    0x15,0x00,  /* Logical Minimum: 0                    */
    0x25,0x01,  /* Logical Maximum: 1                    */
    0x75,0x01,  /* Report Size: 1                        */
    0x95,0x08,  /* Report Count: 8 (modifier bits)       */
    0x81,0x02,  /* Input: Data, Variable, Absolute       */
    0x95,0x01,  /* Report Count: 1 (reserved byte)       */
    0x75,0x08,  /* Report Size: 8                        */
    0x81,0x01,  /* Input: Constant                       */
    0x95,0x06,  /* Report Count: 6 (key array)           */
    0x75,0x08,  /* Report Size: 8                        */
    0x15,0x00,  /* Logical Minimum: 0                    */
    0x25,0x65,  /* Logical Maximum: 101                  */
    0x05,0x07,  /* Usage Page: Keyboard                  */
    0x19,0x00,  /* Usage Minimum: 0                      */
    0x29,0x65,  /* Usage Maximum: 101                    */
    0x81,0x00,  /* Input: Data, Array                    */
    0xC0        /* End Collection                        */
};

/* Combined CDC + HID configuration descriptor */
static const uint8_t s_cfg_desc[] = {
    /* Configuration descriptor */
    0x09, UIOX_USB_DT_CONFIG,
    0x6B, 0x00,  /* wTotalLength = 107 bytes              */
    0x03,        /* bNumInterfaces: 3 (CDC ctrl+data, HID)*/
    0x01,        /* bConfigurationValue                   */
    0x04,        /* iConfiguration                        */
    0xC0,        /* bmAttributes: self-powered            */
    0x32,        /* bMaxPower: 100 mA                     */

    /* --- CDC Interface Association Descriptor --- */
    0x08, 0x0Bu, 0x00, 0x02, 0x02, 0x02, 0x01, 0x05,

    /* CDC Control interface */
    0x09, UIOX_USB_DT_INTERFACE,
    0x00, 0x00,  /* bInterfaceNumber=0, bAlternateSetting=0 */
    0x01,        /* bNumEndpoints: 1 (notify)             */
    0x02,        /* bInterfaceClass: CDC                  */
    0x02,        /* bInterfaceSubClass: ACM               */
    0x01,        /* bInterfaceProtocol: AT commands       */
    0x05,        /* iInterface                            */

    /* CDC Header functional descriptor */
    0x05, 0x24, 0x00, 0x10, 0x01,
    /* CDC ACM functional descriptor */
    0x04, 0x24, 0x02, 0x02,
    /* CDC Union functional descriptor */
    0x05, 0x24, 0x06, 0x00, 0x01,

    /* CDC Notify EP (EP1 IN, Interrupt) */
    0x07, UIOX_USB_DT_ENDPOINT,
    0x81,        /* EP1 IN                                */
    0x03,        /* Interrupt                             */
    0x10, 0x00,  /* wMaxPacketSize: 16                    */
    0x0A,        /* bInterval: 10 ms                      */

    /* CDC Data interface */
    0x09, UIOX_USB_DT_INTERFACE,
    0x01, 0x00,  /* bInterfaceNumber=1                    */
    0x02,        /* bNumEndpoints: 2                      */
    0x0A,        /* bInterfaceClass: CDC-Data             */
    0x00, 0x00, 0x06,

    /* CDC Bulk OUT (EP2 OUT) */
    0x07, UIOX_USB_DT_ENDPOINT,
    0x02,        /* EP2 OUT                               */
    0x02,        /* Bulk                                  */
    0x40, 0x00,  /* wMaxPacketSize: 64                    */
    0x00,

    /* CDC Bulk IN (EP2 IN) */
    0x07, UIOX_USB_DT_ENDPOINT,
    0x82,        /* EP2 IN                                */
    0x02,        /* Bulk                                  */
    0x40, 0x00,  /* wMaxPacketSize: 64                    */
    0x00,

    /* --- HID interface --- */
    0x09, UIOX_USB_DT_INTERFACE,
    0x02, 0x00,  /* bInterfaceNumber=2                    */
    0x01,        /* bNumEndpoints: 1                      */
    UIOX_USB_CLASS_HID,
    UIOX_HID_SUBCLASS_BOOT,
    UIOX_HID_PROTO_KEYBOARD,
    0x07,        /* iInterface                            */

    /* HID descriptor */
    0x09, UIOX_USB_DT_HID,
    0x11, 0x01,  /* bcdHID: 1.11                          */
    0x00,        /* bCountryCode: not localised           */
    0x01,        /* bNumDescriptors: 1                    */
    UIOX_USB_DT_REPORT,
    (uint8_t)sizeof(s_hid_report_desc), 0x00,

    /* HID Interrupt IN (EP3 IN) */
    0x07, UIOX_USB_DT_ENDPOINT,
    0x83,        /* EP3 IN                                */
    0x03,        /* Interrupt                             */
    0x08, 0x00,  /* wMaxPacketSize: 8                     */
    0x01,        /* bInterval: 1 ms                       */
};

/* =========================================================================
 * Stub HAL ops
 * ====================================================================== */

static int stub_init(uiox_usb_hw_t *hw)
{
    (void)hw;
    printf("  [hal] init  DWC2  base=0x%08lX  irq=%u\n",
           (unsigned long)hw->base_addr, hw->irq);
    hw->connected = true;
    hw->speed     = UIOX_USB_SPEED_HIGH;
    return 0;
}

static void stub_deinit (uiox_usb_hw_t *hw) { (void)hw; printf("  [hal] deinit\n"); }
static int  stub_start  (uiox_usb_hw_t *hw) { (void)hw; printf("  [hal] start\n");  return 0; }
static void stub_stop   (uiox_usb_hw_t *hw) { (void)hw; printf("  [hal] stop\n");   }

static int stub_set_role(uiox_usb_hw_t *hw, uiox_usb_role_t role)
{
    (void)hw;
    static const char *names[] = {"NONE","HOST","DEVICE","OTG"};
    printf("  [hal] role → %s\n", names[role]);
    return 0;
}

static int stub_set_address(uiox_usb_hw_t *hw, uint8_t addr)
{
    (void)hw;
    printf("  [hal] USB address → %u\n", addr);
    return 0;
}

static int stub_ep_config(uiox_usb_hw_t *hw, uint8_t ep_addr,
                           uiox_usb_ep_type_t type, uint16_t mps,
                           uint8_t interval)
{
    (void)hw;
    static const char *types[] = {"CTRL","ISOC","BULK","INTR"};
    printf("  [hal] EP config  addr=0x%02X  type=%-4s  mps=%u  interval=%u\n",
           ep_addr, types[type], mps, interval);
    return 0;
}

static int stub_ep_disable(uiox_usb_hw_t *hw, uint8_t ep_addr)
{ (void)hw; printf("  [hal] EP disable  addr=0x%02X\n", ep_addr); return 0; }

static int stub_ep_stall(uiox_usb_hw_t *hw, uint8_t ep_addr, bool stall)
{
    (void)hw;
    printf("  [hal] EP %s  addr=0x%02X\n",
           stall ? "STALL" : "UNSTALL", ep_addr);
    return 0;
}

static int stub_ep_flush(uiox_usb_hw_t *hw, uint8_t ep_addr)
{ (void)hw; (void)ep_addr; return 0; }

static uint32_t s_tx_count = 0;
static int stub_tx_submit(uiox_usb_hw_t *hw, uint8_t ep_addr,
                           uintptr_t phys, uint32_t len)
{
    (void)hw; (void)phys;
    printf("  [hal] TX  EP=0x%02X  len=%u  (#%u)\n",
           ep_addr, len, ++s_tx_count);
    return 0;
}

static int stub_rx_submit(uiox_usb_hw_t *hw, uint8_t ep_addr,
                           uintptr_t phys, uint32_t len)
{
    (void)hw; (void)phys;
    printf("  [hal] RX prime  EP=0x%02X  len=%u\n", ep_addr, len);
    return 0;
}

static int stub_tx_complete(uiox_usb_hw_t *hw, uint8_t ep_addr,
                             uint32_t *bytes_done)
{ (void)hw; (void)ep_addr; *bytes_done = 0; return 0; }

static int stub_rx_complete(uiox_usb_hw_t *hw, uint8_t ep_addr,
                             uint32_t *bytes_done)
{ (void)hw; (void)ep_addr; *bytes_done = 0; return 0; }

static bool stub_vbus_sense(uiox_usb_hw_t *hw)
{ (void)hw; return true; }

static int stub_remote_wakeup(uiox_usb_hw_t *hw)
{ (void)hw; printf("  [hal] remote wakeup\n"); return 0; }

static void stub_isr_sof       (uiox_usb_hw_t *hw) { (void)hw; }
static void stub_isr_reset     (uiox_usb_hw_t *hw) { (void)hw; }
static void stub_isr_suspend   (uiox_usb_hw_t *hw) { (void)hw; }
static void stub_isr_resume    (uiox_usb_hw_t *hw) { (void)hw; }
static void stub_isr_ep        (uiox_usb_hw_t *hw, uint8_t ep) { (void)hw; (void)ep; }
static void stub_isr_connect   (uiox_usb_hw_t *hw) { (void)hw; }
static void stub_isr_disconnect(uiox_usb_hw_t *hw) { (void)hw; }

static const uiox_usb_hw_ops_t stub_ops = {
    .init          = stub_init,
    .deinit        = stub_deinit,
    .start         = stub_start,
    .stop          = stub_stop,
    .set_role      = stub_set_role,
    .set_address   = stub_set_address,
    .ep_config     = stub_ep_config,
    .ep_disable    = stub_ep_disable,
    .ep_stall      = stub_ep_stall,
    .ep_flush      = stub_ep_flush,
    .tx_submit     = stub_tx_submit,
    .rx_submit     = stub_rx_submit,
    .tx_complete   = stub_tx_complete,
    .rx_complete   = stub_rx_complete,
    .vbus_sense    = stub_vbus_sense,
    .remote_wakeup = stub_remote_wakeup,
    .isr_sof       = stub_isr_sof,
    .isr_reset     = stub_isr_reset,
    .isr_suspend   = stub_isr_suspend,
    .isr_resume    = stub_isr_resume,
    .isr_ep        = stub_isr_ep,
    .isr_connect   = stub_isr_connect,
    .isr_disconnect= stub_isr_disconnect,
};

/* =========================================================================
 * Hardware device instance — DWC2 USB OTG HS
 * ====================================================================== */

static uiox_usb_hw_t s_hw = {
    .base_addr   = 0x40040000uL,  /* USB OTG HS MMIO (STM32-style)        */
    .irq         = 77,
    .caps        = UIOX_USB_CAP_HOST   |
                   UIOX_USB_CAP_DEVICE |
                   UIOX_USB_CAP_OTG    |
                   UIOX_USB_CAP_FS     |
                   UIOX_USB_CAP_HS     |
                   UIOX_USB_CAP_DMA    |
                   UIOX_USB_CAP_ISO    |
                   UIOX_USB_CAP_LPM    |
                   UIOX_USB_CAP_VBUS_CTRL,
    .role        = UIOX_USB_ROLE_DEVICE,
    .speed       = UIOX_USB_SPEED_UNKNOWN,
    .num_ep      = 8,
};

/* =========================================================================
 * Event callback
 * ====================================================================== */

static void on_usb_event(uiox_usb_evt_t evt, uint8_t ep_addr, void *ctx)
{
    (void)ctx;
    if (evt == UIOX_USB_EVT_EP_COMPLETE && ep_addr == 0u) return;
    printf("  [event] %-14s  ep=0x%02X\n",
           uiox_usb_evt_name(evt), ep_addr);
}

/* =========================================================================
 * CDC RX callback
 * ====================================================================== */

static void on_cdc_rx(const uint8_t *data, uint16_t len, void *ctx)
{
    (void)ctx;
    printf("  [cdc rx] %u bytes: ", len);
    for (uint16_t i = 0; i < len && i < 16u; i++)
        printf("%c", (data[i] >= 0x20u && data[i] < 0x7Fu) ?
               data[i] : '.');
    printf("\n");
}

/* =========================================================================
 * main
 * ====================================================================== */

int main(void)
{
    printf("=== UIOX USB Stack Demo (CDC + HID composite) ===\n\n");

    /* ------------------------------------------------------------------ */
    /* 1. Open device                                                      */
    /* ------------------------------------------------------------------ */

    printf("--- Open ---\n");
    uiox_usb_device_t   dev;
    uiox_usb_open_params_t p;
    memset(&p, 0, sizeof(p));

    p.hw         = &s_hw;
    p.hw_ops     = &stub_ops;
    p.dev_desc   = &s_dev_desc;
    p.cfg_buf    = s_cfg_desc;
    p.cfg_len    = (uint16_t)sizeof(s_cfg_desc);
    p.evt_cb     = on_usb_event;

    /* String descriptors */
    p.strings[0].idx = 1; p.strings[0].str = "UIOX Project";
    p.strings[1].idx = 2; p.strings[1].str = "CDC+HID Composite";
    p.strings[2].idx = 3; p.strings[2].str = "UIOX-SN-001";
    p.strings[3].idx = 4; p.strings[3].str = "Full Speed Config";
    p.strings[4].idx = 5; p.strings[4].str = "CDC Control";
    p.strings[5].idx = 6; p.strings[5].str = "CDC Data";
    p.strings[6].idx = 7; p.strings[6].str = "HID Keyboard";
    p.num_strings = 7;

    int rc = uiox_usb_open(&dev, &p);
    if (rc < 0) {
        printf("[error] uiox_usb_open failed: %d\n", rc);
        return 1;
    }
    printf("  USB speed : %s\n", uiox_usb_speed_name(s_hw.speed));
    printf("  State     : %s\n", uiox_usb_state_name(dev.subsys.state));
    printf("  VID:PID   : %04X:%04X\n",
           s_dev_desc.idVendor, s_dev_desc.idProduct);

    /* ------------------------------------------------------------------ */
    /* 2. Register class drivers                                           */
    /* ------------------------------------------------------------------ */

    printf("\n--- Register class drivers ---\n");

    /* CDC virtual COM port: EP1 notify, EP2 bulk out/in */
    static uiox_usb_cdc_t s_cdc;
    uiox_usb_cdc_init(&s_cdc, 0x82u, 0x02u, 0x81u);
    s_cdc.on_rx  = on_cdc_rx;
    s_cdc.rx_ctx = NULL;
    uiox_usb_register_class(&dev, &s_cdc.base);
    printf("  CDC registered  EP_IN=0x82  EP_OUT=0x02  EP_NOTIFY=0x81\n");

    /* HID keyboard: EP3 interrupt in */
    static uiox_usb_hid_t s_hid;
    uiox_usb_hid_init(&s_hid, 0x83u,
                       s_hid_report_desc, sizeof(s_hid_report_desc));
    uiox_usb_register_class(&dev, &s_hid.base);
    printf("  HID registered  EP_IN=0x83  report_desc_len=%zu\n",
           sizeof(s_hid_report_desc));

    /* ------------------------------------------------------------------ */
    /* 3. Start USB controller                                             */
    /* ------------------------------------------------------------------ */

    printf("\n--- Start ---\n");
    rc = uiox_usb_start(&dev);
    printf("  USB controller: ACTIVE  rc=%d\n", rc);

    /* ------------------------------------------------------------------ */
    /* 4. Simulate enumeration sequence                                    */
    /* ------------------------------------------------------------------ */

    printf("\n--- Enumeration simulation ---\n");

    /* 4a. Bus reset */
    printf("\n  [bus reset]\n");
    uiox_usb_inject_reset(&dev);

    /* 4b. GET_DESCRIPTOR Device (host reads 18 bytes) */
    printf("\n  [GET_DESCRIPTOR Device]\n");
    uiox_usb_setup_t setup;
    setup.bmRequestType = UIOX_USB_DIR_DEV_TO_HOST | UIOX_USB_TYPE_STANDARD |
                          UIOX_USB_RECIP_DEVICE;
    setup.bRequest = UIOX_USB_REQ_GET_DESCRIPTOR;
    setup.wValue   = (uint16_t)((UIOX_USB_DT_DEVICE << 8u) | 0u);
    setup.wIndex   = 0;
    setup.wLength  = 18u;
    uiox_usb_inject_setup(&dev, &setup);

    /* 4c. SET_ADDRESS */
    printf("\n  [SET_ADDRESS 5]\n");
    setup.bmRequestType = UIOX_USB_DIR_HOST_TO_DEV | UIOX_USB_TYPE_STANDARD |
                          UIOX_USB_RECIP_DEVICE;
    setup.bRequest = UIOX_USB_REQ_SET_ADDRESS;
    setup.wValue   = 5u;
    setup.wIndex   = 0;
    setup.wLength  = 0;
    uiox_usb_inject_setup(&dev, &setup);

    /* 4d. GET_DESCRIPTOR Configuration */
    printf("\n  [GET_DESCRIPTOR Configuration]\n");
    setup.bmRequestType = UIOX_USB_DIR_DEV_TO_HOST | UIOX_USB_TYPE_STANDARD |
                          UIOX_USB_RECIP_DEVICE;
    setup.bRequest = UIOX_USB_REQ_GET_DESCRIPTOR;
    setup.wValue   = (uint16_t)((UIOX_USB_DT_CONFIG << 8u) | 0u);
    setup.wIndex   = 0;
    setup.wLength  = (uint16_t)sizeof(s_cfg_desc);
    uiox_usb_inject_setup(&dev, &setup);

    /* 4e. GET_DESCRIPTOR String (iManufacturer) */
    printf("\n  [GET_DESCRIPTOR String idx=1]\n");
    setup.bRequest = UIOX_USB_REQ_GET_DESCRIPTOR;
    setup.wValue   = (uint16_t)((UIOX_USB_DT_STRING << 8u) | 1u);
    setup.wLength  = 64u;
    uiox_usb_inject_setup(&dev, &setup);

    /* 4f. SET_CONFIGURATION */
    printf("\n  [SET_CONFIGURATION 1]\n");
    setup.bmRequestType = UIOX_USB_DIR_HOST_TO_DEV | UIOX_USB_TYPE_STANDARD |
                          UIOX_USB_RECIP_DEVICE;
    setup.bRequest = UIOX_USB_REQ_SET_CONFIGURATION;
    setup.wValue   = 1u;
    setup.wIndex   = 0;
    setup.wLength  = 0;
    uiox_usb_inject_setup(&dev, &setup);

    printf("\n  State after enumeration: %s\n",
           uiox_usb_state_name(dev.subsys.state));
    printf("  Connected  : %s\n", uiox_usb_connected(&dev)  ? "YES" : "NO");
    printf("  Configured : %s\n", uiox_usb_configured(&dev) ? "YES" : "NO");

    /* ------------------------------------------------------------------ */
    /* 5. HID keyboard report                                             */
    /* ------------------------------------------------------------------ */

    if (uiox_usb_configured(&dev)) {
        printf("\n--- HID keyboard reports ---\n");

        /* Press 'H' (HID keycode 0x0B) */
        uint8_t hid_report[8] = { 0x00, 0x00, 0x0B, 0,0,0,0,0 };
        rc = uiox_usb_hid_send(&s_hid, hid_report, sizeof(hid_report));
        printf("  HID key 'H' press   rc=%d\n", rc);

        /* Release */
        memset(hid_report, 0, sizeof(hid_report));
        rc = uiox_usb_hid_send(&s_hid, hid_report, sizeof(hid_report));
        printf("  HID key release     rc=%d\n", rc);

        /* Press Shift+'I' → uppercase I */
        hid_report[0] = 0x02u;  /* Left Shift */
        hid_report[2] = 0x0Cu;  /* 'I' */
        rc = uiox_usb_hid_send(&s_hid, hid_report, sizeof(hid_report));
        printf("  HID key Shift+'I'   rc=%d\n", rc);

        /* ------------------------------------------------------------------ */
        /* 6. CDC virtual COM TX                                              */
        /* ------------------------------------------------------------------ */

        printf("\n--- CDC virtual COM TX ---\n");
        const char *hello = "Hello from UIOX USB CDC!\r\n";
        rc = uiox_usb_cdc_write(&s_cdc,
                                  (const uint8_t *)hello,
                                  (uint16_t)strlen(hello));
        printf("  CDC TX '%s'  rc=%d\n", hello, rc);

        const char *json = "{\"device\":\"UIOX\",\"class\":\"CDC\"}\r\n";
        rc = uiox_usb_cdc_write(&s_cdc,
                                  (const uint8_t *)json,
                                  (uint16_t)strlen(json));
        printf("  CDC TX JSON  len=%zu  rc=%d\n", strlen(json), rc);

        /* ------------------------------------------------------------------ */
        /* 7. Class-specific requests                                          */
        /* ------------------------------------------------------------------ */

        printf("\n--- Class-specific requests ---\n");

        /* CDC GET_LINE_CODING */
        setup.bmRequestType = UIOX_USB_DIR_DEV_TO_HOST |
                              UIOX_USB_TYPE_CLASS      |
                              UIOX_USB_RECIP_INTERFACE;
        setup.bRequest = UIOX_CDC_REQ_GET_LINE_CODING;
        setup.wValue   = 0;
        setup.wIndex   = 0;
        setup.wLength  = sizeof(uiox_cdc_line_coding_t);
        uiox_usb_inject_setup(&dev, &setup);
        printf("  CDC GET_LINE_CODING  baud=%u  bits=%u\n",
               s_cdc.line_coding.dwDTERate,
               s_cdc.line_coding.bDataBits);

        /* CDC SET_CONTROL_LINE_STATE (DTR=1, RTS=1) */
        setup.bmRequestType = UIOX_USB_DIR_HOST_TO_DEV |
                              UIOX_USB_TYPE_CLASS      |
                              UIOX_USB_RECIP_INTERFACE;
        setup.bRequest = UIOX_CDC_REQ_SET_CTRL_LINE_STATE;
        setup.wValue   = 0x03u;
        setup.wIndex   = 0;
        setup.wLength  = 0;
        uiox_usb_inject_setup(&dev, &setup);
        printf("  CDC SET_CTRL_LINE_STATE  DTR=%d  RTS=%d\n",
               s_cdc.dtr, s_cdc.rts);

        /* HID GET_IDLE */
        setup.bmRequestType = UIOX_USB_DIR_DEV_TO_HOST |
                              UIOX_USB_TYPE_CLASS      |
                              UIOX_USB_RECIP_INTERFACE;
        setup.bRequest = UIOX_HID_REQ_GET_IDLE;
        setup.wValue   = 0;
        setup.wIndex   = 2;
        setup.wLength  = 1;
        uiox_usb_inject_setup(&dev, &setup);
        printf("  HID GET_IDLE  idle_rate=%u (×4ms)\n", s_hid.idle_rate);

        /* HID SET_IDLE (0 = send only on change) */
        setup.bmRequestType = UIOX_USB_DIR_HOST_TO_DEV |
                              UIOX_USB_TYPE_CLASS      |
                              UIOX_USB_RECIP_INTERFACE;
        setup.bRequest = UIOX_HID_REQ_SET_IDLE;
        setup.wValue   = 0x0000u;
        setup.wIndex   = 2;
        setup.wLength  = 0;
        uiox_usb_inject_setup(&dev, &setup);
        printf("  HID SET_IDLE 0 (send on change only)\n");
    }

    /* ------------------------------------------------------------------ */
    /* 8. Suspend / resume                                                 */
    /* ------------------------------------------------------------------ */

    printf("\n--- Suspend / Resume ---\n");
    uiox_usb_inject_suspend(&dev);
    printf("  State after suspend : %s\n",
           uiox_usb_state_name(dev.subsys.state));

    uiox_usb_inject_resume(&dev);
    printf("  State after resume  : %s\n",
           uiox_usb_state_name(dev.subsys.state));

    /* ------------------------------------------------------------------ */
    /* 9. GET_STATUS                                                       */
    /* ------------------------------------------------------------------ */

    printf("\n--- GET_STATUS ---\n");
    setup.bmRequestType = UIOX_USB_DIR_DEV_TO_HOST |
                          UIOX_USB_TYPE_STANDARD   |
                          UIOX_USB_RECIP_DEVICE;
    setup.bRequest = UIOX_USB_REQ_GET_STATUS;
    setup.wValue   = 0;
    setup.wIndex   = 0;
    setup.wLength  = 2u;
    uiox_usb_inject_setup(&dev, &setup);
    printf("  self_powered=%u  remote_wakeup=%u\n",
           dev.subsys.proto.self_powered,
           dev.subsys.proto.remote_wakeup);

    /* ------------------------------------------------------------------ */
    /* 10. Periodic tick loop                                              */
    /* ------------------------------------------------------------------ */

    printf("\n--- Tick loop (5 × 10 ms) ---\n");
    for (uint32_t t = 10u; t <= 50u; t += 10u) {
        uiox_usb_tick(&dev, t);
        uiox_usb_subsys_sof(&dev.subsys);  /* simulate SOF */
        printf("  tick t=%u ms  frame=%u  state=%s\n",
               t, dev.subsys.frame_num,
               uiox_usb_state_name(dev.subsys.state));
    }

    /* ------------------------------------------------------------------ */
    /* 11. Statistics                                                      */
    /* ------------------------------------------------------------------ */

    printf("\n--- Statistics ---\n");
    uiox_usb_print_stats(&dev);

    /* ------------------------------------------------------------------ */
    /* 12. Stop and close                                                  */
    /* ------------------------------------------------------------------ */

    printf("\n--- Stop and close ---\n");
    uiox_usb_stop(&dev);
    printf("  State : %s\n", uiox_usb_state_name(dev.subsys.state));
    uiox_usb_close(&dev);
    printf("  Device: CLOSED\n");

    printf("\n=== UIOX USB Demo complete ===\n");
    return 0;
}
