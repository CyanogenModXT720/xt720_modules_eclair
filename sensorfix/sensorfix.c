/* 
 * sensorfix: ensure correct initialization of kxtf9 orientation
 *            sensor processing on Milestone XT720
 *
 * Copyright (C) 2012 Mioze7Ae
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/string.h>

#include <linux/fs.h>

#include <linux/kxtf9.h>

struct kxtf9_data {
	struct i2c_client *client;
	struct kxtf9_platform_data *pdata;
};

MODULE_AUTHOR("Mioze7Ae");
MODULE_DESCRIPTION("Milestone XT720 sensorfix module");
MODULE_LICENSE("GPL");
MODULE_VERSION("1.0");

/*
 * Motoroi uses LIS331DLH orientation sensor, but Milestone XT720 uses
 * KXTF9 orientation sensor. These sensors require different
 * postprocessing and this information is encoded in the devtree.
 *
 * On Motoroi the relevant parts of the device-tree are:
 *
 *     I2C@0 {
 *             Accelerometer@0 {
 *                                negate_x = [01];
 *                                negate_y = [01];
 *                                negate_z = [01];
 *                                axis_map_x = [01];
 *                                axis_map_y = [00];
 *                                axis_map_z = [02];
 *                              };
 *            };
 *
 * On Milestone XT720, the corresponding parts of the device-tree are:
 *
 *     I2C@0 {
 *             Accelerometer@0 {
 *                                negate_x = [00];
 *                                negate_y = [01];
 *                                negate_z = [01];
 *                                axis_map_x = [00];
 *                                axis_map_y = [01];
 *                                axis_map_z = [02];
 *                              };
 *            };
 *
 * Consequently, on a Milestone XT720 running Motoroi's device-tree
 * (which seems to be required for vulnerable recovery), processing of
 * the sensor rotation is initialized inappropriately and the display
 * is always rotated 90 degrees unless autorotation is disabled.  This
 * module enters the initialized kxtf9 driver and substitutes the
 * correct values to fix that problem.
 *
 */

static int __init sensorfix_init(void)
{
	struct file *kxtf9;
	struct kxtf9_data *tf9;
	int exit_code = -EBUSY;

	kxtf9 = filp_open("/dev/kxtf9", O_RDONLY, 0);
	if (!kxtf9)
	{
		printk(KERN_ERR "sensorfix: failed to open /dev/kxtf9\n");
		return exit_code;
	}

	tf9 = (struct kxtf9_data *)kxtf9->private_data;
	if (!tf9)
	{
		printk(KERN_ERR "sensorfix: failed to fetch kxtf9_platform_data\n");
		goto err_close;
	}

	tf9->pdata->axis_map_x = 0;
	tf9->pdata->axis_map_y = 1;
	tf9->pdata->axis_map_z = 2;

	tf9->pdata->negate_x = 0;
	tf9->pdata->negate_y = 1;
	tf9->pdata->negate_z = 1;

	printk(KERN_INFO "sensorfix: kxtf9 negate_x/y/z = %d/%d/%d, axis_map_x = %d/%d/%d\n",
	       tf9->pdata->negate_x, tf9->pdata->negate_y, tf9->pdata->negate_z, 
	       tf9->pdata->axis_map_x, tf9->pdata->axis_map_y, tf9->pdata->axis_map_z);

	exit_code = 0;

err_close:
	filp_close(kxtf9, NULL);
	return exit_code;
}

module_init(sensorfix_init);
