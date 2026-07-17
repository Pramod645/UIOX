File	Category	What It Defines
uiox_soc.h	SoC	Master umbrella include — uiox_soc_init(), uiox_soc_get_desc(), uiox_soc_get_clk(), uiox_soc_get_pm()
uiox_soc_types.h	SoC	uiox_soc_id_t enum (QEMU/BCM2711/IMX8/RK3588/SiFive…), uiox_soc_desc_t, UIOX_SOC_CAP_* capability flags
uiox_soc_map.h	SoC	MMIO base addresses and IRQ numbers for all 4 architectures (SOC_DRAM_BASE, SOC_GIC_DIST_BASE, SOC_UART0_BASE, SOC_CLINT_BASE, SOC_PLIC_BASE)
uiox_soc_clk.h	SoC	Clock tree: uiox_clk_id_t, uiox_pll_cfg_t, uiox_clk_ctx_t, uiox_clk_init(), uiox_clk_enable(), uiox_clk_get_hz()
uiox_soc_pm.h	SoC	Power domains + reset controller: uiox_pd_id_t, uiox_rst_id_t, uiox_pm_ctx_t, uiox_pm_domain_on/off(), uiox_rst_assert/deassert(), uiox_pm_cpu_on()
uiox_fw_types.h	SoC	Base integer types, uiox_fw_arch_t enum, uiox_fw_err_t error codes, UIOX_FW_BIT() macro — shared by everything
uiox_fw_hw.h	SoC	Platform HW abstraction vtable (uiox_fw_hw_ops_t 18-op table), uiox_fw_platform_t descriptor, UIOX_FW_CAP_* flags, uiox_fw_hw_register()
uiox_fw_clock.h	SoC	Clock/PLL management: uiox_fw_clk_id_t, uiox_fw_clock_t, uiox_fw_clock_init/enable/disable/get_hz()
uiox_fw_power.h	SoC	System-level power: PSCI function IDs, ACPI PM1, uiox_fw_pwr_state_t, uiox_fw_power_ctx_t, uiox_fw_power_reset/shutdown/cpu_on()
uiox_fw_psci.h	SoC	PSCI 1.1 dispatch table: all PSCI function IDs (CPU_ON/OFF/SUSPEND, SYSTEM_OFF/RESET), uiox_psci_ctx_t, SMC/HVC handler
uiox_fw_tz.h	SoC	ARM TrustZone EL3 setup: SCR_EL3_* bits, TZC-400 region config, secure/non-secure GIC group assignments, ERET to EL1
uiox_fw_irq.h	SoC	IRQ controller abstraction: GIC/APIC/PLIC init, irq_register(), irq_enable(), irq_ack()
uiox_fw_mem.h	SoC	Memory management: DDR region descriptors, bump allocator, uiox_fw_memset/memcpy() no-libc helpers
uiox_fw_dma.h	SoC	DMA controller: PL080/PL330/Intel 8237A, uiox_dma_ctrl_t, uiox_dma_xfer_t, uiox_fw_dma_transfer(), 8 channels
uiox_fw_post.h	SoC	Power-On Self Test: CPU register sanity, DRAM walk test, ROM CRC, peripheral smoke tests, uiox_post_report_t
uiox_fw_secboot.h	SoC	Secure boot chain of trust: RoT key, FVC certificate, SHA-256, RSA/ECDSA signature verify, anti-rollback, uiox_secboot_ctx_t
uiox_fw.h	SoC	Master firmware include — pulls in all FwHal headers, defines FW_LOG(), uiox_fw_printf()