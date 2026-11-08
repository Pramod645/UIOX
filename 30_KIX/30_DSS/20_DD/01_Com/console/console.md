50_UIX/01_shell/           front-ends — cli, gui, vbi, nli
      │  (boot context today: in-kernel calls; later: syscalls)
      ▼
30_KIX/32_FS/10_scfs       fd table, mount table, syscall bodies
      │  dev->ops  ← the seam uiox_uart_blk.c provides
      ▼
30_KIX/30_DSS/20_DD/       uart/  (the discipline)   console/ (the name)
      │
      ▼
30_KIX/30_DSS/10_DI/       HAL — putc, rx_pop, registers
      │
      ▼
    silicon
