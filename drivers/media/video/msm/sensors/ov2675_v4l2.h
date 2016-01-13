
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
 */
#ifndef CAMSENSOR_OV2675_V4L2
#define CAMSENSOR_OV2675_V4L2

//Auto Focus Command
#define OV2675_CMD_MAIN 0x3022
#define OV2675_CMD_ACK 0x3023
#define OV2675_CMD_PARA0 0x3024
#define OV2675_CMD_PARA1 0x3025
#define OV2675_CMD_PARA2 0x3026
#define OV2675_CMD_PARA3 0x3027
#define OV2675_CMD_PARA4 0x3028
#define OV2675_CMD_FW_STATUS 0x3029

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//
//Auto Focus Firmware-use 2011-08-24 firmware settings
unsigned char ov2675_afinit_tbl[] =
{

};
#endif /* CAMSENSOR_OV2675 */
