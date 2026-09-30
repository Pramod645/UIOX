| uiox_dev calls | uiox_hw provides |
| --- | --- |
| dev_interrupt_handler() fires | irq_dispatch() looks up and calls the registered handler |
| Driver open uses setjmp | cpu_context_save() / cpu_context_restore() |
| tty_write / tty_read | mmio_write32(UART_BASE,…) / mmio_read32(…) |
| Strategy I/O completion | DMA descriptor chain + IRQ_DISK handler |
| Critical sections | cpu_irq_disable() / cpu_irq_restore() |