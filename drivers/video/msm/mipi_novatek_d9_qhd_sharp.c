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

static int d9_qhd_panel_id = 0;
static int gpio_backlight_en = 0;

#define NT35516_SLEEP_OFF_DELAY 120
#define NT35516_DISPLAY_ON_DELAY 10

/* common setting */
static char get_panel_id[2] = {0xDA, 0x00};
static char exit_sleep[2] = {0x11, 0x00};
static char display_on[2] = {0x29, 0x00};
static char display_off[2] = {0x28, 0x00};
static char enter_sleep[2] = {0x10, 0x00};
static char write_ram[2] = {0x2c, 0x00}; /* write ram */

static struct dsi_cmd_desc novatek_nt35516_qhd_display_off_cmds[] = {
	{DTYPE_DCS_WRITE, 1, 0, 0, 10, sizeof(display_off), display_off},
	{DTYPE_DCS_WRITE, 1, 0, 0, 120, sizeof(enter_sleep), enter_sleep}
};

static struct dsi_cmd_desc novatek_get_d9_panel_id_cmd[] = {
	{DTYPE_DCS_READ, 1, 0, 1, 5, sizeof(get_panel_id), get_panel_id},
};


static char video0[]   = {0xFF, 0xAA, 0x55, 0x25, 1};
static char video2[]   = {0xF3, 2, 3, 7, 0x15};
static char video3[]   = {0xF0, 0x55, 0xAA, 0x52, 8, 0};
static char video4[]   = {0xB1, 0xFC, 0, 0};
static char video6[]   = {0xBC, 0, 0, 0};
static char video44[]  = {0x3A, 0x77};
static char video45[]  = {0x35, 0};

static char video_page1[] = {0xF0, 0x55, 0xAA, 0x52, 8, 1};
static char video_vcom_offset[] = {0xBE, 0x72};

static char cmd0[]     = {0xFF, 0xAA, 0x55, 0x25, 1};
static char cmd2[]     = {0xF3, 2, 3, 7, 0x45};
static char cmd3[]     = {0xF0, 0x55, 0xAA, 0x52, 8, 0};
static char cmd4[]     = {0xB1, 0xEC};
static char cmd26_2[]  = {0xBD, 1, 0x48, 0x10, 0x38, 1};
static char cmd6[]     = {0xBC, 0, 0, 0};
static char cmd44[]    = {0x3A, 7};
static char cmd45[]    = {0x35, 0};


static struct dsi_cmd_desc novatek_sharp_qhd_video_display_on_cmds[] = { 
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, 5, video0},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, 5, video2},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, 6, video3},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, 4, video4},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, 4, video6},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, 2, video44},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, 2, video45},
	{DTYPE_DCS_WRITE, 1, 0, 0, NT35516_SLEEP_OFF_DELAY, sizeof(exit_sleep), exit_sleep},
	{DTYPE_DCS_WRITE, 1, 0, 0, NT35516_DISPLAY_ON_DELAY, sizeof(display_on), display_on},
};

static struct dsi_cmd_desc novatek_sharp_qhd_cmd_display_on_cmds[] = { 
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, 5, cmd0},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, 5, cmd2},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, 6, cmd3},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, 2, cmd4},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, 6, cmd26_2},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, 4, cmd6},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, 2, cmd44},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, 2, cmd45},
	{DTYPE_DCS_WRITE,  1, 0, 0, NT35516_SLEEP_OFF_DELAY, sizeof(exit_sleep), exit_sleep},
	{DTYPE_DCS_WRITE,  1, 0, 0, NT35516_DISPLAY_ON_DELAY,  sizeof(display_on), display_on},
	{DTYPE_DCS_WRITE,  1, 0, 0, 0, sizeof(write_ram), write_ram},
};

static struct dsi_cmd_desc novatek_success_qhd_video_display_on_cmds[] = {
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, 5, video0},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, 5, video2},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, 6, video3},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, 4, video4},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, 4, video6},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, 6, video_page1},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, 2, video_vcom_offset},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, 2, video44},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, 2, video45},
	{DTYPE_DCS_WRITE,  1, 0, 0, NT35516_SLEEP_OFF_DELAY, sizeof(exit_sleep), exit_sleep},
	{DTYPE_DCS_WRITE,  1, 0, 0, NT35516_DISPLAY_ON_DELAY, sizeof(display_on), display_on},
};


