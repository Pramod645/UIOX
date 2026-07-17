---
title: ALS — Ambient Light Sensor Hardware Description & Pin Layout
file: 02_FwHal/include/uiox_fw_als.h
date: 2026-07-16
project: UIOX (github.com/Pramod645/UIOX)
---

# ALS — Ambient Light Sensor
## Hardware Description & Pin Layout
### `02_FwHal/include/uiox_fw_als.h`

---

## 1 · Overview

The Ambient Light Sensor (ALS) driver in UIOX supports lux-measurement ICs
connected over the I²C bus. It provides a unified HAL for two common parts:

| Property | VEML7700 | OPT3001 |
| --- | --- | --- |
| **Manufacturer** | Vishay | Texas Instruments |
| **Interface** | I²C | I²C |
| **I²C Address** | 0x10 (ADDR pin LOW) | 0x44–0x47 (selectable) |
| **Supply Voltage** | 2.5 V – 3.6 V | 1.6 V – 3.6 V |
| **Lux Range** | 0 – 120 000 lx | 0.01 – 83 865 lx |
| **Resolution** | 16-bit | 16-bit |
| **Interrupt** | Active-low, open-drain | Active-low, open-drain |
| **UIOX init helper** | `uiox_fw_als_init_veml7700()` | `uiox_fw_als_init_opt3001()` |

---

## 2 · Pin Layout

### 2.1 I²C Bus Pins (shared with other I²C devices)

| Pin | Signal | Direction | Description |
| --- | --- | --- | --- |
| SDA | Serial Data | Bidirectional | I²C data line — requires 4.7 kΩ pull-up to VCC |
| SCL | Serial Clock | Input (from master) | I²C clock line — requires 4.7 kΩ pull-up to VCC |
| GND | Ground | Power | Common ground reference |
| VCC | Power | Power | 2.5 V – 3.6 V supply (VEML7700) or 1.6–3.6 V (OPT3001) |

### 2.2 VEML7700 — Package & Pin Assignments (2×2 mm ODFN-6)

| Pin # | Name | Direction | Description |
| --- | --- | --- | --- |
| 1 | SDA | Bidirectional | I²C serial data |
| 2 | SCL | Input | I²C serial clock |
| 3 | ADDR | Input | I²C address select: LOW → 0x10, HIGH → 0x48 |
| 4 | INT | Output (OD) | Interrupt: active-low, open-drain; assert when lux crosses threshold |
| 5 | GND | Power | Ground |
| 6 | VCC | Power | 2.5 V – 3.6 V |

### 2.3 OPT3001 — Package & Pin Assignments (SOT-5X1 / USON-6)

| Pin # | Name | Direction | Description |
| --- | --- | --- | --- |
| 1 | SCL | Input | I²C serial clock |
| 2 | SDA | Bidirectional | I²C serial data |
| 3 | GND | Power | Ground |
| 4 | INT | Output (OD) | Interrupt active-low; latched until result register read |
| 5 | VCC | Power | 1.6 V – 3.6 V |
| 6 | ADDR0 | Input | Address bit 0: sets I²C address [0x44–0x47] |

### 2.4 UIOX GPIO Assignments

| Signal | UIOX GPIO Pin | Direction | Notes |
| --- | --- | --- | --- |
| INT | configurable via `uiox_als_dev_t.irq` | Input, IRQ_FALLING | Triggers `uiox_fw_als_irq()` |
| ADDR (VEML7700) | tied to GND or VCC on PCB | — | Selects 0x10 or 0x48 |
| ADDR0 (OPT3001) | tied to GND/VCC/SDA/SCL | — | Selects address 0x44–0x47 |

---

## 3 · Register Map

### 3.1 VEML7700 Registers (I²C, 16-bit little-endian)

| Reg Addr | Name | R/W | Description |
| --- | --- | --- | --- |
| 0x00 | ALS_CONF | R/W | Gain[12:11], IT[9:6], PERS[5:4], INT_EN[1], SD[0] |
| 0x01 | ALS_WH | R/W | High threshold window (raw counts) |
| 0x02 | ALS_WL | R/W | Low threshold window (raw counts) |
| 0x03 | POWER_SAVING | R/W | PSM_EN[0], PSM[2:1] — power save mode |
| 0x04 | ALS | R | ALS channel result (raw 16-bit count) |
| 0x05 | WHITE | R | White channel result (raw 16-bit count) |
| 0x06 | ALS_INT | R | INT_TH_HIGH[15], INT_TH_LOW[14] — interrupt flags |

### 3.2 VEML7700 ALS_CONF Bit Fields

| Bits | Field | Description |
| --- | --- | --- |
| [12:11] | ALS_GAIN | 00=×1, 01=×2, 10=×1/8, 11=×1/4 |
| [9:6] | ALS_IT | Integration time: 0000=100ms, 0001=200ms, 0010=400ms, 0011=800ms, 1000=50ms, 1100=25ms |
| [5:4] | ALS_PERS | Interrupt persistence: 00=1, 01=2, 10=4, 11=8 |
| [1] | ALS_INT_EN | 1=interrupt enabled |
| [0] | ALS_SD | 1=shut down (low power), 0=active |

### 3.3 OPT3001 Registers (I²C, 16-bit big-endian)

| Reg Addr | Name | R/W | Description |
| --- | --- | --- | --- |
| 0x00 | RESULT | R | Exponent[15:12] + Mantissa[11:0] — lux result |
| 0x01 | CONFIGURATION | R/W | RN[15:12], CT[11], M[10:9], OVF[8], CRF[7], FH[6], FL[5], L[4], POL[3], ME[2], FC[1:0] |
| 0x02 | LOW_LIMIT | R/W | Low threshold (same format as RESULT) |
| 0x03 | HIGH_LIMIT | R/W | High threshold (same format as RESULT) |
| 0x7E | MANUFACTURER_ID | R | 0x5449 ("TI") |
| 0x7F | DEVICE_ID | R | 0x3001 |

