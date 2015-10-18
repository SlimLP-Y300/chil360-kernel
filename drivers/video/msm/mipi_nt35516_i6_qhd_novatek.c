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

#include <linux/delay.h>
#include <linux/module.h>
#include <mach/gpio.h>
#include <mach/pmic.h>
#include <mach/socinfo.h>

#include "msm_fb.h"
#include "mipi_dsi.h"
#include "mipi_NT35516.h"

static struct msm_panel_common_pdata *mipi_novatek_nt35516_pdata;
static struct dsi_buf novatek_nt35516_qhd_tx_buf;
static struct dsi_buf novatek_nt35516_qhd_rx_buf;

//static int mipi_nt35516_bl_ctrl = 0;
static int gpio_backlight_en = 0;

#define NT35516_SLEEP_OFF_DELAY 120
#define NT35516_DISPLAY_ON_DELAY 40
#define NT35516_V2_DISPLAY_ON_DELAY 10

/* common setting */
static char nt_exit_sleep[2] = {0x11, 0x00};
static char nt_display_on[2] = {0x29, 0x00};
static char nt_display_off[2] = {0x28, 0x00};
static char nt_enter_sleep[2] = {0x10, 0x00};
//static char write_ram[2] = {0x2c, 0x00}; /* write ram */

static struct dsi_cmd_desc nt35516_display_off_cmds[] = {
	{DTYPE_DCS_WRITE, 1, 0, 0, 10, sizeof(nt_display_off), nt_display_off},
	{DTYPE_DCS_WRITE, 1, 0, 0, 120, sizeof(nt_enter_sleep), nt_enter_sleep}
};


