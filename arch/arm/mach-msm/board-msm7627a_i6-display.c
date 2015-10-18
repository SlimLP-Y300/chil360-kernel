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
#define DEBUG
#include <linux/init.h>
#include <linux/ioport.h>
#include <linux/platform_device.h>
#include <linux/bootmem.h>
#include <linux/regulator/consumer.h>
#include <linux/gpio.h>
#include <asm/mach-types.h>
#include <asm/io.h>
#include <mach/msm_bus_board.h>
#include <mach/msm_memtypes.h>
#include <mach/board.h>
#include <mach/gpiomux.h>
#include <mach/socinfo.h>
#include <mach/rpc_pmapp.h>
#include "devices.h"
#include "board-msm7627a.h"
#include <linux/kernel.h>
#include <linux/module.h>

#ifdef CONFIG_FB_MSM_TRIPLE_BUFFER
#define MSM_FB_SIZE		0x4BF000
#define MSM7x25A_MSM_FB_SIZE    0x1C2000
#define MSM8x25_MSM_FB_SIZE	0x5FA000
#else
#define MSM_FB_SIZE		0x32A000
#define MSM7x25A_MSM_FB_SIZE	0x12C000
#define MSM8x25_MSM_FB_SIZE	0x3FC000
#endif

//linxc 12-06-19 +++
#define MSM8x25_I6_MSM_FB_SIZE  0x5FB000		//add linxc 2012-12-21 +++
#define GPIO_I6_LCD_BRDG_RESET_N	85
#define GPIO_I6_LCD_BACKLIGHT_EN	96
#define GPIO_I6_LCD_VDD_3V3_EN    12
#define GPIO_I6_MIPI_DETECT       107
//linxc 12-06-19 - - -


#define MACHINE_IS_JSR_I6  \
 (machine_is_msm7627a_evb() || machine_is_msm8625_evb() || machine_is_msm8625_qrd5() || machine_is_msm7x27a_qrd5a())

#define MACHINE_IS_JSR_I6Q  \
 (machine_is_msm8625_skud())

/*
 * Reserve enough v4l2 space for a double buffered full screen
 * res image (864x480x1.5x2)
 */
#define MSM_V4L2_VIDEO_OVERLAY_BUF_SIZE 1244160

static unsigned fb_size = MSM_FB_SIZE;
static int __init fb_size_setup(char *p)
{
	fb_size = memparse(p, NULL);
	return 0;
}

early_param("fb_size", fb_size_setup);


static struct resource msm_fb_resources[] = {
	{
		.flags  = IORESOURCE_DMA,
	}
};

#ifdef CONFIG_MSM_V4L2_VIDEO_OVERLAY_DEVICE
static struct resource msm_v4l2_video_overlay_resources[] = {
	{
		.flags = IORESOURCE_DMA,
	}
};
#endif


static int i6_mipi_gpio_initialized = 0;
static int i6_mipi_gpio_status = 0;

#define GPIO_I6_MIPI_DETECT    107

static int I6_mipi_panel_is_novatek_nt35516(void)
{
	unsigned int conf;
	pr_err("%s\n", __func__);
	if (!i6_mipi_gpio_initialized) {
		if (gpio_request(GPIO_I6_MIPI_DETECT, "I6_mipi_detect") < 0) {
			pr_err("%s: gpio_request I6_mipi_detect failed!", __func__);
			return i6_mipi_gpio_status;
		}
		conf = GPIO_CFG(GPIO_I6_MIPI_DETECT, 0, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA);  // 0x6B0
		if (gpio_tlmm_config(conf, GPIO_CFG_ENABLE) >= 0) {
			i6_mipi_gpio_status = gpio_get_value(GPIO_I6_MIPI_DETECT);
			pr_err("%s: is_nt35516 = %d\n", __func__, i6_mipi_gpio_status);
			i6_mipi_gpio_initialized = 1;
		} else {
			pr_err("%s: unable to config I6_mipi_detect\n", __func__);
		}
		gpio_free(GPIO_I6_MIPI_DETECT);
	}
	return i6_mipi_gpio_status;
}

