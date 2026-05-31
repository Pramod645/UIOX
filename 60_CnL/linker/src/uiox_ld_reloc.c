/*
 * uiox_ld_reloc.c - UIOX linker relocation engine
 */
#include "../include/uiox_ld_reloc.h"
#include <string.h>
#include <stdio.h>

void uld_reloc_table_init(uld_reloc_table_t *rt)
{
    memset(rt, 0, sizeof(*rt));
}

int uld_reloc_add(uld_reloc_table_t *rt,
                   uld_addr_t offset, uld_u32_t sym_idx,
                   uld_reloc_type_t type, uld_s64_t addend,
                   uld_u32_t sect_idx, uld_u32_t obj_idx)
{
    if (rt->count >= ULD_MAX_RELOCS) return -1;
    uld_reloc_t *r = &rt->entries[rt->count++];
    r->offset   = offset;
    r->sym_idx  = sym_idx;
    r->type     = type;
    r->addend   = addend;
    r->sect_idx = sect_idx;
    r->obj_idx  = obj_idx;
    return 0;
}

int uld_reloc_apply(uld_reloc_table_t *rt,
                     uld_sym_table_t   *syms,
                     uld_sect_table_t  *sects,
                     uld_diag_ctx_t    *diag)
{
    int errors = 0;

    for (uld_u32_t ri = 0; ri < rt->count; ri++) {
        uld_reloc_t *r = &rt->entries[ri];

        /* get symbol value */
        if (r->sym_idx >= syms->count) {
            ULD_ERR(diag, "<reloc>", 0,
                    "reloc[%u]: invalid symbol index %u",
                    ri, r->sym_idx);
            errors++; continue;
        }
        uld_symbol_t *sym = syms->list[r->sym_idx];
        if (!sym->defined) {
            ULD_ERR(diag, "<reloc>", 0,
                    "reloc[%u]: unresolved symbol '%s'",
                    ri, sym->name);
            errors++; continue;
        }

        /* get output section to patch */
        if (r->sect_idx >= sects->count) {
            errors++; continue;
        }
        uld_output_sect_t *sect = &sects->sects[r->sect_idx];
        uld_u64_t off = r->offset - sect->vaddr;
        if (off >= sect->size) {
            errors++; continue;
        }

        uld_s64_t S = (uld_s64_t)sym->value;
        uld_s64_t A = r->addend;
        uld_s64_t P = (uld_s64_t)(sect->vaddr + off);

        switch (r->type) {

            case ULD_R_ABS8: {
                uld_u8_t v = (uld_u8_t)(S + A);
                sect->data[off] = v;
                break;
            }
            case ULD_R_ABS16: {
                uld_u16_t v = (uld_u16_t)(S + A);
                memcpy(sect->data + off, &v, 2);
                break;
            }
            case ULD_R_ABS32: {
                uld_u32_t v = (uld_u32_t)(S + A);
                memcpy(sect->data + off, &v, 4);
                break;
            }
            case ULD_R_ABS64: {
                uld_u64_t v = (uld_u64_t)(S + A);
                memcpy(sect->data + off, &v, 8);
                break;
            }
            case ULD_R_REL8: {
                uld_s64_t v = S + A - P;
                if (v < -128 || v > 127) {
                    ULD_WARN(diag, "<reloc>", 0,
                             "REL8 overflow for '%s'", sym->name);
                }
                sect->data[off] = (uld_u8_t)(uld_s8_t)v;
                break;
            }
            case ULD_R_REL16: {
                uld_s64_t v = S + A - P;
                uld_u16_t w = (uld_u16_t)(uld_s16_t)v;
                memcpy(sect->data + off, &w, 2);
                break;
            }
            case ULD_R_REL32:
            case ULD_R_X86_PLT32: {
                uld_s64_t v = S + A - P;
                uld_u32_t w = (uld_u32_t)(uld_s32_t)v;
                memcpy(sect->data + off, &w, 4);
                break;
            }
            case ULD_R_REL64: {
                uld_s64_t v = S + A - P;
                memcpy(sect->data + off, &v, 8);
                break;
            }
            case ULD_R_ARM_B26: {
                /* ARM B/BL: [23:0] word offset */
                uld_s64_t word_off = (S + A - P) / 4;
                uld_u32_t instr;
                memcpy(&instr, sect->data + off, 4);
                instr = (instr & 0xFF000000u) |
                        ((uld_u32_t)word_off & 0x00FFFFFFu);
                memcpy(sect->data + off, &instr, 4);
                break;
            }
            case ULD_R_ARM_MOVW: {
                /* MOVW: imm16 low 16 bits of S+A */
                uld_u32_t imm16 = (uld_u32_t)(S + A) & 0xFFFF;
                uld_u32_t instr;
                memcpy(&instr, sect->data + off, 4);
                instr = (instr & 0xFFF0F000u) |
                        ((imm16 & 0xF000u) << 4) |
                        (imm16  & 0x0FFFu);
                memcpy(sect->data + off, &instr, 4);
                break;
            }
            case ULD_R_ARM_MOVT: {
                /* MOVT: imm16 high 16 bits of S+A */
                uld_u32_t imm16 = ((uld_u32_t)(S + A) >> 16) & 0xFFFF;
                uld_u32_t instr;
                memcpy(&instr, sect->data + off, 4);
                instr = (instr & 0xFFF0F000u) |
                        ((imm16 & 0xF000u) << 4) |
                        (imm16  & 0x0FFFu);
                memcpy(sect->data + off, &instr, 4);
                break;
            }
            case ULD_R_A64_CALL26:
            case ULD_R_A64_JUMP26: {
                /* AArch64 BL/B: imm26 word offset [25:0] */
                uld_s64_t word_off = (S + A - P) / 4;
                uld_u32_t instr;
                memcpy(&instr, sect->data + off, 4);
                instr = (instr & 0xFC000000u) |
                        ((uld_u32_t)word_off & 0x03FFFFFFu);
                memcpy(sect->data + off, &instr, 4);
                break;
            }
            case ULD_R_A64_ADR: {
                /* AArch64 ADR: imm21 */
                uld_s64_t disp = S + A - P;
                uld_u32_t instr;
                memcpy(&instr, sect->data + off, 4);
                uld_u32_t immlo = (uld_u32_t)(disp & 0x3);
                uld_u32_t immhi = (uld_u32_t)((disp >> 2) & 0x7FFFF);
                instr = (instr & 0x9F00001Fu) |
                        (immlo << 29) | (immhi << 5);
                memcpy(sect->data + off, &instr, 4);
                break;
            }
            case ULD_R_A64_ADRP: {
                /* AArch64 ADRP: page-relative imm21 */
                uld_s64_t disp = ((S + A) & ~0xFFFLL) - (P & ~0xFFFLL);
                uld_u32_t instr;
                memcpy(&instr, sect->data + off, 4);
                uld_u32_t immlo = (uld_u32_t)((disp >> 12) & 0x3);
                uld_u32_t immhi = (uld_u32_t)((disp >> 14) & 0x7FFFF);
                instr = (instr & 0x9F00001Fu) |
                        (immlo << 29) | (immhi << 5);
                memcpy(sect->data + off, &instr, 4);
                break;
            }
            case ULD_R_A64_LO12: {
                /* AArch64 ADD/LDR low 12 bits */
                uld_u32_t imm12 = (uld_u32_t)(S + A) & 0xFFF;
                uld_u32_t instr;
                memcpy(&instr, sect->data + off, 4);
                instr = (instr & 0xFFC003FFu) | (imm12 << 10);
                memcpy(sect->data + off, &instr, 4);
                break;
            }
            case ULD_R_A64_ABS64: {
                uld_u64_t v = (uld_u64_t)(S + A);
                memcpy(sect->data + off, &v, 8);
                break;
            }
            default:
                ULD_WARN(diag, "<reloc>", 0,
                         "unhandled reloc type %d for '%s'",
                         r->type, sym->name);
                break;
        }
    }
    return errors;
}