static char cmd0[5] = {
  0xFF, 0xAA, 0x55, 0x25, 0x01, 
};
static char cmd1[36] = {
  0xF2, 0x00, 0x00, 0x4A, 0x0A, 0xA8, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x0B, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x01, 0x51, 
  0x00, 0x01, 0x00, 0x01, 
};
static char cmd2[8] = {
  0xF3, 0x02, 0x03, 0x07, 0x45, 0x88, 0xD4, 0x0D, 
};
static char cmd3[6] = {
  0xF0, 0x55, 0xAA, 0x52, 0x08, 0x00, 
};
static char cmd4[4] = {
  0xB1, 0xEC, 0x00, 0x00, 
};
static char cmd5[3] = {
  0xB7, 0x00, 0x00, 
};
static char cmd6[5] = {
  0xB8, 0x01, 0x02, 0x02, 0x02, 
};
static char cmd7[4] = {
  0xBB, 0x63, 0x03, 0x63, 
};
static char cmd8[4] = {
  0xBC, 0x00, 0x00, 0x00, 
};
static char cmd9[7] = {
  0xC9, 0x63, 0x06, 0x0D, 0x1A, 0x17, 0x00, 
};
static char cmd10[6] = {
  0xBD, 0x01, 0x38, 0x10, 0x38, 0x01, 
};
static char cmd11[2] = {
  0xD0, 0x01, 
};
static char cmd12[6] = {
  0xF0, 0x55, 0xAA, 0x52, 0x08, 0x01, 
};
static char cmd13[4] = {
  0xB0, 0x05, 0x05, 0x05, 
};
static char cmd14[4] = {
  0xB1, 0x05, 0x05, 0x05, 
};
static char cmd15[4] = {
  0xB2, 0x01, 0x01, 0x01, 
};
static char cmd16[4] = {
  0xB3, 0x08, 0x08, 0x08, 
};
static char cmd17[4] = {
  0xB4, 0x09, 0x09, 0x09, 
};
static char cmd18[4] = {
  0xB6, 0x44, 0x44, 0x44, 
};
static char cmd19[4] = {
  0xB7, 0x34, 0x34, 0x34, 
};
static char cmd20[4] = {
  0xB8, 0x20, 0x20, 0x20, 
};
static char cmd21[4] = {
  0xB9, 0x27, 0x27, 0x27, 
};
static char cmd22[4] = {
  0xBA, 0x26, 0x26, 0x26, 
};
static char cmd23[4] = {
  0xBC, 0x00, 0xC8, 0x00, 
};
static char cmd24[4] = {
  0xBD, 0x00, 0xC8, 0x00, 
};
static char cmd25[2] = {
  0xBE, 0x63, 
};
static char cmd26[3] = {
  0xC0, 0x00, 0x08, 
};
static char cmd27[2] = {
  0xCA, 0x00, 
};
static char cmd28[5] = {
  0xD0, 0x0A, 0x10, 0x0D, 0x0F, 
};
static char cmd29[17] = {
  0xD1, 0x00, 0x70, 0x00, 0xCE, 0x00, 0xF7, 0x01, 
  0x10, 0x01, 0x21, 0x01, 0x44, 0x01, 0x62, 0x01, 
  0x8D, 
};
static char cmd30[17] = {
  0xD5, 0x00, 0x70, 0x00, 0xCE, 0x00, 0xF7, 0x01, 
  0x10, 0x01, 0x21, 0x01, 0x44, 0x01, 0x62, 0x01, 
  0x8D, 
};
static char cmd31[17] = {
  0xD9, 0x00, 0x70, 0x00, 0xCE, 0x00, 0xF7, 0x01, 
  0x10, 0x01, 0x21, 0x01, 0x44, 0x01, 0x62, 0x01, 
  0x8D, 
};
static char cmd32[17] = {
  0xE0, 0x00, 0x70, 0x00, 0xCE, 0x00, 0xF7, 0x01, 
  0x10, 0x01, 0x21, 0x01, 0x44, 0x01, 0x62, 0x01, 
  0x8D, 
};
static char cmd33[17] = {
  0xE4, 0x00, 0x70, 0x00, 0xCE, 0x00, 0xF7, 0x01, 
  0x10, 0x01, 0x21, 0x01, 0x44, 0x01, 0x62, 0x01, 
  0x8D, 
};
static char cmd34[17] = {
  0xE8, 0x00, 0x70, 0x00, 0xCE, 0x00, 0xF7, 0x01, 
  0x10, 0x01, 0x21, 0x01, 0x44, 0x01, 0x62, 0x01, 
  0x8D, 
};
static char cmd35[17] = {
  0xD2, 0x01, 0xAF, 0x01, 0xE4, 0x02, 0x0C, 0x02, 
  0x4D, 0x02, 0x82, 0x02, 0x84, 0x02, 0xB8, 0x02, 
  0xF0, 
};
static char cmd36[17] = {
  0xD6, 0x01, 0xAF, 0x01, 0xE4, 0x02, 0x0C, 0x02, 
  0x4D, 0x02, 0x82, 0x02, 0x84, 0x02, 0xB8, 0x02, 
  0xF0, 
};
static char cmd37[17] = {
  0xDD, 0x01, 0xAF, 0x01, 0xE4, 0x02, 0x0C, 0x02, 
  0x4D, 0x02, 0x82, 0x02, 0x84, 0x02, 0xB8, 0x02, 
  0xF0, 
};
static char cmd38[17] = {
  0xE1, 0x01, 0xAF, 0x01, 0xE4, 0x02, 0x0C, 0x02, 
  0x4D, 0x02, 0x82, 0x02, 0x84, 0x02, 0xB8, 0x02, 
  0xF0, 
};
static char cmd39[17] = {
  0xE5, 0x01, 0xAF, 0x01, 0xE4, 0x02, 0x0C, 0x02, 
  0x4D, 0x02, 0x82, 0x02, 0x84, 0x02, 0xB8, 0x02, 
  0xF0, 
};
static char cmd40[17] = {
  0xE9, 0x01, 0xAF, 0x01, 0xE4, 0x02, 0x0C, 0x02, 
  0x4D, 0x02, 0x82, 0x02, 0x84, 0x02, 0xB8, 0x02, 
  0xF0, 
};
static char cmd41[17] = {
  0xD3, 0x03, 0x14, 0x03, 0x40, 0x03, 0x58, 0x03, 
  0x70, 0x03, 0x80, 0x03, 0x90, 0x03, 0xA0, 0x03, 
  0xB0, 
};
static char cmd42[17] = {
  0xD7, 0x03, 0x14, 0x03, 0x40, 0x03, 0x58, 0x03, 
  0x70, 0x03, 0x80, 0x03, 0x90, 0x03, 0xA0, 0x03, 
  0xB0, 
};
static char cmd43[17] = {
  0xDE, 0x03, 0x14, 0x03, 0x40, 0x03, 0x58, 0x03, 
  0x70, 0x03, 0x80, 0x03, 0x90, 0x03, 0xA0, 0x03, 
  0xB0, 
};
static char cmd44[17] = {
  0xE2, 0x03, 0x14, 0x03, 0x40, 0x03, 0x58, 0x03, 
  0x70, 0x03, 0x80, 0x03, 0x90, 0x03, 0xA0, 0x03, 
  0xB0, 
};
static char cmd45[17] = {
  0xE6, 0x03, 0x14, 0x03, 0x40, 0x03, 0x58, 0x03, 
  0x70, 0x03, 0x80, 0x03, 0x90, 0x03, 0xA0, 0x03, 
  0xB0, 
};
static char cmd46[17] = {
  0xEA, 0x03, 0x14, 0x03, 0x40, 0x03, 0x58, 0x03, 
  0x70, 0x03, 0x80, 0x03, 0x90, 0x03, 0xA0, 0x03, 
  0xB0, 
};
static char cmd47[5] = {
  0xD4, 0x03, 0xFD, 0x03, 0xFF, 
};
static char cmd48[5] = {
  0xD8, 0x03, 0xFD, 0x03, 0xFF, 
};
static char cmd49[5] = {
  0xDF, 0x03, 0xFD, 0x03, 0xFF, 
};
static char cmd50[5] = {
  0xE3, 0x03, 0xFD, 0x03, 0xFF, 
};
static char cmd51[5] = {
  0xE7, 0x03, 0xFD, 0x03, 0xFF, 
};
static char cmd52[5] = {
  0xEB, 0x03, 0xFD, 0x03, 0xFF, 
};
static char cmd53[2] = {
  0x35, 0x00, 
};
static char cmd54[3] = {
  0x44, 0x01, 0x3F, 
};
static char cmd55[2] = {
  0x3A, 0x77, 
};
static char cmd58[2] = {
  0x36, 0xD0, 
};
static char cmd59[2] = {
  0x53, 0x24, 
};
static char cmd60[2] = {
  0x55, 0x00, 
};

