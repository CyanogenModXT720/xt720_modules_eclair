/*
 * This modules maps additional mtd partions of the Motorola Milestone XT720
 *
 * Copyright (C) 2012 Mioze7Ae
 * Copyright (C) 2010 Janne Grunau
 * Copyright (C) 2010 Mike Baker
 *
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 */


#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/err.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/partitions.h>

/*
 * Motoroi XT720 / Milestone XT720 MTD partition layouts:
 *
 * The 2.1 and 2.2 have different /system, /data & /cache partition
 * sizes. The MDT partitions are set from the CDT for regular boot, so
 * they're already visibile and correct in bootstrapped recovery. Most
 * other partitions are identical across 2.1 and 2.2 sbf's for both
 * Motoroi XT720 and Milestone XT720. We only add the missing ones.
 *
 *     mtdparts=omap2-nand.0:
 *         128k(mbmloader),
 *         640k(mbm),
 *         640k(mbmbackup),
 *         384k(bploader),
 *         384k(cdt),
 *     === 1536k(pds),
 *         384k(lbl),
 *         384k(lbl_backup),
 *     === 384k(cid),
 *         1536k(sp),
 *         384k(devtree),
 *     === 640k(logo),
 *     === 384k(misc),
 *     === 3584k(boot),
 *         3840k(bpsw),
 *     === 4608k(recovery),
 *     === 8960k(cdrom),
 *         384k(unused0),
 *     === 204416k(system),    | === 173696k(system),         
 *         384k(unused1),      |     384k(unused1),         
 *     === 106m(cache),        | === 50m(cache),         
 *     === 177280k(userdata),  | === 265344k(userdata),    
 *     === 1536k(cust),
 *         384k(unused2),
 *     === 2m(kpanic),
 *         512k(rsv)
 *
 *     === : present in bootstrap recovery via cdt
 *
 */

/* Note: Inverted sense because these are masks -- bits to be _removed_ */
#define MTD_RO (MTD_WRITEABLE)
#define MTD_RW 0
#define mtd_hack_part(psize, pstart, pname, pflags)     \
        .name       = pname,                            \
        .size       = psize * 1024,                     \
        .offset     = pstart * 1024,                    \
        .mask_flags = pflags,

struct mtd_partition part[] = {
   { mtd_hack_part(    128,      0, "mbmloader",  MTD_RO ) },
   { mtd_hack_part(    640,    128, "mbm",        MTD_RO ) },
   { mtd_hack_part(    640,    768, "mbmbackup",  MTD_RO ) },
   { mtd_hack_part(    384,   1408, "bploader",   MTD_RO ) },
   { mtd_hack_part(    384,   1792, "cdt",        MTD_RO ) },
/* { mtd_hack_part(   1536,   2176, "pds",        MTD_RO ) }, */ /* cdt arg */
   { mtd_hack_part(    384,   3712, "lbl",        MTD_RO ) },
   { mtd_hack_part(    384,   4096, "lbl_backup", MTD_RO ) },
/* { mtd_hack_part(    384,   4480, "cid",        MTD_RO ) }, */ /* cdt arg */
   { mtd_hack_part(   1536,   4864, "sp",         MTD_RO ) },
   { mtd_hack_part(    384,   6400, "devtree",    MTD_RO ) },
/* { mtd_hack_part(    640,   6784, "logo",       MTD_RO ) }, */ /* cdt arg */
/* { mtd_hack_part(    384,   7424, "misc",       MTD_RO ) }, */ /* cdt arg */
/* { mtd_hack_part(   3584,   7808, "boot",       MTD_RO ) }, */ /* cdt arg */
   { mtd_hack_part(   3840,  11392, "bpsw",       MTD_RO ) },
/* { mtd_hack_part(   4608,  15232, "recovery",   MTD_RO ) }, */ /* cdt arg */
/* { mtd_hack_part(   8960,  19840, "cdrom",      MTD_RO ) }, */ /* cdt arg */
   { mtd_hack_part(    384,  28800, "unused0",    MTD_RO ) },
/* { mtd_hack_part( 204416,  29184, "system",     MTD_RO ) }, */ /* cdt arg */
/* { mtd_hack_part(    384, 233600, "unused1",    MTD_RO ) }, */ /* varies */
/* { mtd_hack_part( 108544, 233984, "cache",      MTD_RO ) }, */ /* cdt arg */
/* { mtd_hack_part( 177280, 342528, "userdata",   MTD_RO ) }, */ /* cdt arg */
/* { mtd_hack_part(   1536, 519808, "cust",       MTD_RO ) }, */ /* cdt arg */
   { mtd_hack_part(    384, 521344, "unused2",    MTD_RO ) },
/* { mtd_hack_part(   2048, 521728, "kpanic",     MTD_RO ) }, */ /* cdt arg */
   { mtd_hack_part(    512, 523776, "rsv",        MTD_RO ) },
   { mtd_hack_part( 524288,      0, "totality",   MTD_RO ) },
};

static int mark_boot_writeable(void)
{
    struct mtd_info *mtd = NULL;

    mtd = get_mtd_device_nm("boot");
    if (IS_ERR(mtd))
        return -1;

    printk(KERN_INFO "mtd-hack: original 'boot' partition flags: %d\n", mtd->flags);
    mtd->flags = (mtd->flags | MTD_WRITEABLE);
    printk(KERN_INFO "mtd-hack: modified 'boot' partition flags: %d\n", mtd->flags);

    return 0;
}

static int create_missing_flash_parts(struct device *dev, void *data)
{
    struct mtd_info *mtd = NULL;
    
    printk(KERN_INFO "mtd-hack: device %s\n", dev->init_name);

    mtd = dev_get_drvdata(dev);
    if (!mtd)
        return -1;

    printk(KERN_INFO "mtd-hack: mtd name %s, type %d, size %llu\n",
           mtd->name, mtd->type, mtd->size);
    add_mtd_partitions(mtd, part, sizeof(part)/sizeof(part[0]));	
    printk(KERN_INFO "mtd-hack: mtd name %s, type %d, size %llu\n",
           mtd->name, mtd->type, mtd->size);

    return 0;
}

static int __init mtd_init(void)
{
    struct device_driver *devdrv;
    int err = 0;

    mark_boot_writeable();

    devdrv = driver_find("omap2-nand", &platform_bus_type);
    printk(KERN_INFO "mtd-hack: found driver %s modname %s\n", devdrv->name, devdrv->mod_name);

    err = driver_for_each_device(devdrv, NULL, NULL, create_missing_flash_parts);
    printk(KERN_INFO "mtd hack loaded");

    return 0;
}
 
module_init(mtd_init);

MODULE_LICENSE("GPL");