const char *uld_reloc_type_str(uld_reloc_type_t t)
{
    switch (t) {
        case ULD_R_NONE:       return "NONE";
        case ULD_R_ABS8:       return "ABS8";
        case ULD_R_ABS16:      return "ABS16";
        case ULD_R_ABS32:      return "ABS32";
        case ULD_R_ABS64:      return "ABS64";
        case ULD_R_REL8:       return "REL8";
        case ULD_R_REL16:      return "REL16";
        case ULD_R_REL32:      return "REL32";
        case ULD_R_REL64:      return "REL64";
        case ULD_R_X86_PLT32:  return "X86_PLT32";
        case ULD_R_X86_GOT32:  return "X86_GOT32";
        case ULD_R_ARM_B26:    return "ARM_B26";
        case ULD_R_ARM_MOVW:   return "ARM_MOVW";
        case ULD_R_ARM_MOVT:   return "ARM_MOVT";
        case ULD_R_A64_CALL26: return "A64_CALL26";
        case ULD_R_A64_JUMP26: return "A64_JUMP26";
        case ULD_R_A64_ADR:    return "A64_ADR";
        case ULD_R_A64_ADRP:   return "A64_ADRP";
        case ULD_R_A64_LO12:   return "A64_LO12";
        case ULD_R_A64_ABS64:  return "A64_ABS64";
        default:               return "UNKNOWN";
    }
}