static int msm_fb_detect_panel(const char *name)
{
	int ret = -ENODEV;
	pr_err("%s: name = \"%s\"\n", __func__, name);
	if (MACHINE_IS_JSR_I6) {
		if (!strncmp(name, "mipi_cmd_novatek_nt35516_qhd", 28)) {
			if (I6_mipi_panel_is_novatek_nt35516()) {
				pr_err("I6 LCD is cmd novatek nt35516\n");
				ret = 0;
			}
		}
		if (!strncmp(name, "mipi_cmd_truly_nt35516_qhd", 25)) {
			if (!I6_mipi_panel_is_novatek_nt35516()) {
				pr_err("I6 LCD is truly\n");
				ret = 0;
			}
		}
	} 
	return ret;
}


static struct msm_fb_platform_data msm_fb_pdata = {
	.detect_client = msm_fb_detect_panel,
};

static struct platform_device msm_fb_device = {
	.name   = "msm_fb",
	.id     = 0,
	.num_resources  = ARRAY_SIZE(msm_fb_resources),
	.resource       = msm_fb_resources,
	.dev    = {
		.platform_data = &msm_fb_pdata,
	}
};

#ifdef CONFIG_MSM_V4L2_VIDEO_OVERLAY_DEVICE
static struct platform_device msm_v4l2_video_overlay_device = {
		.name   = "msm_v4l2_overlay_pd",
		.id     = 0,
		.num_resources  = ARRAY_SIZE(msm_v4l2_video_overlay_resources),
		.resource       = msm_v4l2_video_overlay_resources,
	};
#endif


static int i6_bl_level = 0;

static int mipi_I6_qhd_set_bl(int level)
{
	int ret = 0;
	int value = 0;
	ret = pmapp_disp_backlight_set_brightness(level);
	if (ret)
		pr_err("%s: can't set lcd backlight! \n", __func__);
	if (level == i6_bl_level) {
		pr_err("%s: no change\n", __func__);
		return 0;
	}
	if (level == 255) {
		value = 0;
	} else {
		if (i6_bl_level != 255) {
			i6_bl_level = level;
			return 0;
		}
		msleep(25);
		value = 1;
	}
	gpio_set_value(GPIO_I6_LCD_BACKLIGHT_EN, value);
	i6_bl_level = level;
	return 0;
}

// -------------------------------------------------------------------

static int mipi_truly_nt35516_qhd_rotate_panel(void)
{
	int rotate = 0;
	if (machine_is_msm8625_qrd5() || machine_is_msm7x27a_qrd5a())
		rotate = 1;
	return rotate;
}

static int mipi_nt35516_qhd_rotate_panel(void)
{
	int rotate = 0;
	if (machine_is_msm8625_qrd5() || machine_is_msm7x27a_qrd5a())
		rotate = 1;
	return rotate;
}

// -------------------------------------------------------------------

static struct msm_panel_common_pdata mipi_truly_nt35516_qhd_pdata = {
	.pmic_backlight = NULL,
	.rotate_panel = mipi_truly_nt35516_qhd_rotate_panel,
};

static struct platform_device mipi_dsi_truly_nt35516_qhd_panel_device = {
	.name   = "mipi_truly_nt35516_qhd",
	.id     = 0,
	.dev    = {
		.platform_data = &mipi_truly_nt35516_qhd_pdata,
	}
};

// -------------------------------------------------------------------

static struct msm_panel_common_pdata mipi_novatek_nt35516_qhd_pdata = {
	.pmic_backlight = NULL,
	.rotate_panel = mipi_nt35516_qhd_rotate_panel,
};

static struct platform_device mipi_dsi_novatek_nt35516_qhd_panel_device = {
	.name   = "mipi_novatek_nt35516_qhd",
	.id     = 0,
	.dev    = {
		.platform_data = &mipi_novatek_nt35516_qhd_pdata,
	}
};