static char S93506_set_0xFF[] = {
	0xFF, 0xAA, 0x55, 0x25, 1
};
static char S93506_set_0xF2[] = {
	0xF2, 0, 0, 0x4A, 0xA, 0xA8, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0xB, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x40,
	1, 0x51, 0, 1, 0, 1
};
static char S93506_set_0xF3[] = {
	0xF3, 2, 3, 7, 0x45, 0x88, 0xD1, 0xD
};
static char S93506_set_0xF4[] = {
	0xF4, 0, 0xA
};
static char S93506_set_page0[] = {
	0xF0, 0x55, 0xAA, 0x52, 8, 0
};
static char S93506_set_0xB1[] = {
	0xB1, 0xEC, 0
};
static char S93506_set_0xB8[] = {
	0xB8, 1, 2, 2, 2
};
static char S93506_set_INVCTR[] = {
	0xBC, 0, 0, 0
};
static char S93506_set_0xC9[] = {
	0xC9, 0x63, 6, 0xD, 0x1A, 0x17, 0
};
static char S93506_set_page1[] = {
	0xF0, 0x55, 0xAA, 0x52, 8, 1
};
static char S93506_set_0xB0[] = {
	0xB0, 5, 5, 5
};
static char S93506_p1_0xB1[] = {
	0xB1, 5, 5, 5
};
static char S93506_set_0xB2[] = {
	0xB2, 1, 1, 1
};
static char S93506_set_0xB3[] = {
	0xB3, 0xE, 0xE, 0xE
};
static char S93506_set_0xB4[] = {
	0xB4, 0xA, 0xA, 0xA
};
static char S93506_set_0xB6[] = {
	0xB6, 0x44, 0x44, 0x44
};
static char S93506_set_0xB7[] = {
	0xB7, 0x34, 0x34, 0x34
};
static char S93506_p1_0xB8[] = {
	0xB8, 0x20, 0x20, 0x20
};
static char S93506_set_0xB9[] = {
	0xB9, 0x26, 0x26, 0x26
};
static char S93506_set_0xBA[] = {
	0xBA, 0x24, 0x24, 0x24
};
static char S93506_set_0xBC[] = {
	0xBC, 0, 0xC8, 0
};
static char S93506_set_0xBD[] = {
	0xBD, 0, 0xC8, 0
};
static char S93506_set_0xBE[] = {
	0xBE, 0x80
};
static char S93506_set_0xC0[] = {
	0xC0, 4, 0
};
static char S93506_set_0xCA[] = {
	0xCA, 0
};
static char S93506_set_0xD0[] = {
	0xD0, 0xA, 0x10, 0xD, 0xF
};
static char S93506_set_0xD1[] = {
	0xD1, 0, 0x70, 0, 0x9C, 0, 0xDD, 1, 0xB, 1, 0x1F, 1,
	0x3F, 1, 0x60, 1, 0x8D
};
static char S93506_set_0xD2[] = {
	0xD2, 1, 0xB0, 1, 0xE7, 2, 0x11, 2, 0x52, 2, 0x86,
	2, 0x88, 2, 0xBD, 2, 0xF4
};
static char S93506_set_0xD3[] = {
	0xD3, 3, 0x1B, 3, 0x47, 3, 0x67, 3, 0x87, 3, 0x9E,
	3, 0xB4, 3, 0xC2, 3, 0xD1
};
static char S93506_set_0xD4[] = {
	0xD4, 3, 0xD7, 3, 0xFF
};
static char S93506_set_0xD5[] = {
	0xD5, 0, 0x70, 0, 0x9C, 0, 0xDD, 1, 0xB, 1, 0x1F, 1,
	0x3F, 1, 0x60, 1, 0x8D
};
static char S93506_set_0xD6[] = {
	0xD6, 1, 0xB0, 1, 0xE7, 2, 0x11, 2, 0x52, 2, 0x86,
	2, 0x88, 2, 0xBD, 2, 0xF4
};
static char S93506_set_0xD7[] = {
	0xD7, 3, 0x1B, 3, 0x47, 3, 0x67, 3, 0x87, 3, 0x9E,
	3, 0xB4, 3, 0xC2, 3, 0xD1
};
static char S93506_set_0xD8[] = {
	0xD8, 3, 0xD7, 3, 0xFF
};
static char S93506_set_0xD9[] = {
	0xD9, 0, 0x70, 0, 0x9C, 0, 0xDD, 1, 0xB, 1, 0x1F, 1,
	0x3F, 1, 0x60, 1, 0x8D
};
static char S93506_set_0xDD[] = {
	0xDD, 1, 0xB0, 1, 0xE7, 2, 0x11, 2, 0x52, 2, 0x86,
	2, 0x88, 2, 0xBD, 2, 0xF4
};
static char S93506_set_0xDE[] = {
	0xDE, 3, 0x1B, 3, 0x47, 3, 0x67, 3, 0x87, 3, 0x9E,
	3, 0xB4, 3, 0xC2, 3, 0xD1
};
static char S93506_set_0xDF[] = {
	0xDF, 3, 0xD7, 3, 0xFF
};
static char S93506_set_0xE0[] = {
	0xE0, 0, 0x70, 0, 0x9C, 0, 0xDD, 1, 0xB, 1, 0x1F, 1,
	0x3F, 1, 0x60, 1, 0x8D
};
static char S93506_set_0xE1[] = {
	0xE1, 1, 0xB0, 1, 0xE7, 2, 0x11, 2, 0x52, 2, 0x86,
	2, 0x88, 2, 0xBD, 2, 0xF4
};
static char S93506_set_0xE2[] = {
	0xE2, 3, 0x1B, 3, 0x47, 3, 0x67, 3, 0x87, 3, 0x9E,
	3, 0xB4, 3, 0xC2, 3, 0xD1
};
static char S93506_set_0xE3[] = {
	0xE3, 3, 0xD7, 3, 0xFF
};
static char S93506_set_0xE4[] = {
	0xE4, 0, 0x70, 0, 0x9C, 0, 0xDD, 1, 0xB, 1, 0x1F, 1,
	0x3F, 1, 0x60, 1, 0x8D
};
static char S93506_set_0xE5[] = {
	0xE5, 1, 0xB0, 1, 0xE7, 2, 0x11, 2, 0x52, 2, 0x86,
	2, 0x88, 2, 0xBD, 2, 0xF4
};
static char S93506_set_0xE6[] = {
	0xE6, 3, 0x1B, 3, 0x47, 3, 0x67, 3, 0x87, 3, 0x9E,
	3, 0xB4, 3, 0xC2, 3, 0xD1
};
static char S93506_set_0xE7[] = {
	0xE7, 3, 0xD7, 3, 0xFF
};
static char S93506_set_0xE8[] = {
	0xE8, 0, 0x70, 0, 0x9C, 0, 0xDD, 1, 0xB, 1, 0x1F, 1,
	0x3F, 1, 0x60, 1, 0x8D
};
static char S93506_set_0xE9[] = {
	0xE9, 1, 0xB0, 1, 0xE7, 2, 0x11, 2, 0x52, 2, 0x86,
	2, 0x88, 2, 0xBD, 2, 0xF4
};
static char S93506_set_0xEA[] = {
	0xEA, 3, 0x1B, 3, 0x47, 3, 0x67, 3, 0x87, 3, 0x9E,
	3, 0xB4, 3, 0xC2, 3, 0xD1
};
static char S93506_set_0xEB[] = {
	0xEB, 3, 0xD7, 3, 0xFF
};
static char S93506_set_0x3A[] = {
	0x3A, 0x77
};
static char S93506_set_0x13[] = {
	0x13, 0
};
static char S93506_set_0x29[] = {
	0x29, 0
};
static char S93506_set_0x2C[] = {
	0x2C, 0
};

