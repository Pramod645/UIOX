/*
 * 01_uBoot/src/uiox_boot_dt.c
 * Device-tree runtime extraction:
 *   /chosen -> bootargs          (replaces UIOX_CMDLINE)
 *   /soc/  -> peripheral bases  (overlays uiox_soc_map.h)
 */
#include "uiox_boot.h"
#include "uiox_boot_mem.h"

#define FDT_BEGIN_NODE 0x00000001u
#define FDT_END_NODE   0x00000002u
#define FDT_PROP       0x00000003u
#define FDT_NOP        0x00000004u
#define FDT_END        0x00000009u
#define UIOX_DTB_MAGIC 0xD00DFEEDu

#define OFF_DT_STRUCT   8u
#define OFF_DT_STRINGS 12u

static uint32_t dt_be32(const uint8_t *p)
{
    return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16)
         | ((uint32_t)p[2] <<  8) |  (uint32_t)p[3];
}
static uint64_t dt_be64(const uint8_t *p)
{
    return ((uint64_t)dt_be32(p) << 32) | (uint64_t)dt_be32(p + 4);
}

typedef int (*dt_node_fn)(void *ctx, uint32_t depth, const char *name);
typedef int (*dt_prop_fn)(void *ctx, uint32_t depth, const char *name,
                          const uint8_t *val, uint32_t len);

static int dt_walk(const uint8_t *fdt, dt_node_fn nfn, dt_prop_fn pfn, void *ctx)
{
    uint32_t     struct_off = dt_be32(fdt + OFF_DT_STRUCT);
    const uint8_t *p        = fdt + struct_off;
    const char   *strtab    = (const char *)(fdt + dt_be32(fdt + OFF_DT_STRINGS));
    uint32_t      depth     = 0u;

    for (;;) {
        uint32_t tok = dt_be32(p); p += 4u;
        if (tok == FDT_BEGIN_NODE) {
            const char *name = (const char *)p;
            if (nfn && nfn(ctx, depth, name)) return 1;
            while (*p) p++;
            p = (const uint8_t *)(((uintptr_t)p + 4u) & ~3u);
            depth++;
        } else if (tok == FDT_END_NODE) {
            if (depth) depth--;
        } else if (tok == FDT_PROP) {
            uint32_t plen = dt_be32(p); p += 4u;
            uint32_t noff = dt_be32(p); p += 4u;
            const char *nm = strtab + noff;
            if (pfn && pfn(ctx, depth, nm, p, plen)) return 1;
            p += ((plen + 3u) & ~3u);
        } else if (tok == FDT_NOP) {
            /* skip */
        } else {
            return 0;
        }
    }
}

/* ── /chosen → bootargs ─────────────────────────────────────────── */
struct chosen_ctx { char *out; uint32_t max; int in_chosen; int found; };

static int chosen_node(void *c, uint32_t depth, const char *name)
{
    struct chosen_ctx *x = (struct chosen_ctx *)c;
    x->in_chosen = (depth == 1u) && name[0]=='c' && name[1]=='h' && name[2]=='o'
                && name[3]=='s' && name[4]=='e' && name[5]=='n' && name[6]=='\0';
    return 0;
}

static int chosen_prop(void *c, uint32_t depth, const char *name,
                       const uint8_t *val, uint32_t len)
{
    struct chosen_ctx *x = (struct chosen_ctx *)c;
    (void)depth;
    if (!x->in_chosen) return 0;
    if (name[0]=='b' && name[1]=='o' && name[2]=='o' && name[3]=='t'
        && name[4]=='a' && name[5]=='r' && name[6]=='g' && name[7]=='s'
        && name[8]=='\0') {
        uint32_t n = (len < x->max - 1u) ? len : x->max - 1u;
        for (uint32_t i = 0; i < n; i++) x->out[i] = (char)val[i];
        x->out[n] = '\0';
        x->found  = 1;
        return 1;
    }
    return 0;
}

uiox_boot_err_t uiox_boot_dt_chosen(const void *fdt, char *bootargs_out,
                                    uint32_t max_len)
{
    if (!fdt || !bootargs_out || max_len < 2u) return UIOX_BOOT_ERR_INVAL;
    if (dt_be32((const uint8_t *)fdt) != UIOX_DTB_MAGIC) return UIOX_BOOT_ERR_BADMAGIC;
    struct chosen_ctx x = { bootargs_out, max_len, 0, 0 };
    bootargs_out[0] = '\0';
    dt_walk((const uint8_t *)fdt, chosen_node, chosen_prop, &x);
    return x.found ? UIOX_BOOT_OK : UIOX_BOOT_ERR_NOTFOUND;
}

/* ── /soc → peripheral bases ────────────────────────────────────── */
struct soc_ctx {
    uiox_soc_runtime_t *m;
    uint64_t  reg_base, reg_size;
    uint32_t  addr_cells, size_cells;
    char      compat[64];
    uint32_t  irq0;
    int       in_soc;
    uint32_t  depth_soc;
};

static void soc_reg(const uint8_t *val, uint32_t len,
                    uint32_t ac, uint32_t sc,
                    uint64_t *base, uint64_t *size)
{
    *base = 0u; *size = 0u;
    if (len >= (ac + sc) * 4u) {
        *base = (ac == 2u) ? dt_be64(val) : dt_be32(val);
        const uint8_t *sp = val + ac * 4u;
        *size = (sc == 2u) ? dt_be64(sp) : dt_be32(sp);
    }
}

static int pfx(const char *s, const char *p)
{
    while (*p) { if (*s++ != *p++) return 0; }
    return 1;
}