// -------------------------------------------------------------------

static struct msm_panel_common_pdata mipi_video_nt35516_qhd_qhd_v2_pdata = {
	.pmic_backlight = mipi_I6_qhd_set_bl,
	.rotate_panel = mipi_truly_nt35516_qhd_rotate_panel,
};

static struct platform_device mipi_dsi_truly_nt35516_qhd_v2_panel_device = {
	.name   = "mipi_truly_nt35516_qhd",
	.id     = 0,
	.dev    = {
		.platform_data = &mipi_video_nt35516_qhd_qhd_v2_pdata,
	}
};

// -------------------------------------------------------------------

static struct msm_panel_common_pdata mipi_novatek_nt35516_qhd_v2_pdata = {
	.pmic_backlight = mipi_I6_qhd_set_bl,
	.rotate_panel = mipi_nt35516_qhd_rotate_panel,
};

static struct platform_device mipi_dsi_novatek_nt35516_qhd_v2_panel_device = {
	.name   = "mipi_novatek_nt35516_qhd",
	.id     = 0,
	.dev    = {
		.platform_data = &mipi_novatek_nt35516_qhd_v2_pdata,
	}
};

// -------------------------------------------------------------------

static struct platform_device *evb_fb_devices[] __initdata = {
	&msm_fb_device,
	//&mipi_dsi_NT35510_panel_device,
	//&mipi_dsi_NT35516_panel_device,
	&mipi_dsi_truly_nt35516_qhd_panel_device,
	&mipi_dsi_novatek_nt35516_qhd_panel_device,
};

static struct platform_device *evb2_fb_devices[] __initdata = {
	&msm_fb_device,
	&mipi_dsi_truly_nt35516_qhd_v2_panel_device,
	&mipi_dsi_novatek_nt35516_qhd_v2_panel_device,
};

// -------------------------------------------------------------------

void __init msm_msm7627a_allocate_memory_regions(void)
{
	void *addr;
	unsigned long fb_size = MSM_FB_SIZE;

	if (machine_is_msm7625a_surf() || machine_is_msm7625a_ffa()) {
		fb_size = MSM7x25A_MSM_FB_SIZE;
	} else
	if (MACHINE_IS_JSR_I6) {
		fb_size = MSM8x25_MSM_FB_SIZE;
		if (machine_is_msm8625_qrd5()) 
			fb_size = MSM8x25_I6_MSM_FB_SIZE;
	} else
	if (machine_is_msm8625q_skud()) {
		fb_size = MSM8x25_MSM_FB_SIZE;
	}

	addr = alloc_bootmem_align(fb_size, 0x1000);
	msm_fb_resources[0].start = __pa(addr);
	msm_fb_resources[0].end = msm_fb_resources[0].start + fb_size - 1;
	pr_info("allocating %lu bytes at %p (%lx physical) for fb\n", fb_size, addr, __pa(addr));

#ifdef CONFIG_MSM_V4L2_VIDEO_OVERLAY_DEVICE
	fb_size = MSM_V4L2_VIDEO_OVERLAY_BUF_SIZE;
	addr = alloc_bootmem_align(fb_size, 0x1000);
	msm_v4l2_video_overlay_resources[0].start = __pa(addr);
	msm_v4l2_video_overlay_resources[0].end =
		msm_v4l2_video_overlay_resources[0].start + fb_size - 1;
	pr_debug("allocating %lu bytes at %p (%lx physical) for v4l2\n",
		fb_size, addr, __pa(addr));
#endif

}

static struct msm_panel_common_pdata mdp_pdata = {
	.gpio = 97,
	.mdp_rev = MDP_REV_303,
	.cont_splash_enabled = 0x1,
};

#define GPIO_LCDC_BRDG_PD	128
#define GPIO_LCDC_BRDG_RESET_N	129
#define GPIO_LCD_DSI_SEL	125
#define LCDC_RESET_PHYS		0x90008014