static struct dsi_cmd_desc novatek_success_qhd_cmd_display_on_cmds[] = { 
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, 5, S93506_set_0xFF},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, 0x24, S93506_set_0xF2},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, 8, S93506_set_0xF3},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, 3, S93506_set_0xF4},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, 6, S93506_set_page0},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, 3, S93506_set_0xB1},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, 5, S93506_set_0xB8},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, 4, S93506_set_INVCTR},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, 7, S93506_set_0xC9},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, 6, S93506_set_page1},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, 4, S93506_set_0xB0},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, 4, S93506_p1_0xB1},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, 4, S93506_set_0xB2},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, 4, S93506_set_0xB3},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, 4, S93506_set_0xB4},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, 4, S93506_set_0xB6},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, 4, S93506_set_0xB7},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, 4, S93506_p1_0xB8},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, 4, S93506_set_0xB9},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, 4, S93506_set_0xBA},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, 4, S93506_set_0xBC},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, 4, S93506_set_0xBD},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, 2, S93506_set_0xBE},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, 3, S93506_set_0xC0},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, 2, S93506_set_0xCA},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, 5, S93506_set_0xD0},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, 0x11, S93506_set_0xD1},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, 0x11, S93506_set_0xD2},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, 0x11, S93506_set_0xD3},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, 5,    S93506_set_0xD4},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, 0x11, S93506_set_0xD5},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, 0x11, S93506_set_0xD6},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, 0x11, S93506_set_0xD7},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, 5,    S93506_set_0xD8},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, 0x11, S93506_set_0xD9},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, 0x11, S93506_set_0xDD},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, 0x11, S93506_set_0xDE},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, 5,    S93506_set_0xDF},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, 0x11, S93506_set_0xE0},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, 0x11, S93506_set_0xE1},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, 0x11, S93506_set_0xE2},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, 5,    S93506_set_0xE3},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, 0x11, S93506_set_0xE4},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, 0x11, S93506_set_0xE5},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, 0x11, S93506_set_0xE6},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, 5,    S93506_set_0xE7},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, 0x11, S93506_set_0xE8},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, 0x11, S93506_set_0xE9},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, 0x11, S93506_set_0xEA},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,    5, S93506_set_0xEB},
	{DTYPE_DCS_WRITE,  1, 0, 0, NT35516_SLEEP_OFF_DELAY, sizeof(exit_sleep), exit_sleep},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,    2, S93506_set_0x3A},
	{DTYPE_DCS_WRITE,  1, 0, 0, 0,    2, S93506_set_0x13},
	{DTYPE_DCS_WRITE,  1, 0, 0, 0xA,  2, S93506_set_0x29},
	{DTYPE_DCS_WRITE,  1, 0, 0, 0,    2, S93506_set_0x2C},
};