static struct dsi_cmd_desc novatek_nt35516_qhd_cmd_display_on_cmds[] = {
  {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(cmd0), cmd0},
  {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(cmd1), cmd1},
  {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(cmd2), cmd2},
  {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(cmd3), cmd3},
  {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(cmd4), cmd4},
  {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(cmd5), cmd5},
  {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(cmd6), cmd6},
  {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(cmd7), cmd7},
  {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(cmd8), cmd8},
  {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(cmd9), cmd9},
  {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(cmd10), cmd10},
  {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(cmd11), cmd11},
  {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(cmd12), cmd12},
  {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(cmd13), cmd13},
  {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(cmd14), cmd14},
  {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(cmd15), cmd15},
  {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(cmd16), cmd16},
  {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(cmd17), cmd17},
  {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(cmd18), cmd18},
  {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(cmd19), cmd19},
  {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(cmd20), cmd20},
  {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(cmd21), cmd21},
  {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(cmd22), cmd22},
  {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(cmd23), cmd23},
  {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(cmd24), cmd24},
  {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(cmd25), cmd25},
  {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(cmd26), cmd26},
  {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(cmd27), cmd27},
  {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(cmd28), cmd28},
  {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(cmd29), cmd29},
  {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(cmd30), cmd30},
  {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(cmd31), cmd31},
  {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(cmd32), cmd32},
  {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(cmd33), cmd33},
  {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(cmd34), cmd34},
  {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(cmd35), cmd35},
  {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(cmd36), cmd36},
  {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(cmd37), cmd37},
  {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(cmd38), cmd38},
  {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(cmd39), cmd39},
  {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(cmd40), cmd40},
  {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(cmd41), cmd41},
  {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(cmd42), cmd42},
  {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(cmd43), cmd43},
  {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(cmd44), cmd44},
  {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(cmd45), cmd45},
  {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(cmd46), cmd46},
  {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(cmd47), cmd47},
  {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(cmd48), cmd48},
  {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(cmd49), cmd49},
  {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(cmd50), cmd50},
  {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(cmd51), cmd51},
  {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(cmd52), cmd52},
  {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(cmd53), cmd53},
  {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(cmd54), cmd54},
  {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(cmd55), cmd55},
  {DTYPE_DCS_WRITE, 1, 0, 0, NT35516_SLEEP_OFF_DELAY, sizeof(nt_exit_sleep), nt_exit_sleep},
  {DTYPE_DCS_WRITE, 1, 0, 0, NT35516_DISPLAY_ON_DELAY, sizeof(nt_display_on), nt_display_on},
  {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(cmd58), cmd58},
  {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(cmd59), cmd59},
  {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(cmd60), cmd60},
};



