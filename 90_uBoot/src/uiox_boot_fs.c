/*
 * uiox_boot_fs.c  —  FAT32 read-only driver.
 */
#include "uiox_boot_fs.h"
#include "uiox_boot_mem.h"
#include "uiox_boot_console.h"

static uboot_u8_t g_s[512];
static uboot_u16_t r16(const uboot_u8_t *p)
{return(uboot_u16_t)(p[0]|(uboot_u16_t)(p[1]<<8));}
static uboot_u32_t r32(const uboot_u8_t *p)
{return(uboot_u32_t)(p[0]|(p[1]<<8)|(p[2]<<16)|((uboot_u32_t)p[3]<<24));}

static int rs(uboot_fat32_t *fs,uboot_u32_t lba)
{return fs->blk->read_sectors(fs->part_start_lba+lba,1,g_s);}

int uboot_fat32_init(uboot_fat32_t *fs,const uboot_blk_ops_t *blk,uboot_u64_t plba)
{
    if(!fs||!blk) return UBOOT_EINVAL;
    uboot_memset(fs,0,sizeof(*fs));
    fs->blk=blk; fs->part_start_lba=plba;
    if(blk->read_sectors(plba,1,g_s)!=0) return UBOOT_ENODEV;
    if(g_s[510]!=0x55||g_s[511]!=0xAA) return UBOOT_EBADIMG;
    fs->bytes_per_sect=r16(g_s+FAT32_OFS_BYTES_PER_SECT);
    fs->sects_per_clus=g_s[FAT32_OFS_SECTS_PER_CLUS];
    fs->rsvd_sects=r16(g_s+FAT32_OFS_RSVD_SECTS);
    uboot_u8_t nf=g_s[FAT32_OFS_NUM_FATS];
    uboot_u32_t fsz=r32(g_s+FAT32_OFS_FAT_SIZE32);
    fs->root_clus=r32(g_s+FAT32_OFS_ROOT_CLUS);
    fs->fat_start_sect=fs->rsvd_sects;
    fs->data_start_sect=fs->rsvd_sects+nf*fsz;
    return UBOOT_OK;
}

static uboot_u32_t c2s(uboot_fat32_t *fs,uboot_u32_t c)
{return fs->data_start_sect+(c-2u)*(uboot_u32_t)fs->sects_per_clus;}

static uboot_u32_t nxtc(uboot_fat32_t *fs,uboot_u32_t c)
{uboot_u32_t off=c*4u,sect=fs->fat_start_sect+off/512u;
if(rs(fs,sect)!=0) return 0x0FFFFFFFu;
return r32(g_s+off%512u)&0x0FFFFFFFu;}

static void to83(const char *n,char o[11])
{uboot_memset((void*)o,' ',11);int i=0;
for(;*n&&*n!='.'&&i<8;n++,i++){char c=*n;if(c>='a'&&c<='z')c-=32;o[i]=c;}
if(*n=='.') n++;
for(i=8;*n&&i<11;n++,i++){char c=*n;if(c>='a'&&c<='z')c-=32;o[i]=c;}}

int uboot_fat32_load(uboot_fat32_t *fs,const char *fn,
                      void *dest,uboot_size_t max,uboot_size_t *sz)
{
    if(!fs||!fn||!dest) return UBOOT_EINVAL;
    char fat[11]; to83(fn,fat);
    uboot_u32_t clus=fs->root_clus;
    uboot_bool_t found=UBOOT_FALSE;
    uboot_u32_t fsz=0,fclus=0;
    while(clus<FAT32_EOC&&!found){
        uboot_u32_t s=c2s(fs,clus);
        for(uboot_u32_t si=0;si<(uboot_u32_t)fs->sects_per_clus&&!found;si++){
            if(rs(fs,s+si)!=0) return UBOOT_ENODEV;
            for(int e=0;e<16&&!found;e++){
                uboot_u8_t *de=g_s+e*FAT32_DE_SIZE;
                if(de[0]==0x00) goto done;
                if(de[0]==0xE5||de[FAT32_DE_ATTR]==FAT32_ATTR_LFN) continue;
                if(de[FAT32_DE_ATTR]&FAT32_ATTR_DIR) continue;
                if(uboot_memcmp(de,fat,11)==0){
                    fclus=((uboot_u32_t)r16(de+FAT32_DE_CLUS_HI)<<16)
                           |r16(de+FAT32_DE_CLUS_LO);
                    fsz=r32(de+FAT32_DE_FILE_SIZE);
                    found=UBOOT_TRUE;
                }
            }
        }
        clus=nxtc(fs,clus);
    }
done:
    if(!found) return UBOOT_ENOENT;
    if(fsz>max) return UBOOT_ENOMEM;
    uboot_u8_t *d=(uboot_u8_t*)dest;
    uboot_size_t rem=fsz; clus=fclus;
    while(clus<FAT32_EOC&&rem>0){
        uboot_u32_t s=c2s(fs,clus);
        for(uboot_u32_t si=0;si<(uboot_u32_t)fs->sects_per_clus&&rem>0;si++){
            if(rs(fs,s+si)!=0) return UBOOT_ENODEV;
            uboot_size_t t=rem<512u?rem:512u;
            uboot_memcpy(d,g_s,t);d+=t;rem-=t;
        }
        clus=nxtc(fs,clus);
    }
    if(sz) *sz=fsz;
    return UBOOT_OK;
}

int uboot_fat32_exists(uboot_fat32_t *fs,const char *fn)
{uboot_u8_t b[1];uboot_size_t s=0;
int r=uboot_fat32_load(fs,fn,b,1,&s);
return(r==UBOOT_OK||r==UBOOT_ENOMEM)?UBOOT_TRUE:UBOOT_FALSE;}

int uboot_blk_probe_emmc(uboot_blk_ops_t *o){(void)o;return UBOOT_ENODEV;}
int uboot_blk_probe_nvme(uboot_blk_ops_t *o,uboot_u32_t i){(void)o;(void)i;return UBOOT_ENODEV;}
int uboot_blk_probe_usb(uboot_blk_ops_t *o){(void)o;return UBOOT_ENODEV;}