---

## 4 · Gain & Integration Time Settings

| `uiox_als_gain_t` | VEML7700 | OPT3001 | Max Lux |
| --- | --- | --- | --- |
| `UIOX_ALS_GAIN_1X` | ×1 | Auto-range | 120 000 lx |
| `UIOX_ALS_GAIN_2X` | ×2 | — | 60 000 lx |
| `UIOX_ALS_GAIN_1_8X` | ×1/8 | — | 960 000 lx |
| `UIOX_ALS_GAIN_1_4X` | ×1/4 | — | 480 000 lx |

| `uiox_als_itime_t` | VEML7700 IT | OPT3001 CT | Conversion Time |
| --- | --- | --- | --- |
| `UIOX_ALS_IT_25MS` | 25 ms | — | 25 ms |
| `UIOX_ALS_IT_50MS` | 50 ms | — | 50 ms |
| `UIOX_ALS_IT_100MS` | 100 ms | 100 ms | 100 ms |
| `UIOX_ALS_IT_200MS` | 200 ms | — | 200 ms |
| `UIOX_ALS_IT_400MS` | 400 ms | — | 400 ms |
| `UIOX_ALS_IT_800MS` | 800 ms | 800 ms | 800 ms |

---

## 5 · UIOX Software Interface

### 5.1 Device Context Structure

```c
typedef struct {
    uiox_i2c_dev_t  *i2c;          /* I²C bus handle                  */
    uint8_t          addr;          /* I²C slave address               */
    uiox_als_chip_t  chip;          /* UIOX_ALS_VEML7700 / OPT3001     */
    uiox_als_gain_t  gain;          /* Current gain setting            */
    uiox_als_itime_t itime;         /* Integration time                */
    uint32_t         lux_milli;     /* Last lux reading × 1000         */
    bool             initialized;
} uiox_als_dev_t;
```

### 5.2 API Summary

| Function | Description |
| --- | --- |
| `uiox_fw_als_init(dev, ops)` | Initialise ALS with ops vtable |
| `uiox_fw_als_read_lux(dev, &lux_milli)` | Read lux × 1000 (integer, no float) |
| `uiox_fw_als_read_raw(dev, &als, &white)` | Read raw 16-bit ALS and WHITE channels |
| `uiox_fw_als_set_thresh(dev, low, high)` | Set interrupt threshold window |
| `uiox_fw_als_auto_gain(dev)` | Adjust gain/itime if saturated or too dark |
| `uiox_fw_als_init_veml7700(dev, i2c)` | Platform helper — init VEML7700 on I²C bus |
| `uiox_fw_als_init_opt3001(dev, i2c)` | Platform helper — init OPT3001 on I²C bus |

### 5.3 Typical Initialisation Sequence

```c
/* 1. Init I²C bus first */
uiox_fw_i2c_init_dw(&i2c_dev, UIOX_I2C_ARM64_BASE, 24000000, 35,
                     UIOX_I2C_SPEED_FAST);

/* 2. Init ALS (VEML7700 at 0x10) */
uiox_als_dev_t als;
uiox_fw_als_init_veml7700(&als, &i2c_dev);

/* 3. Read lux */
uint32_t lux_milli;
uiox_fw_als_read_lux(&als, &lux_milli);
printf("Lux: %u.%03u\n", lux_milli / 1000, lux_milli % 1000);

/* 4. Enable interrupt threshold */
uiox_fw_als_set_thresh(&als, 100, 50000);  /* 0.1 lx – 50 lx window */
```

---

## 6 · Schematic Connection Diagram

```
  SoC / MCU                          VEML7700
  ──────────                         ────────
  I²C_SDA  ──── 4.7kΩ ──── VCC       SDA (pin 1)
  I²C_SCL  ──── 4.7kΩ ──── VCC       SCL (pin 2)
                                      ADDR (pin 3) ── GND  (→ addr 0x10)
  GPIO_INT ◄──────────────────────── INT (pin 4)  (OD, active-low)
  GND ────────────────────────────── GND (pin 5)
  3.3V ───────────────────────────── VCC (pin 6)
```

---

## 7 · Electrical Characteristics

| Parameter | Min | Typical | Max | Unit |
| --- | --- | --- | --- | --- |
| Supply Voltage (VEML7700) | 2.5 | 3.0 | 3.6 | V |
| Supply Voltage (OPT3001) | 1.6 | 3.3 | 3.6 | V |
| Supply Current (active) | — | 90 | 180 | µA |
| Supply Current (shutdown) | — | 0.5 | 2 | µA |
| I²C SCL Frequency | 10 | 400 | 400 | kHz |
| INT pin pull-up voltage | — | VCC | — | V |
| INT output low voltage | — | 0.1 | 0.4 | V |
| Operating Temperature | -40 | 25 | +85 | °C |

---

## 8 · MMIO / IRQ Summary

| Resource | Value | Notes |
| --- | --- | --- |
| I²C Bus (ARM64) | `0x09040000` | DesignWare I²C base |
| I²C Bus (ARM32) | `0x10002000` | versatilepb I²C base |
| I²C Bus (x86) | `0xEFA0` port | Intel ICH SMBus |
| VEML7700 I²C Address | `0x10` or `0x48` | Set by ADDR pin |
| OPT3001 I²C Address | `0x44`–`0x47` | Set by ADDR0 pin |
| Interrupt GPIO | Configurable | Active-low, falling-edge trigger |

---

*Generated by UIOX documentation toolchain · 2026-07-16*
*Source: 02_FwHal/include/uiox_fw_als.h — github.com/Pramod645/UIOX*