static int mipi_novatek_d9_qhd_lcd_on(struct platform_device *pdev)
{
	struct msm_fb_data_type *mfd;
	struct mipi_panel_info *mipi;
	struct dsi_buf *dp;
	struct dsi_cmd_desc *cmds;
	int cnt;
	int ret = 0;

	pr_debug("%s: E\n", __func__);

	mfd = platform_get_drvdata(pdev);
	if (!mfd)
		return -ENODEV;

	if (mfd->key != MFD_KEY)
		return -EINVAL;

	mipi = &mfd->panel_info.mipi;

	if (!mfd->cont_splash_done) {
		mfd->cont_splash_done = 1;
		return 0;
	}

	dp = &d9_novatek_qhd_tx_buf;
	pr_info("%s: mode = %d\n", __func__, mipi->mode);
	if (!d9_qhd_panel_id && mipi->mode == DSI_CMD_MODE) {
		int id;
		mipi_dsi_cmd_bta_sw_trigger();
		cmds = novatek_get_d9_panel_id_cmd;
		cnt = ARRAY_SIZE(novatek_get_d9_panel_id_cmd);
		mipi_dsi_cmds_rx(mfd, dp, &d9_novatek_qhd_rx_buf, cmds, cnt);
		id = *(short *)d9_novatek_qhd_rx_buf.data;
		pr_info("%s: linxc d9 panel id=0x%x, lp=0x%x\n", __func__, id, id);
		d9_qhd_panel_id = id;
	}
	if (d9_qhd_panel_id == 0x22) {
		pr_info("%s: default sku5 sharp panel \n", __func__);
		if (mipi->mode == DSI_VIDEO_MODE) {
			cmds = novatek_sharp_qhd_video_display_on_cmds;
			cnt = ARRAY_SIZE(novatek_sharp_qhd_video_display_on_cmds);
			mipi_dsi_cmds_tx(dp, cmds, cnt);
		}
		if (mipi->mode == DSI_CMD_MODE) {
			cmds = novatek_sharp_qhd_cmd_display_on_cmds;
			cnt = ARRAY_SIZE(novatek_sharp_qhd_cmd_display_on_cmds);
			mipi_dsi_cmds_tx(dp, cmds, cnt);
		}
	} else {
		pr_info("%s: sku5 success panel \n", __func__);
		if (mipi->mode == DSI_VIDEO_MODE) {
			cmds = novatek_success_qhd_video_display_on_cmds;
			cnt = ARRAY_SIZE(novatek_success_qhd_video_display_on_cmds);
			mipi_dsi_cmds_tx(dp, cmds, cnt);
		}
		if (mipi->mode == DSI_CMD_MODE) {
			cmds = novatek_success_qhd_cmd_display_on_cmds;
			cnt = ARRAY_SIZE(novatek_success_qhd_cmd_display_on_cmds);
			mipi_dsi_cmds_tx(dp, cmds, cnt);
		}
	}

	pr_debug("%s: X\n", __func__);
	return ret;
}

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
	.on	= mipi_novatek_d9_qhd_lcd_on,
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