enum {
	DSI_SINGLE_LANE = 1,
	DSI_TWO_LANES,
};

static int msm_fb_get_lane_config(void)
{
	/* For MSM7627A SURF/FFA and QRD */
	int rc = DSI_TWO_LANES;
	if (machine_is_msm7625a_surf() || machine_is_msm7625a_ffa()) {
		rc = DSI_SINGLE_LANE;
		pr_info("DSI_SINGLE_LANES\n");
	} else {
		pr_info("DSI_TWO_LANES\n");
	}
	return rc;
}

static int msm_fb_dsi_client_I6_reset(void)
{
	int rc = 0;
	pr_err("linxc: %s \n", __func__);
	rc = gpio_request(GPIO_I6_LCD_BRDG_RESET_N, "i6_lcd_brdg_reset_n");
	if (rc < 0)
		pr_err("%s: failed to request i6 lcd brdg reset_n\n", __func__);
	return rc;
}

static int msm_fb_dsi_client_reset(void)
{
	int rc = 0;
	if (MACHINE_IS_JSR_I6)
		rc = msm_fb_dsi_client_I6_reset();
	return rc;
}

static struct regulator_bulk_data regs_dsi[] = {
	{ .supply = "gp2",   .min_uV = 2850000, .max_uV = 2850000 },
	{ .supply = "msme1", .min_uV = 1800000, .max_uV = 1800000 },
};

static int dsi_gpio_initialized;

/* Common EXT power control for QRD */
static struct regulator *gpio_reg_2p85v;
static struct regulator *gpio_reg_1p8v;
//static struct regulator *gpio_reg_3p3v;

static int mipi_dsi_panel_msm_power(int on)
{
	int rc = 0;

	/* I2C-controlled GPIO Expander -init of the GPIOs very late */
	if (unlikely(!dsi_gpio_initialized)) {
		pmapp_disp_backlight_init();

		rc = gpio_request(GPIO_DISPLAY_PWR_EN, "gpio_disp_pwr");
		if (rc < 0) {
			pr_err("failed to request gpio_disp_pwr\n");
			return rc;
		}
		rc = regulator_bulk_get(NULL, ARRAY_SIZE(regs_dsi), regs_dsi);
		if (rc) {
			pr_err("%s: could not get regulators: %d\n",
					__func__, rc);
			goto fail_gpio2;
		}

		rc = regulator_bulk_set_voltage(ARRAY_SIZE(regs_dsi),
						regs_dsi);
		if (rc) {
			pr_err("%s: could not set voltages: %d\n",
					__func__, rc);
			goto fail_vreg;
		}
		if (pmapp_disp_backlight_set_brightness(100))
			pr_err("%s: backlight set brightness failed\n", __func__);

		dsi_gpio_initialized = 1;
	}

	if (on) {
		gpio_set_value_cansleep(GPIO_LCDC_BRDG_PD, 0);
		gpio_set_value_cansleep(GPIO_LCDC_BRDG_RESET_N, 0);
		msleep(20);
		gpio_set_value_cansleep(GPIO_LCDC_BRDG_RESET_N, 1);
		msleep(20);
	} else {
		gpio_set_value_cansleep(GPIO_LCDC_BRDG_PD, 1);
	}

	rc = on ? regulator_bulk_enable(ARRAY_SIZE(regs_dsi), regs_dsi) :
		  regulator_bulk_disable(ARRAY_SIZE(regs_dsi), regs_dsi);

	if (rc)
		pr_err("%s: could not %sable regulators: %d\n",
				__func__, on ? "en" : "dis", rc);

	return rc;
fail_vreg:
	regulator_bulk_free(ARRAY_SIZE(regs_dsi), regs_dsi);
fail_gpio2:
//	gpio_free(GPIO_BACKLIGHT_EN);
//fail_gpio1:
	gpio_free(GPIO_DISPLAY_PWR_EN);
	dsi_gpio_initialized = 0;
	return rc;
}