static void soc_commit(struct soc_ctx *x)
{
    if (!x->reg_base) return;
    uiox_soc_runtime_t *m = x->m;

    if (pfx(x->compat, "arm,pl011")) {
        m->uart0_base = x->reg_base; m->uart_irq = x->irq0; m->sourced_from_dt = 1u;
    } else if (pfx(x->compat, "arm,pl190")) {
        m->vic_base = x->reg_base; m->sourced_from_dt = 1u;
    } else if (pfx(x->compat, "arm,sp804")) {
        m->timer_base = x->reg_base; m->timer_irq = x->irq0; m->sourced_from_dt = 1u;
    } else if (pfx(x->compat, "arm,gic")) {
        if (!m->gic_dist_base) m->gic_dist_base = x->reg_base;
        else                   m->gic_cpu_base  = x->reg_base;
        m->sourced_from_dt = 1u;
    } else if (pfx(x->compat, "ns16550")) {
        if (!m->uart0_base) { m->uart0_base = x->reg_base; m->uart_irq = x->irq0; }
        m->sourced_from_dt = 1u;
    } else if (pfx(x->compat, "sifive,uart")) {
        if (!m->uart0_base) { m->uart0_base = x->reg_base; m->uart_irq = x->irq0; }
        m->sourced_from_dt = 1u;
    } else if (pfx(x->compat, "riscv,plic")) {
        m->plic_base = x->reg_base; m->sourced_from_dt = 1u;
    } else if (pfx(x->compat, "sifive,clint")) {
        m->clint_base = x->reg_base; m->sourced_from_dt = 1u;
    } else if (pfx(x->compat, "virtio,mmio")) {
        if (!m->virtio_base) { m->virtio_base = x->reg_base; m->virtio_stride = x->reg_size; }
        m->sourced_from_dt = 1u;
    } else if (pfx(x->compat, "uiox,")) {
        m->storage_base = x->reg_base; m->sourced_from_dt = 1u;
    }
}

static int soc_node(void *c, uint32_t depth, const char *name)
{
    struct soc_ctx *x = (struct soc_ctx *)c;
    if (depth == 1u && name[0]=='s' && name[1]=='o' && name[2]=='c' && name[3]=='\0') {
        if (x->in_soc) soc_commit(x);
        x->in_soc = 1; x->depth_soc = depth;
    } else if (x->in_soc && depth <= x->depth_soc) {
        soc_commit(x);
        x->in_soc = 0;
    } else if (x->in_soc) {
        soc_commit(x);
    }
    x->reg_base = 0u; x->reg_size = 0u; x->irq0 = 0u; x->compat[0] = '\0';
    return 0;
}

static int soc_prop(void *c, uint32_t depth, const char *name,
                    const uint8_t *val, uint32_t len)
{
    struct soc_ctx *x = (struct soc_ctx *)c;
    if (!x->in_soc) return 0;
    if (depth == x->depth_soc + 1u) {
        if (name[0]=='#' && name[1]=='a' && len == 4u) x->addr_cells = dt_be32(val);
        else if (name[0]=='#' && name[1]=='s' && len == 4u) x->size_cells = dt_be32(val);
        return 0;
    }
    if (name[0]=='r' && name[1]=='e' && name[2]=='g' && name[3]=='\0')
        soc_reg(val, len, x->addr_cells, x->size_cells, &x->reg_base, &x->reg_size);
    else if (name[0]=='c' && len >= 2u) {
        uint32_t n = (len < sizeof(x->compat) - 1u) ? len : sizeof(x->compat) - 1u;
        for (uint32_t i = 0; i < n; i++) x->compat[i] = (char)val[i];
        x->compat[n] = '\0';
    } else if (name[0]=='i' && name[1]=='n' && len >= 4u) {
        x->irq0 = dt_be32(val);
    }
    return 0;
}

uiox_boot_err_t uiox_boot_dt_soc(const void *fdt, uiox_soc_runtime_t *out)
{
    if (!fdt || !out) return UIOX_BOOT_ERR_INVAL;
    if (dt_be32((const uint8_t *)fdt) != UIOX_DTB_MAGIC) return UIOX_BOOT_ERR_BADMAGIC;

    for (uint32_t i = 0; i < sizeof(*out); i++) ((uint8_t *)out)[i] = 0u;

    struct soc_ctx x;
    for (uint32_t i = 0; i < sizeof(x); i++) ((uint8_t *)&x)[i] = 0u;
    x.m = out;
    x.addr_cells = 1u; x.size_cells = 1u;

    dt_walk((const uint8_t *)fdt, soc_node, soc_prop, &x);
    soc_commit(&x);
    return out->sourced_from_dt ? UIOX_BOOT_OK : UIOX_BOOT_ERR_NOTFOUND;
}

uiox_boot_err_t uiox_boot_dt_apply(uint64_t dtb_pa, char *bootargs_out,
                                   uint32_t max_len, uiox_soc_runtime_t *out)
{
    if (!dtb_pa) return UIOX_BOOT_ERR_NOTFOUND;
    const void *fdt = (const void *)(__UINTPTR_TYPE__)dtb_pa;
    uiox_boot_err_t rc = UIOX_BOOT_OK;
    if (bootargs_out && max_len)
        if (uiox_boot_dt_chosen(fdt, bootargs_out, max_len) != UIOX_BOOT_OK)
            rc = UIOX_BOOT_ERR_NOTFOUND;
    if (out)
        if (uiox_boot_dt_soc(fdt, out) != UIOX_BOOT_OK)
            rc = UIOX_BOOT_ERR_NOTFOUND;
    return rc;
}
