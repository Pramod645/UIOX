uart.md

https://ece353.engr.wisc.edu/serial-interfaces/uart-basics/
https://www.ezurio.com/resources/blog/an-overview-of-uart-protocols

uart interfaces with processor

+-------------------------------------------------------+

|                      PROCESSOR                        |
|   +------------+     +-------------+     +---------+  |
|   |  CPU Core  | <-> | System Bus  | <-> | Address |  |
|   +------------+     | (Data/Addr) |     | Decode  |  |
|                      +-------------+     +---------+  |
+-----------------------------|-------------------------+
                              |
              [Processor Interface Signals]
              - Data Bus (D0-D7 / D31)
              - Address Bus / Chip Select (CS)
              - Read/Write Controls (RD#, WR#)
              - Interrupt Request (IRQ)
                              v
+-----------------------------|-------------------------+

|                       UART CONTROLLER                 |
|  +-------------------------------------------------+  |
|  |             Control & Register Logic            |  |
|  +------------------+------------------------------+  |
|                     |                              |  |
|          +----------v----------+                   |  |
|          | Baud Rate Generator |                   |  |
|          +----------+----------+                   |  |
|                     |                              |  |
|         TX FIFO     v                              |  |
|      [Transmit] -> Shift Register -> [ TX Pin ] ----+--+--> (To external RX)

|                                                    |
|      [Receive]  <- Shift Register <- [ RX Pin ] ----+--+--< (From external TX)

|         RX FIFO                                    |
+-------------------------------------------------------+