static char cmd2_0[5] = {
  0xFF, 0xAA, 0x55, 0x25, 0x01, 
};
static char cmd2_1[36] = {
  0xF2, 0x00, 0x00, 0x4A, 0x0A, 0xA8, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x0B, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x01, 0x51, 
  0x00, 0x01, 0x00, 0x01, 
};
static char cmd2_2[8] = {
  0xF3, 0x02, 0x03, 0x07, 0x45, 0x88, 0xD4, 0x0D, 
};
static char cmd2_3[6] = {
  0xF0, 0x55, 0xAA, 0x52, 0x08, 0x00, 
};
static char cmd2_4[4] = {
  0xB1, 0xEC, 0x00, 0x00, 
};
static char cmd2_5[3] = {
  0xB7, 0x00, 0x00, 
};
static char cmd2_6[5] = {
  0xB8, 0x01, 0x02, 0x02, 0x02, 
};
static char cmd2_7[4] = {
  0xBB, 0x63, 0x03, 0x63, 
};
static char cmd2_8[4] = {
  0xBC, 0x00, 0x00, 0x00, 
};
static char cmd2_9[7] = {
  0xC9, 0x63, 0x06, 0x0D, 0x1A, 0x17, 0x00, 
};
static char cmd2_10[6] = {
  0xBD, 0x01, 0x38, 0x10, 0x38, 0x01, 
};
static char cmd2_11[2] = {
  0xD0, 0x01, 
};
static char cmd2_12[6] = {
  0xF0, 0x55, 0xAA, 0x52, 0x08, 0x01, 
};
static char cmd2_13[4] = {
  0xB0, 0x05, 0x05, 0x05, 
};
static char cmd2_14[4] = {
  0xB1, 0x05, 0x05, 0x05, 
};
static char cmd2_15[4] = {
  0xB2, 0x01, 0x01, 0x01, 
};
static char cmd2_16[4] = {
  0xB3, 0x08, 0x08, 0x08, 
};
static char cmd2_17[4] = {
  0xB4, 0x09, 0x09, 0x09, 
};
static char cmd2_18[4] = {
  0xB6, 0x44, 0x44, 0x44, 
};
static char cmd2_19[4] = {
  0xB7, 0x34, 0x34, 0x34, 
};
static char cmd2_20[4] = {
  0xB8, 0x20, 0x20, 0x20, 
};
static char cmd2_21[4] = {
  0xB9, 0x27, 0x27, 0x27, 
};
static char cmd2_22[4] = {
  0xBA, 0x26, 0x26, 0x26, 
};
static char cmd2_23[4] = {
  0xBC, 0x00, 0xC8, 0x00, 
};
static char cmd2_24[4] = {
  0xBD, 0x00, 0xC8, 0x00, 
};
static char cmd2_25[2] = {
  0xBE, 0x63, 
};
static char cmd2_26[3] = {
  0xC0, 0x00, 0x08, 
};
static char cmd2_27[2] = {
  0xCA, 0x00, 
};
static char cmd2_28[5] = {
  0xD0, 0x0A, 0x10, 0x0D, 0x0F, 
};
static char cmd2_29[17] = {
  0xD1, 0x00, 0x70, 0x00, 0xCE, 0x00, 0xF7, 0x01, 
  0x10, 0x01, 0x21, 0x01, 0x44, 0x01, 0x62, 0x01, 
  0x8D, 
};
static char cmd2_30[17] = {
  0xD5, 0x00, 0x70, 0x00, 0xCE, 0x00, 0xF7, 0x01, 
  0x10, 0x01, 0x21, 0x01, 0x44, 0x01, 0x62, 0x01, 
  0x8D, 
};
static char cmd2_31[17] = {
  0xD9, 0x00, 0x70, 0x00, 0xCE, 0x00, 0xF7, 0x01, 
  0x10, 0x01, 0x21, 0x01, 0x44, 0x01, 0x62, 0x01, 
  0x8D, 
};
static char cmd2_32[17] = {
  0xE0, 0x00, 0x70, 0x00, 0xCE, 0x00, 0xF7, 0x01, 
  0x10, 0x01, 0x21, 0x01, 0x44, 0x01, 0x62, 0x01, 
  0x8D, 
};
static char cmd2_33[17] = {
  0xE4, 0x00, 0x70, 0x00, 0xCE, 0x00, 0xF7, 0x01, 
  0x10, 0x01, 0x21, 0x01, 0x44, 0x01, 0x62, 0x01, 
  0x8D, 
};
static char cmd2_34[17] = {
  0xE8, 0x00, 0x70, 0x00, 0xCE, 0x00, 0xF7, 0x01, 
  0x10, 0x01, 0x21, 0x01, 0x44, 0x01, 0x62, 0x01, 
  0x8D, 
};
static char cmd2_35[17] = {
  0xD2, 0x01, 0xAF, 0x01, 0xE4, 0x02, 0x0C, 0x02, 
  0x4D, 0x02, 0x82, 0x02, 0x84, 0x02, 0xB8, 0x02, 
  0xF0, 
};
static char cmd2_36[17] = {
  0xD6, 0x01, 0xAF, 0x01, 0xE4, 0x02, 0x0C, 0x02, 
  0x4D, 0x02, 0x82, 0x02, 0x84, 0x02, 0xB8, 0x02, 
  0xF0, 
};
static char cmd2_37[17] = {
  0xDD, 0x01, 0xAF, 0x01, 0xE4, 0x02, 0x0C, 0x02, 
  0x4D, 0x02, 0x82, 0x02, 0x84, 0x02, 0xB8, 0x02, 
  0xF0, 
};
static char cmd2_38[17] = {
  0xE1, 0x01, 0xAF, 0x01, 0xE4, 0x02, 0x0C, 0x02, 
  0x4D, 0x02, 0x82, 0x02, 0x84, 0x02, 0xB8, 0x02, 
  0xF0, 
};
static char cmd2_39[17] = {
  0xE5, 0x01, 0xAF, 0x01, 0xE4, 0x02, 0x0C, 0x02, 
  0x4D, 0x02, 0x82, 0x02, 0x84, 0x02, 0xB8, 0x02, 
  0xF0, 
};
static char cmd2_40[17] = {
  0xE9, 0x01, 0xAF, 0x01, 0xE4, 0x02, 0x0C, 0x02, 
  0x4D, 0x02, 0x82, 0x02, 0x84, 0x02, 0xB8, 0x02, 
  0xF0, 
};
static char cmd2_41[17] = {
  0xD3, 0x03, 0x14, 0x03, 0x40, 0x03, 0x58, 0x03, 
  0x70, 0x03, 0x80, 0x03, 0x90, 0x03, 0xA0, 0x03, 
  0xB0, 
};
static char cmd2_42[17] = {
  0xD7, 0x03, 0x14, 0x03, 0x40, 0x03, 0x58, 0x03, 
  0x70, 0x03, 0x80, 0x03, 0x90, 0x03, 0xA0, 0x03, 
  0xB0, 
};
static char cmd2_43[17] = {
  0xDE, 0x03, 0x14, 0x03, 0x40, 0x03, 0x58, 0x03, 
  0x70, 0x03, 0x80, 0x03, 0x90, 0x03, 0xA0, 0x03, 
  0xB0, 
};
static char cmd2_44[17] = {
  0xE2, 0x03, 0x14, 0x03, 0x40, 0x03, 0x58, 0x03, 
  0x70, 0x03, 0x80, 0x03, 0x90, 0x03, 0xA0, 0x03, 
  0xB0, 
};
static char cmd2_45[17] = {
  0xE6, 0x03, 0x14, 0x03, 0x40, 0x03, 0x58, 0x03, 
  0x70, 0x03, 0x80, 0x03, 0x90, 0x03, 0xA0, 0x03, 
  0xB0, 
};
static char cmd2_46[17] = {
  0xEA, 0x03, 0x14, 0x03, 0x40, 0x03, 0x58, 0x03, 
  0x70, 0x03, 0x80, 0x03, 0x90, 0x03, 0xA0, 0x03, 
  0xB0, 
};
static char cmd2_47[5] = {
  0xD4, 0x03, 0xFD, 0x03, 0xFF, 
};
static char cmd2_48[5] = {
  0xD8, 0x03, 0xFD, 0x03, 0xFF, 
};
static char cmd2_49[5] = {
  0xDF, 0x03, 0xFD, 0x03, 0xFF, 
};
static char cmd2_50[5] = {
  0xE3, 0x03, 0xFD, 0x03, 0xFF, 
};
static char cmd2_51[5] = {
  0xE7, 0x03, 0xFD, 0x03, 0xFF, 
};
static char cmd2_52[5] = {
  0xEB, 0x03, 0xFD, 0x03, 0xFF, 
};
static char cmd2_53[2] = {
  0x13, 0x00, 
};
static char cmd2_54[2] = {
  0x35, 0x00, 
};
static char cmd2_55[3] = {
  0x44, 0x01, 0x3F, 
};
static char cmd2_56[2] = {
  0x3A, 0x77, 
};
static char cmd2_57[2] = {
  0x36, 0xD0, 
};

