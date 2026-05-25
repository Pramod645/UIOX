/*
 * idt.c  —  x86_64 IDT setup and interrupt dispatch.
 */

 #include "../include/arch.h"
 #include <string.h>
 #include <stdint.h>
 
 ARCH_ALIGNED(16) idt_entry_t idt_table[IDT_ENTRY_COUNT];
 idtr_t           idtr_value;
 
 static irq_handler_t irq_handlers[IDT_ENTRY_COUNT];
 
 /* ── ISR stub declarations (defined in boot.S) ───────────── */
 #define DECL_ISR(n) extern void isr##n(void);
 DECL_ISR(0)   DECL_ISR(1)   DECL_ISR(2)   DECL_ISR(3)
 DECL_ISR(4)   DECL_ISR(5)   DECL_ISR(6)   DECL_ISR(7)
 DECL_ISR(8)   DECL_ISR(9)   DECL_ISR(10)  DECL_ISR(11)
 DECL_ISR(12)  DECL_ISR(13)  DECL_ISR(14)  DECL_ISR(15)
 DECL_ISR(16)  DECL_ISR(17)  DECL_ISR(18)  DECL_ISR(19)
 DECL_ISR(20)  DECL_ISR(21)  DECL_ISR(22)  DECL_ISR(23)
 DECL_ISR(24)  DECL_ISR(25)  DECL_ISR(26)  DECL_ISR(27)
 DECL_ISR(28)  DECL_ISR(29)  DECL_ISR(30)  DECL_ISR(31)
 DECL_ISR(32)  DECL_ISR(33)  DECL_ISR(34)  DECL_ISR(35)
 DECL_ISR(36)  DECL_ISR(37)  DECL_ISR(38)  DECL_ISR(39)
 DECL_ISR(40)  DECL_ISR(41)  DECL_ISR(42)  DECL_ISR(43)
 DECL_ISR(44)  DECL_ISR(45)  DECL_ISR(46)  DECL_ISR(47)
 DECL_ISR(48)  DECL_ISR(49)  DECL_ISR(50)  DECL_ISR(51)
 DECL_ISR(52)  DECL_ISR(53)  DECL_ISR(54)  DECL_ISR(55)
 DECL_ISR(56)  DECL_ISR(57)  DECL_ISR(58)  DECL_ISR(59)
 DECL_ISR(60)  DECL_ISR(61)  DECL_ISR(62)  DECL_ISR(63)
 DECL_ISR(64)  DECL_ISR(65)  DECL_ISR(66)  DECL_ISR(67)
 DECL_ISR(68)  DECL_ISR(69)  DECL_ISR(70)  DECL_ISR(71)
 DECL_ISR(72)  DECL_ISR(73)  DECL_ISR(74)  DECL_ISR(75)
 DECL_ISR(76)  DECL_ISR(77)  DECL_ISR(78)  DECL_ISR(79)
 DECL_ISR(80)  DECL_ISR(81)  DECL_ISR(82)  DECL_ISR(83)
 DECL_ISR(84)  DECL_ISR(85)  DECL_ISR(86)  DECL_ISR(87)
 DECL_ISR(88)  DECL_ISR(89)  DECL_ISR(90)  DECL_ISR(91)
 DECL_ISR(92)  DECL_ISR(93)  DECL_ISR(94)  DECL_ISR(95)
 DECL_ISR(96)  DECL_ISR(97)  DECL_ISR(98)  DECL_ISR(99)
 DECL_ISR(100) DECL_ISR(101) DECL_ISR(102) DECL_ISR(103)
 DECL_ISR(104) DECL_ISR(105) DECL_ISR(106) DECL_ISR(107)
 DECL_ISR(108) DECL_ISR(109) DECL_ISR(110) DECL_ISR(111)
 DECL_ISR(112) DECL_ISR(113) DECL_ISR(114) DECL_ISR(115)
 DECL_ISR(116) DECL_ISR(117) DECL_ISR(118) DECL_ISR(119)
 DECL_ISR(120) DECL_ISR(121) DECL_ISR(122) DECL_ISR(123)
 DECL_ISR(124) DECL_ISR(125) DECL_ISR(126) DECL_ISR(127)
 DECL_ISR(128) DECL_ISR(129) DECL_ISR(130) DECL_ISR(131)
 DECL_ISR(132) DECL_ISR(133) DECL_ISR(134) DECL_ISR(135)
 DECL_ISR(136) DECL_ISR(137) DECL_ISR(138) DECL_ISR(139)
 DECL_ISR(140) DECL_ISR(141) DECL_ISR(142) DECL_ISR(143)
 DECL_ISR(144) DECL_ISR(145) DECL_ISR(146) DECL_ISR(147)
 DECL_ISR(148) DECL_ISR(149) DECL_ISR(150) DECL_ISR(151)
 DECL_ISR(152) DECL_ISR(153) DECL_ISR(154) DECL_ISR(155)
 DECL_ISR(156) DECL_ISR(157) DECL_ISR(158) DECL_ISR(159)
 DECL_ISR(160) DECL_ISR(161) DECL_ISR(162) DECL_ISR(163)
 DECL_ISR(164) DECL_ISR(165) DECL_ISR(166) DECL_ISR(167)
 DECL_ISR(168) DECL_ISR(169) DECL_ISR(170) DECL_ISR(171)
 DECL_ISR(172) DECL_ISR(173) DECL_ISR(174) DECL_ISR(175)
 DECL_ISR(176) DECL_ISR(177) DECL_ISR(178) DECL_ISR(179)
 DECL_ISR(180) DECL_ISR(181) DECL_ISR(182) DECL_ISR(183)
 DECL_ISR(184) DECL_ISR(185) DECL_ISR(186) DECL_ISR(187)
 DECL_ISR(188) DECL_ISR(189) DECL_ISR(190) DECL_ISR(191)
 DECL_ISR(192) DECL_ISR(193) DECL_ISR(194) DECL_ISR(195)
 DECL_ISR(196) DECL_ISR(197) DECL_ISR(198) DECL_ISR(199)
 DECL_ISR(200) DECL_ISR(201) DECL_ISR(202) DECL_ISR(203)
 DECL_ISR(204) DECL_ISR(205) DECL_ISR(206) DECL_ISR(207)
 DECL_ISR(208) DECL_ISR(209) DECL_ISR(210) DECL_ISR(211)
 DECL_ISR(212) DECL_ISR(213) DECL_ISR(214) DECL_ISR(215)
 DECL_ISR(216) DECL_ISR(217) DECL_ISR(218) DECL_ISR(219)
 DECL_ISR(220) DECL_ISR(221) DECL_ISR(222) DECL_ISR(223)
 DECL_ISR(224) DECL_ISR(225) DECL_ISR(226) DECL_ISR(227)
 DECL_ISR(228) DECL_ISR(229) DECL_ISR(230) DECL_ISR(231)
 DECL_ISR(232) DECL_ISR(233) DECL_ISR(234) DECL_ISR(235)
 DECL_ISR(236) DECL_ISR(237) DECL_ISR(238) DECL_ISR(239)
 DECL_ISR(240) DECL_ISR(241) DECL_ISR(242) DECL_ISR(243)
 DECL_ISR(244) DECL_ISR(245) DECL_ISR(246) DECL_ISR(247)
 DECL_ISR(248) DECL_ISR(249) DECL_ISR(250) DECL_ISR(251)
 DECL_ISR(252) DECL_ISR(253) DECL_ISR(254) DECL_ISR(255)
 
 /* table of all 256 ISR stubs */
 static void (*isr_stub_table[256])(void) = {
     isr0,   isr1,   isr2,   isr3,   isr4,   isr5,   isr6,   isr7,
     isr8,   isr9,   isr10,  isr11,  isr12,  isr13,  isr14,  isr15,
     isr16,  isr17,  isr18,  isr19,  isr20,  isr21,  isr22,  isr23,
     isr24,  isr25,  isr26,  isr27,  isr28,  isr29,  isr30,  isr31,
     isr32,  isr33,  isr34,  isr35,  isr36,  isr37,  isr38,  isr39,
     isr40,  isr41,  isr42,  isr43,  isr44,  isr45,  isr46,  isr47,
     isr48,  isr49,  isr50,  isr51,  isr52,  isr53,  isr54,  isr55,
     isr56,  isr57,  isr58,  isr59,  isr60,  isr61,  isr62,  isr63,
     isr64,  isr65,  isr66,  isr67,  isr68,  isr69,  isr70,  isr71,
     isr72,  isr73,  isr74,  isr75,  isr76,  isr77,  isr78,  isr79,
     isr80,  isr81,  isr82,  isr83,  isr84,  isr85,  isr86,  isr87,
     isr88,  isr89,  isr90,  isr91,  isr92,  isr93,  isr94,  isr95,
     isr96,  isr97,  isr98,  isr99,  isr100, isr101, isr102, isr103,
     isr104, isr105, isr106, isr107, isr108, isr109, isr110, isr111,
     isr112, isr113, isr114, isr115, isr116, isr117, isr118, isr119,
     isr120, isr121, isr122, isr123, isr124, isr125, isr126, isr127,
     isr128, isr129, isr130, isr131, isr132, isr133, isr134, isr135,
     isr136, isr137, isr138, isr139, isr140, isr141, isr142, isr143,
     isr144, isr145, isr146, isr147, isr148, isr149, isr150, isr151,
     isr152, isr153, isr154, isr155, isr156, isr157, isr158, isr159,
     isr160, isr161, isr162, isr163, isr164, isr165, isr166, isr167,
     isr168, isr169, isr170, isr171, isr172, isr173, isr174, isr175,
     isr176, isr177, isr178, isr179, isr180, isr181, isr182, isr183,
     isr184, isr185, isr186, isr187, isr188, isr189, isr190, isr191,
     isr192, isr193, isr194, isr195, isr196, isr197, isr198, isr199,
     isr200, isr201, isr202, isr203, isr204, isr205, isr206, isr207,
     isr208, isr209, isr210, isr211, isr212, isr213, isr214, isr215,
     isr216, isr217, isr218, isr219, isr220, isr221, isr222, isr223,
     isr224, isr225, isr226, isr227, isr228, isr229, isr230, isr231,
     isr232, isr233, isr234, isr235, isr236, isr237, isr238, isr239,
     isr240, isr241, isr242, isr243, isr244, isr245, isr246, isr247,
     isr248, isr249, isr250, isr251, isr252, isr253, isr254, isr255
 };
 
 /* ── idt_set_gate ────────────────────────────────────────── */
 void idt_set_gate(uint8_t  vec,
                    uint64_t handler,
                    uint16_t selector,
                    uint8_t  type,
                    uint8_t  ist)
 {
     idt_table[vec].offset_low  =  handler        & 0xFFFF;
     idt_table[vec].selector    =  selector;
     idt_table[vec].ist         =  ist & 0x07;
     idt_table[vec].type_attr   =  type;
     idt_table[vec].offset_mid  = (handler >> 16) & 0xFFFF;
     idt_table[vec].offset_high = (handler >> 32) & 0xFFFFFFFF;
     idt_table[vec].reserved    =  0;
 }
 
 /* ── idt_register_handler ────────────────────────────────── */
 void idt_register_handler(uint8_t vec, irq_handler_t fn)
 {
     irq_handlers[vec] = fn;
 }
 
 /* ── idt_load ────────────────────────────────────────────── */
 void idt_load(void)
 {
     idtr_value.limit = (uint16_t)(sizeof(idt_table) - 1);
     idtr_value.base  = (uint64_t)idt_table;
     __asm__ volatile("lidt %0" :: "m"(idtr_value) : "memory");
 }
 
 /* ── idt_init ────────────────────────────────────────────── */
 void idt_init(void)
 {
     memset(idt_table,    0, sizeof(idt_table));
     memset(irq_handlers, 0, sizeof(irq_handlers));
 
     /* install all 256 stubs */
     for (int i = 0; i < IDT_ENTRY_COUNT; i++) {
         idt_set_gate((uint8_t)i,
                      (uint64_t)isr_stub_table[i],
                      GDT_KERN_CODE_SEL,
                      IDT_INTR_GATE,
                      0);
     }
 
     /* double fault uses IST slot 1 for its own stack */
     idt_set_gate(EXC_DOUBLE_FAULT,
                  (uint64_t)isr8,
                  GDT_KERN_CODE_SEL,
                  IDT_INTR_GATE,
                  1);
 
     /* remap 8259A PIC: IRQ0–7 → 0x20, IRQ8–15 → 0x28 */
     pic_init(IRQ_BASE, IRQ_BASE + 8);
 
     idt_load();
 }
 
 /* ── interrupt_dispatch ──────────────────────────────────── */
 void interrupt_dispatch(uint64_t vec,
                          uint64_t err,
                          void    *frame)
 {
     if (vec < IDT_ENTRY_COUNT && irq_handlers[vec])
         irq_handlers[vec](vec, err, frame);
     else {
         /* unhandled: print minimal info and halt */
         (void)vec; (void)err; (void)frame;
         __asm__ volatile("cli; hlt");
     }
 
     /* send EOI to PIC for hardware IRQs */
     if (vec >= (uint64_t)IRQ_BASE && vec < (uint64_t)(IRQ_BASE + 16))
         pic_send_eoi((uint8_t)(vec - IRQ_BASE));
 }
 