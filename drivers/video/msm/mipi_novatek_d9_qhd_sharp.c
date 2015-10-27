/* Copyright (c) 2012, Code Aurora Forum. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
//#define DEBUG

#include <linux/delay.h>
#include <linux/module.h>
#include <mach/gpio.h>
#include <mach/pmic.h>
#include <mach/socinfo.h>

#include "msm_fb.h"
#include "mipi_dsi.h"
#include "mipi_NT35516.h"

static struct msm_panel_common_pdata *mipi_novatek_sharp_qhd_pdata;
static struct dsi_buf d9_novatek_qhd_tx_buf;
static struct dsi_buf d9_novatek_qhd_rx_buf;

//static int d9_qhd_panel_id = 0;
static int gpio_backlight_en = 0;

#define NT35516_SLEEP_OFF_DELAY 120
#define NT35516_DISPLAY_ON_DELAY 40
#define NT35516_V2_DISPLAY_ON_DELAY 10

/* common setting */
//static char nt_exit_sleep[2] = {0x11, 0x00};
//static char nt_display_on[2] = {0x29, 0x00};
static char nt_display_off[2] = {0x28, 0x00};
static char nt_enter_sleep[2] = {0x10, 0x00};
//static char write_ram[2] = {0x2c, 0x00}; /* write ram */

static struct dsi_cmd_desc novatek_nt35516_qhd_display_off_cmds[] = {
	{DTYPE_DCS_WRITE, 1, 0, 0, 10, sizeof(nt_display_off), nt_display_off},
	{DTYPE_DCS_WRITE, 1, 0, 0, 120, sizeof(nt_enter_sleep), nt_enter_sleep}
};

/*
static int mipi_novatek_d9_qhd_lcd_on(struct platform_device *pdev)
{
	struct msm_fb_data_type *mfd;
	struct mipi_panel_info *mipi;
	//struct dsi_buf *dp;
	//struct dsi_cmd_desc *cmds;
	int ret = 0;

	pr_debug("%s: E\n", __func__);

	mfd = platform_get_drvdata(pdev);
	if (!mfd)
		return -ENODEV;

	if (mfd->key != MFD_KEY)
		return -EINVAL;

	mipi  = &mfd->panel_info.mipi;

	if (!mfd->cont_splash_done) {
		mfd->cont_splash_done = 1;
		return 0;
	}

	// fixme

	if (ret == 0)
		mipi_dsi_cmd_bta_sw_trigger();

	pr_debug("%s: X\n", __func__);
	return ret;
}
*/
static int mipi_novatek_d9_qhd_lcd_off(struct platform_device *pdev)
{
	struct msm_fb_data_type *mfd;

	pr_debug("%s: E\n", __func__);

	mfd = platform_get_drvdata(pdev);

	if (!mfd)
		return -ENODEV;
	if (mfd->key != MFD_KEY)
		return -EINVAL;

	pr_info("%s: mipi_dsi_cmds_tx ...\n", __func__);
	mipi_dsi_cmds_tx(&d9_novatek_qhd_tx_buf, novatek_nt35516_qhd_display_off_cmds,
			ARRAY_SIZE(novatek_nt35516_qhd_display_off_cmds));

	pr_debug("%s: X\n", __func__);
	return 0;
}
/*
static ssize_t mipi_novatek_sharp_wta_bl_ctrl(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	ssize_t ret = strnlen(buf, PAGE_SIZE);
	int err;

	err =  kstrtoint(buf, 0, &mipi_novatek_sharp_bl_ctrl);
	if (err)
		return ret;

	pr_info("%s: bl ctrl set to %d\n", __func__, mipi_novatek_sharp_bl_ctrl);

	return ret;
}

static DEVICE_ATTR(bl_ctrl, S_IWUSR, NULL, mipi_novatek_sharp_wta_bl_ctrl);

static struct attribute *mipi_novatek_sharp_fs_attrs[] = {
	&dev_attr_bl_ctrl.attr,
	NULL,
};

static struct attribute_group mipi_nt35516_fs_attr_group = {
	.attrs = mipi_nt35516_fs_attrs,
};

static int mipi_novatek_sharp_create_sysfs(struct platform_device *pdev)
{
	int rc;
	struct msm_fb_data_type *mfd = platform_get_drvdata(pdev);

	if (!mfd) {
		pr_err("%s: mfd not found\n", __func__);
		return -ENODEV;
	}
	if (!mfd->fbi) {
		pr_err("%s: mfd->fbi not found\n", __func__);
		return -ENODEV;
	}
	if (!mfd->fbi->dev) {
		pr_err("%s: mfd->fbi->dev not found\n", __func__);
		return -ENODEV;
	}
	rc = sysfs_create_group(&mfd->fbi->dev->kobj,
		&mipi_novatek_sharp_fs_attr_group);
	if (rc) {
		pr_err("%s: sysfs group creation failed, rc=%d\n",
			__func__, rc);
		return rc;
	}

	return 0;
}
*/
static int __devinit mipi_novatek_sharp_qhd_lcd_probe(struct platform_device *pdev)
{
	struct platform_device *pthisdev = NULL;
	pr_info("%s: pdev->id = %d\n", __func__, pdev->id);

	if (pdev->id == 0) {
		mipi_novatek_sharp_qhd_pdata = pdev->dev.platform_data;
		if (mipi_novatek_sharp_qhd_pdata)
			gpio_backlight_en = mipi_novatek_sharp_qhd_pdata->gpio;
		return 0;
	}

	pthisdev = msm_fb_add_device(pdev);
	//mipi_novatek_sharp_create_sysfs(pthisdev);

	return 0;
}