static struct dsi_cmd_desc novatek_nt35516_qhd_v2_cmd_display_on_cmds[] = {
  {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(cmd2_0), cmd2_0},
  {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(cmd2_1), cmd2_1},
  {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(cmd2_2), cmd2_2},
  {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(cmd2_3), cmd2_3},
  {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(cmd2_4), cmd2_4},
  {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(cmd2_5), cmd2_5},
  {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(cmd2_6), cmd2_6},
  {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(cmd2_7), cmd2_7},
  {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(cmd2_8), cmd2_8},
  {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(cmd2_9), cmd2_9},
  {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(cmd2_10), cmd2_10},
  {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(cmd2_11), cmd2_11},
  {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(cmd2_12), cmd2_12},
  {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(cmd2_13), cmd2_13},
  {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(cmd2_14), cmd2_14},
  {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(cmd2_15), cmd2_15},
  {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(cmd2_16), cmd2_16},
  {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(cmd2_17), cmd2_17},
  {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(cmd2_18), cmd2_18},
  {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(cmd2_19), cmd2_19},
  {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(cmd2_20), cmd2_20},
  {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(cmd2_21), cmd2_21},
  {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(cmd2_22), cmd2_22},
  {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(cmd2_23), cmd2_23},
  {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(cmd2_24), cmd2_24},
  {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(cmd2_25), cmd2_25},
  {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(cmd2_26), cmd2_26},
  {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(cmd2_27), cmd2_27},
  {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(cmd2_28), cmd2_28},
  {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(cmd2_29), cmd2_29},
  {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(cmd2_30), cmd2_30},
  {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(cmd2_31), cmd2_31},
  {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(cmd2_32), cmd2_32},
  {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(cmd2_33), cmd2_33},
  {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(cmd2_34), cmd2_34},
  {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(cmd2_35), cmd2_35},
  {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(cmd2_36), cmd2_36},
  {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(cmd2_37), cmd2_37},
  {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(cmd2_38), cmd2_38},
  {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(cmd2_39), cmd2_39},
  {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(cmd2_40), cmd2_40},
  {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(cmd2_41), cmd2_41},
  {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(cmd2_42), cmd2_42},
  {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(cmd2_43), cmd2_43},
  {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(cmd2_44), cmd2_44},
  {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(cmd2_45), cmd2_45},
  {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(cmd2_46), cmd2_46},
  {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(cmd2_47), cmd2_47},
  {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(cmd2_48), cmd2_48},
  {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(cmd2_49), cmd2_49},
  {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(cmd2_50), cmd2_50},
  {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(cmd2_51), cmd2_51},
  {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(cmd2_52), cmd2_52},
  {DTYPE_DCS_WRITE, 1, 0, 0, 0, sizeof(cmd2_53), cmd2_53},
  {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(cmd2_54), cmd2_54},
  {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(cmd2_55), cmd2_55},
  {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(cmd2_56), cmd2_56},
  {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(cmd2_57), cmd2_57},
  {DTYPE_DCS_WRITE, 1, 0, 0, NT35516_SLEEP_OFF_DELAY, sizeof(nt_exit_sleep), nt_exit_sleep},
  {DTYPE_DCS_WRITE, 1, 0, 0, NT35516_V2_DISPLAY_ON_DELAY, sizeof(nt_display_on), nt_display_on},
};