static char mipi_dsi_splash_is_enabled(void);

static int mipi_dsi_panel_power(int on)
{
	return mipi_dsi_panel_msm_power(on);
}

#define MDP_303_VSYNC_GPIO 97

#ifdef CONFIG_FB_MSM_MIPI_DSI
static struct mipi_dsi_platform_data mipi_dsi_pdata = {
	.vsync_gpio         = MDP_303_VSYNC_GPIO,
	.dsi_power_save     = mipi_dsi_panel_power,
	.dsi_client_reset   = msm_fb_dsi_client_reset,
	.get_lane_config    = msm_fb_get_lane_config,
	.splash_is_enabled  = mipi_dsi_splash_is_enabled,
};
#endif

static char mipi_dsi_splash_is_enabled(void)
{
	return mdp_pdata.cont_splash_enabled;
}

static char prim_panel_name[PANEL_NAME_MAX_LEN];
static int __init prim_display_setup(char *param)
{
	if (strnlen(param, PANEL_NAME_MAX_LEN))
		strlcpy(prim_panel_name, param, PANEL_NAME_MAX_LEN);
	return 0;
}
early_param("prim_display", prim_display_setup);

static int disable_splash;

void msm7x27a_set_display_params(char *prim_panel)
{
	int n0, n1;
	const char * pn0 = "mipi_cmd_novatek_nt35516_qhd";
	const char * pn1 = "mipi_cmd_truly_nt35516_qhd";
	char * ppn = (char *)msm_fb_pdata.prim_panel_name;
	if (strnlen(prim_panel, PANEL_NAME_MAX_LEN)) {
		strlcpy(ppn, prim_panel, PANEL_NAME_MAX_LEN);
		pr_err("msm_fb_pdata.prim_panel_name = %s\n", ppn);
	}
	if (strnlen(ppn, PANEL_NAME_MAX_LEN)) {
		n0 = strncmp(ppn, pn0, strlen(pn0));
		n1 = strncmp(ppn, pn1, strlen(pn1));
		if (n0 && n1)
			disable_splash = 1;
	}
}

void __init msm_fb_add_devices(void)
{
	int rc = 0;

	msm7x27a_set_display_params(prim_panel_name);

	if (MACHINE_IS_JSR_I6) {
		if (disable_splash)
			mdp_pdata.cont_splash_enabled = 0x0;
		if (socinfo_get_platform_version() == 0x10000) {
			platform_add_devices(evb_fb_devices, ARRAY_SIZE(evb_fb_devices));
		}
		if (socinfo_get_platform_version() == 0x20000) {
			platform_add_devices(evb2_fb_devices, ARRAY_SIZE(evb2_fb_devices));
		}
	}

	msm_fb_register_device("mdp", &mdp_pdata);
	
#ifdef CONFIG_FB_MSM_MIPI_DSI
	msm_fb_register_device("mipi_dsi", &mipi_dsi_pdata);
#endif
	if (MACHINE_IS_JSR_I6) {
		gpio_reg_2p85v = regulator_get(&mipi_dsi_device.dev, "lcd_vdd");
		if (IS_ERR(gpio_reg_2p85v))
			pr_err("%s:ext_2p85v regulator get failed", __func__);

		gpio_reg_1p8v = regulator_get(&mipi_dsi_device.dev, "lcd_vddi");
		if (IS_ERR(gpio_reg_1p8v))
			pr_err("%s:ext_1p8v regulator get failed", __func__);

		if (mdp_pdata.cont_splash_enabled) {
			/*Enable EXT_2.85 and 1.8 regulators*/
			rc = regulator_enable(gpio_reg_2p85v);
			if (rc < 0)
				pr_err("%s: reg enable failed\n", __func__);
			rc = regulator_enable(gpio_reg_1p8v);
			if (rc < 0)
				pr_err("%s: reg enable failed\n", __func__);
		}
	}
}