static struct platform_driver this_driver = {
	.probe  = mipi_novatek_sharp_qhd_lcd_probe,
	.driver = {
		.name = "mipi_novatek_sharp_qhd",
	},
};

static void mipi_d9_qhd_set_backlight(struct msm_fb_data_type *mfd)
{
	int bl_level = mfd->bl_level;
	
	if (mipi_novatek_sharp_qhd_pdata && mipi_novatek_sharp_qhd_pdata->pmic_backlight) {
		mipi_novatek_sharp_qhd_pdata->pmic_backlight(bl_level);
	} else {
		pr_err("%s: Backlight level set failed\n", __func__);
	}
}

static struct msm_fb_panel_data novatek_sharp_qhd_panel_data = {
	//.on	= mipi_novatek_d9_qhd_lcd_on,
	.off = mipi_novatek_d9_qhd_lcd_off,
	.set_backlight = mipi_d9_qhd_set_backlight,
};

static int ch_used[3];

static int mipi_novatek_sharp_qhd_lcd_init(void)
{
	mipi_dsi_buf_alloc(&d9_novatek_qhd_tx_buf, DSI_BUF_SIZE);
	mipi_dsi_buf_alloc(&d9_novatek_qhd_rx_buf, DSI_BUF_SIZE);

	return platform_driver_register(&this_driver);
}

int mipi_novatek_sharp_qhd_device_register(struct msm_panel_info *pinfo,
					u32 channel, u32 panel)
{
	struct platform_device *pdev = NULL;
	int ret;

	pr_err("%s\n", __func__);
	if ((channel >= 3) || ch_used[channel])
		return -ENODEV;

	ch_used[channel] = TRUE;

	ret = mipi_novatek_sharp_qhd_lcd_init();
	if (ret) {
		pr_err("%s: failed with ret %u\n", __func__, ret);
		return ret;
	}

	pdev = platform_device_alloc("mipi_novatek_sharp_qhd", (panel << 8)|channel);
	if (!pdev)
		return -ENOMEM;

	novatek_sharp_qhd_panel_data.panel_info = *pinfo;
	ret = platform_device_add_data(pdev, &novatek_sharp_qhd_panel_data,
				sizeof(novatek_sharp_qhd_panel_data));
	if (ret) {
		pr_debug("%s: platform_device_add_data failed!\n", __func__);
		goto err_device_put;
	}

	ret = platform_device_add(pdev);
	if (ret) {
		pr_debug("%s: platform_device_register failed!\n", __func__);
		goto err_device_put;
	}

	return 0;

err_device_put:
	platform_device_put(pdev);
	return ret;
}