static char video0[5] = {
  0xFF, 0xAA, 0x55, 0x25, 0x01, 
};
static char video1[36] = {
  0xF2, 0x00, 0x00, 0x4A, 0x0A, 0xA8, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x0B, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x01, 0x51, 
  0x00, 0x01, 0x00, 0x01, 
};
static char video2[8] = {
  0xF3, 0x02, 0x03, 0x07, 0x45, 0x88, 0xD4, 0x0D, 
};
static char video3[6] = {
  0xF0, 0x55, 0xAA, 0x52, 0x08, 0x00, 
};
static char video4[4] = {
  0xB1, 0x7C, 0x00, 0x00, 
};
static char video5[5] = {
  0xB8, 0x01, 0x02, 0x02, 0x02, 
};
static char video6[4] = {
  0xBB, 0x63, 0x03, 0x63, 
};
static char video7[4] = {
  0xBC, 0x00, 0x00, 0x00, 
};
static char video8[7] = {
  0xC9, 0x63, 0x06, 0x0D, 0x1A, 0x17, 0x00, 
};
static char video9[2] = {
  0xD0, 0x01, 
};
static char video10[6] = {
  0xF0, 0x55, 0xAA, 0x52, 0x08, 0x01, 
};
static char video11[4] = {
  0xB0, 0x05, 0x05, 0x05, 
};
static char video12[4] = {
  0xB1, 0x05, 0x05, 0x05, 
};
static char video13[4] = {
  0xB2, 0x01, 0x01, 0x01, 
};
static char video14[4] = {
  0xB3, 0x08, 0x08, 0x08, 
};
static char video15[4] = {
  0xB4, 0x09, 0x09, 0x09, 
};
static char video16[4] = {
  0xB6, 0x44, 0x44, 0x44, 
};
static char video17[4] = {
  0xB7, 0x34, 0x34, 0x34, 
};
static char video18[4] = {
  0xB8, 0x20, 0x20, 0x20, 
};
static char video19[4] = {
  0xB9, 0x27, 0x27, 0x27, 
};
static char video20[4] = {
  0xBA, 0x26, 0x26, 0x26, 
};
static char video21[4] = {
  0xBC, 0x00, 0xC8, 0x00, 
};
static char video22[4] = {
  0xBD, 0x00, 0xC8, 0x00, 
};
static char video23[2] = {
  0xBE, 0x63, 
};
static char video24[3] = {
  0xC0, 0x00, 0x08, 
};
static char video25[2] = {
  0xCA, 0x00, 
};
static char video26[5] = {
  0xD0, 0x0A, 0x10, 0x0D, 0x0F, 
};
static char video27[17] = {
  0xD1, 0x00, 0x70, 0x00, 0xCE, 0x00, 0xF7, 0x01, 
  0x10, 0x01, 0x21, 0x01, 0x44, 0x01, 0x62, 0x01, 
  0x8D, 
};
static char video28[17] = {
  0xD5, 0x00, 0x70, 0x00, 0xCE, 0x00, 0xF7, 0x01, 
  0x10, 0x01, 0x21, 0x01, 0x44, 0x01, 0x62, 0x01, 
  0x8D, 
};
static char video29[17] = {
  0xD9, 0x00, 0x70, 0x00, 0xCE, 0x00, 0xF7, 0x01, 
  0x10, 0x01, 0x21, 0x01, 0x44, 0x01, 0x62, 0x01, 
  0x8D, 
};
static char video30[17] = {
  0xE0, 0x00, 0x70, 0x00, 0xCE, 0x00, 0xF7, 0x01, 
  0x10, 0x01, 0x21, 0x01, 0x44, 0x01, 0x62, 0x01, 
  0x8D, 
};
static char video31[17] = {
  0xE4, 0x00, 0x70, 0x00, 0xCE, 0x00, 0xF7, 0x01, 
  0x10, 0x01, 0x21, 0x01, 0x44, 0x01, 0x62, 0x01, 
  0x8D, 
};
static char video32[17] = {
  0xE8, 0x00, 0x70, 0x00, 0xCE, 0x00, 0xF7, 0x01, 
  0x10, 0x01, 0x21, 0x01, 0x44, 0x01, 0x62, 0x01, 
  0x8D, 
};
static char video33[17] = {
  0xD2, 0x01, 0xAF, 0x01, 0xE4, 0x02, 0x0C, 0x02, 
  0x4D, 0x02, 0x82, 0x02, 0x84, 0x02, 0xB8, 0x02, 
  0xF0, 
};
static char video34[17] = {
  0xD6, 0x01, 0xAF, 0x01, 0xE4, 0x02, 0x0C, 0x02, 
  0x4D, 0x02, 0x82, 0x02, 0x84, 0x02, 0xB8, 0x02, 
  0xF0, 
};
static char video35[17] = {
  0xDD, 0x01, 0xAF, 0x01, 0xE4, 0x02, 0x0C, 0x02, 
  0x4D, 0x02, 0x82, 0x02, 0x84, 0x02, 0xB8, 0x02, 
  0xF0, 
};
static char video36[17] = {
  0xE1, 0x01, 0xAF, 0x01, 0xE4, 0x02, 0x0C, 0x02, 
  0x4D, 0x02, 0x82, 0x02, 0x84, 0x02, 0xB8, 0x02, 
  0xF0, 
};
static char video37[17] = {
  0xE5, 0x01, 0xAF, 0x01, 0xE4, 0x02, 0x0C, 0x02, 
  0x4D, 0x02, 0x82, 0x02, 0x84, 0x02, 0xB8, 0x02, 
  0xF0, 
};
static char video38[17] = {
  0xE9, 0x01, 0xAF, 0x01, 0xE4, 0x02, 0x0C, 0x02, 
  0x4D, 0x02, 0x82, 0x02, 0x84, 0x02, 0xB8, 0x02, 
  0xF0, 
};
static char video39[17] = {
  0xD3, 0x03, 0x14, 0x03, 0x40, 0x03, 0x58, 0x03, 
  0x70, 0x03, 0x80, 0x03, 0x90, 0x03, 0xA0, 0x03, 
  0xB0, 
};
static char video40[17] = {
  0xD7, 0x03, 0x14, 0x03, 0x40, 0x03, 0x58, 0x03, 
  0x70, 0x03, 0x80, 0x03, 0x90, 0x03, 0xA0, 0x03, 
  0xB0, 
};
static char video41[17] = {
  0xDE, 0x03, 0x14, 0x03, 0x40, 0x03, 0x58, 0x03, 
  0x70, 0x03, 0x80, 0x03, 0x90, 0x03, 0xA0, 0x03, 
  0xB0, 
};
static char video42[17] = {
  0xE2, 0x03, 0x14, 0x03, 0x40, 0x03, 0x58, 0x03, 
  0x70, 0x03, 0x80, 0x03, 0x90, 0x03, 0xA0, 0x03, 
  0xB0, 
};
static char video43[17] = {
  0xE6, 0x03, 0x14, 0x03, 0x40, 0x03, 0x58, 0x03, 
  0x70, 0x03, 0x80, 0x03, 0x90, 0x03, 0xA0, 0x03, 
  0xB0, 
};
static char video44[17] = {
  0xEA, 0x03, 0x14, 0x03, 0x40, 0x03, 0x58, 0x03, 
  0x70, 0x03, 0x80, 0x03, 0x90, 0x03, 0xA0, 0x03, 
  0xB0, 
};
static char video45[5] = {
  0xD4, 0x03, 0xFD, 0x03, 0xFF, 
};
static char video46[5] = {
  0xD8, 0x03, 0xFD, 0x03, 0xFF, 
};
static char video47[5] = {
  0xDF, 0x03, 0xFD, 0x03, 0xFF, 
};
static char video48[5] = {
  0xE3, 0x03, 0xFD, 0x03, 0xFF, 
};
static char video49[5] = {
  0xE7, 0x03, 0xFD, 0x03, 0xFF, 
};
static char video50[5] = {
  0xEB, 0x03, 0xFD, 0x03, 0xFF, 
};
static char video52[2] = {
  0x36, 0xD0, 
};
static char video53[2] = {
  0x3A, 0x77, 
};
static char video54[2] = {
  0x53, 0x24, 
};
static char video55[2] = {
  0x55, 0x00, 
};

static struct dsi_cmd_desc novatek_nt35516_qhd_video_display_on_cmds[] = {
  {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(video0), video0},
  {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(video1), video1},
  {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(video2), video2},
  {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(video3), video3},
  {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(video4), video4},
  {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(video5), video5},
  {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(video6), video6},
  {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(video7), video7},
  {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(video8), video8},
  {DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(video9), video9},
  {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(video10), video10},
  {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(video11), video11},
  {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(video12), video12},
  {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(video13), video13},
  {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(video14), video14},
  {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(video15), video15},
  {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(video16), video16},
  {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(video17), video17},
  {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(video18), video18},
  {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(video19), video19},
  {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(video20), video20},
  {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(video21), video21},
  {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(video22), video22},
  {DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(video23), video23},
  {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(video24), video24},
  {DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(video25), video25},
  {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(video26), video26},
  {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(video27), video27},
  {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(video28), video28},
  {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(video29), video29},
  {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(video30), video30},
  {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(video31), video31},
  {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(video32), video32},
  {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(video33), video33},
  {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(video34), video34},
  {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(video35), video35},
  {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(video36), video36},
  {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(video37), video37},
  {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(video38), video38},
  {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(video39), video39},
  {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(video40), video40},
  {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(video41), video41},
  {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(video42), video42},
  {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(video43), video43},
  {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(video44), video44},
  {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(video45), video45},
  {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(video46), video46},
  {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(video47), video47},
  {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(video48), video48},
  {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(video49), video49},
  {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(video50), video50},
  {DTYPE_DCS_WRITE, 1, 0, 0, NT35516_SLEEP_OFF_DELAY, sizeof(nt_exit_sleep), nt_exit_sleep},
  {DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(video52), video52},
  {DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(video53), video53},
  {DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(video54), video54},
  {DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(video55), video55},
  {DTYPE_DCS_WRITE, 1, 0, 0, NT35516_DISPLAY_ON_DELAY, sizeof(nt_display_on), nt_display_on},
};


static int mipi_novatek_nt35516_qhd_lcd_on(struct platform_device *pdev)
{
	struct msm_fb_data_type *mfd;
	struct mipi_panel_info *mipi;
	struct dsi_buf *dp;
	struct dsi_cmd_desc *cmds;
	int cnt;
	uint32_t spv;
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

	spv = socinfo_get_platform_version();
	if (mipi->mode == DSI_VIDEO_MODE) {
		pr_err("%s: display on video mode \n", __func__);
		dp = &novatek_nt35516_qhd_tx_buf;
		cmds = novatek_nt35516_qhd_video_display_on_cmds;
		cnt = ARRAY_SIZE(novatek_nt35516_qhd_video_display_on_cmds);  // 57
		mipi_dsi_cmds_tx(dp, cmds, cnt);
	} else {
		pr_err("%s: display on cmd mode \n", __func__);
		if (spv == 0x10000) {
			dp = &novatek_nt35516_qhd_tx_buf;
			cmds = novatek_nt35516_qhd_cmd_display_on_cmds;
			cnt = ARRAY_SIZE(novatek_nt35516_qhd_cmd_display_on_cmds);  // 61
			mipi_dsi_cmds_tx(dp, cmds, cnt);
		} else
		if (spv == 0x20000) {
			dp = &novatek_nt35516_qhd_tx_buf;			
			cmds = novatek_nt35516_qhd_v2_cmd_display_on_cmds;
			cnt = ARRAY_SIZE(novatek_nt35516_qhd_v2_cmd_display_on_cmds);  // 60
			mipi_dsi_cmds_tx(dp, cmds, cnt);
		} else {
			pr_err("%s: socinfo.platform.version UNKNOWN (%d) \n", __func__, spv);
			ret = -EINVAL;
		}
	}

	if (ret == 0)
		mipi_dsi_cmd_bta_sw_trigger();

	pr_debug("%s: X\n", __func__);
	return ret;
}

static int mipi_novatek_nt35516_qhd_lcd_off(struct platform_device *pdev)
{
	struct msm_fb_data_type *mfd;

	pr_debug("%s: E\n", __func__);

	mfd = platform_get_drvdata(pdev);

	if (!mfd)
		return -ENODEV;
	if (mfd->key != MFD_KEY)
		return -EINVAL;

	mipi_dsi_cmds_tx(&novatek_nt35516_qhd_tx_buf, nt35516_display_off_cmds,
			ARRAY_SIZE(nt35516_display_off_cmds));

	pr_debug("%s: X\n", __func__);
	return 0;
}
/*
static ssize_t mipi_nt35516_wta_bl_ctrl(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	ssize_t ret = strnlen(buf, PAGE_SIZE);
	int err;

	err =  kstrtoint(buf, 0, &mipi_nt35516_bl_ctrl);
	if (err)
		return ret;

	pr_info("%s: bl ctrl set to %d\n", __func__, mipi_nt35516_bl_ctrl);

	return ret;
}

static DEVICE_ATTR(bl_ctrl, S_IWUSR, NULL, mipi_nt35516_wta_bl_ctrl);

static struct attribute *mipi_nt35516_fs_attrs[] = {
	&dev_attr_bl_ctrl.attr,
	NULL,
};

static struct attribute_group mipi_nt35516_fs_attr_group = {
	.attrs = mipi_nt35516_fs_attrs,
};

static int mipi_nt35516_create_sysfs(struct platform_device *pdev)
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
		&mipi_nt35516_fs_attr_group);
	if (rc) {
		pr_err("%s: sysfs group creation failed, rc=%d\n",
			__func__, rc);
		return rc;
	}

	return 0;
}
*/
static int __devinit mipi_novatek_nt35516_qhd_lcd_probe(struct platform_device *pdev)
{
	struct platform_device *pthisdev = NULL;
	pr_debug("%s\n", __func__);

	if (pdev->id == 0) {
		mipi_novatek_nt35516_pdata = pdev->dev.platform_data;
		if (mipi_novatek_nt35516_pdata)
			gpio_backlight_en = mipi_novatek_nt35516_pdata->gpio;
		return 0;
	}

	pthisdev = msm_fb_add_device(pdev);
	//mipi_nt35516_create_sysfs(pthisdev);

	return 0;
}

static struct platform_driver this_driver = {
	.probe  = mipi_novatek_nt35516_qhd_lcd_probe,
	.driver = {
		.name = "mipi_novatek_nt35516_qhd",
	},
};

static char pwm0[2] = {
	0x51, 0x8C,
};
static struct dsi_cmd_desc novatek_nt35516_qhd_pwm_cmds[] = {
	{DTYPE_DCS_LWRITE, 1, 0, 0, 1, sizeof(pwm0), pwm0},
};

static int old_bl_level;

static void mipi_nt35516_set_backlight(struct msm_fb_data_type *mfd)
{
	int bl_level = mfd->bl_level;
	uint32_t spv = socinfo_get_platform_version();
	
	if (spv == 0x10000) {
		gpio_set_value_cansleep(gpio_backlight_en, bl_level != 0);
		mipi_dsi_mdp_busy_wait(mfd);
		old_bl_level = bl_level;
		mipi_dsi_cmds_tx(&novatek_nt35516_qhd_tx_buf, novatek_nt35516_qhd_pwm_cmds, 1);
	} else
	if (spv == 0x20000) {
		if (mipi_novatek_nt35516_pdata->pmic_backlight)   // TODO
			mipi_novatek_nt35516_pdata->pmic_backlight(255 - bl_level);
	}
}

static struct msm_fb_panel_data novatek_nt35516_qhd_panel_data = {
	.on	= mipi_novatek_nt35516_qhd_lcd_on,
	.off = mipi_novatek_nt35516_qhd_lcd_off,
	.set_backlight = mipi_nt35516_set_backlight,
};

static int ch_used[3];

static int mipi_novatek_nt35516_qhd_lcd_init(void)
{
	mipi_dsi_buf_alloc(&novatek_nt35516_qhd_tx_buf, DSI_BUF_SIZE);
	mipi_dsi_buf_alloc(&novatek_nt35516_qhd_rx_buf, DSI_BUF_SIZE);

	return platform_driver_register(&this_driver);
}

int mipi_novatek_nt35516_qhd_device_register(struct msm_panel_info *pinfo,
					u32 channel, u32 panel)
{
	struct platform_device *pdev = NULL;
	int ret;

	pr_err("%s\n", __func__);
	if ((channel >= 3) || ch_used[channel])
		return -ENODEV;

	ch_used[channel] = TRUE;

	ret = mipi_novatek_nt35516_qhd_lcd_init();
	if (ret) {
		pr_err("%s: failed with ret %u\n", __func__, ret);
		return ret;
	}

	pdev = platform_device_alloc("mipi_novatek_nt35516_qhd", (panel << 8)|channel);
	if (!pdev)
		return -ENOMEM;

	novatek_nt35516_qhd_panel_data.panel_info = *pinfo;
	ret = platform_device_add_data(pdev, &novatek_nt35516_qhd_panel_data,
				sizeof(novatek_nt35516_qhd_panel_data));
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
