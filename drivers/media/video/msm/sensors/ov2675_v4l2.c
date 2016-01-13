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

#include "msm_sensor.h"
#include "msm.h"
#include "ov2675_v4l2.h"

#define SENSOR_NAME "ov2675"
#define PLATFORM_DRIVER_NAME "msm_camera_ov2675"
#define ov2675_obj ov2675_##obj

#ifdef CDBG
#undef CDBG
#endif
#ifdef CDBG_HIGH
#undef CDBG_HIGH
#endif

//#define OV2675_VERBOSE_DGB

#ifdef OV2675_VERBOSE_DGB
#define CDBG(fmt, args...) printk(fmt, ##args)
#define CDBG_HIGH(fmt, args...) printk(fmt, ##args)
#else
#define CDBG(fmt, args...) do { } while (0)
#define CDBG_HIGH(fmt, args...) printk(fmt, ##args)
#endif
extern int lcd_camera_power_onoff(int on);
static struct msm_sensor_ctrl_t * ov2675_v4l2_ctrl; //for OV2675 i2c read and write
static struct msm_sensor_ctrl_t ov2675_s_ctrl;

static int effect_value = CAMERA_EFFECT_OFF;
static unsigned int SAT_U = 0x80; /* DEFAULT SATURATION VALUES*/
static unsigned int SAT_V = 0x80; /* DEFAULT SATURATION VALUES*/

DEFINE_MUTEX(ov2675_mut);

static struct msm_camera_i2c_reg_conf ov2675_start_settings[] = {
//	{0x30ad,0x00},
	{0x3086, 0x00},  /* streaming on */
};

static struct msm_camera_i2c_reg_conf ov2675_stop_settings[] = {
//	{0x30ad,0x0a},	
	{0x3086, 0x0f},  /* streaming off*/
};

#if 0
struct msm_camera_i2c_reg_conf ov2675_prev_30fps_settings[] = {
//{0x3013,0xf7},
//{0x3010,0x82},
{0x3012,0x10},
{0x302a,0x02},
{0x302b,0xE6},
{0x306f,0x14},
{0x3362,0x90},
{0x3070,0x5d},
{0x3072,0x5d},
{0x301c,0x07},
{0x301d,0x07},
{0x3020,0x01},
{0x3021,0x18},
{0x3022,0x00},
{0x3023,0x06},
{0x3024,0x06},
{0x3025,0x58},
{0x3026,0x02},
{0x3027,0x5E},
{0x3088,0x03},
{0x3089,0x20},
{0x308A,0x02},
{0x308B,0x58},
{0x3316,0x64},
{0x3317,0x25},
{0x3318,0x80},
{0x3319,0x08},
{0x331A,0x64},
{0x331B,0x4B},
{0x331C,0x00},
{0x331D,0x38},
{0x3302,0x11},
};
struct msm_camera_i2c_reg_conf ov2675_snap_settings[] = {
//{0x3013,0xf2},
//{0x3010,0x81},
{0x3012,0x00},
{0x302a,0x05},
{0x302b,0xCB},
{0x306f,0x54},
{0x3362,0x80},
{0x3070,0x5d},
{0x3072,0x5d},
{0x301c,0x0f},
{0x301d,0x0f},
{0x3020,0x01},
{0x3021,0x18},
{0x3022,0x00},
{0x3023,0x0A},
{0x3024,0x06},
{0x3025,0x58},
{0x3026,0x04},
{0x3027,0xbc},
{0x3088,0x06},
{0x3089,0x40},
{0x308A,0x04},
{0x308B,0xB0},
{0x3316,0x64},
{0x3317,0x4B},
{0x3318,0x00},
{0x3319,0x2C},
{0x331A,0x64},
{0x331B,0x4B},
{0x331C,0x00},
{0x331D,0x4C},
{0x3302,0x01},
};
//set sensor init setting
struct msm_camera_i2c_reg_conf ov2675_init_settings[] = {
//@@ 1600x1200 YUV 15 FPS MIPI
//2650 Rev1D reference setting 04092008
//24MHz 		//5FPS
//1600x1200	//YUV	output 
//HREF		positive
//Vsync		positive
//AEC//		Auto//	0x3013[0]=1
//AGC// 		Auto//	0x3013[2]=1	16x ceiling// 	0x3015[2:0]=011
//Banding Filter//Auto//	0x3013[5]=1	
//50/60 Auto detection	ON	0x3014[6]=1
//Night mode 	off//	0x3014[3]=0
//AWB// 		ON//	0x3300[5:4]=11
//LC 		ON//	0x3300[3:2]=11	
//UVadjust 	ON//	0x3301[6]=1
//WBC 		ON//	0x3301[1:0]=11
//UV average	ON//	0x3302[1:0]=01
//Scaling	Off//	0x3302[4]=0
//DNS 		Auto//	0x3306[2]=0  
//Sharpness 	Auto// 	0x3306[3]=0
//IO & Clock & Analog Setup
{0x308c,0x80},
{0x308d,0x0e},
{0x360b,0x00},
{0x30b0,0xff},
{0x30b1,0xff},
{0x30b2,0x24},
//{0x3085,0x20},
{0x300e,0x3a},
{0x300f,0xa6},
{0x3010,0x81},
{0x3082,0x01},
{0x30f4,0x01},
{0x3091,0xc0},
{0x30ac,0x42},
{0x30d1,0x08},
{0x30a8,0x54},
{0x3015,0x02},
{0x3093,0x00},
{0x307e,0xe5},
{0x3079,0x00},
{0x30aa,0x82},
{0x3017,0x40},
{0x30f3,0x83},
{0x306a,0x0c},
{0x306d,0x00},
{0x336a,0x3c},
{0x3076,0x6a},
{0x30d9,0x95},
{0x3016,0x52},
{0x3601,0x30},
{0x304e,0x88},
{0x30f1,0x82},
{0x306f,0x14},
{0x3012,0x10},
{0x3011,0x01},
{0x302a,0x02},
{0x302b,0xe6},
{0x3028,0x07},
{0x3029,0x93},
{0x3391,0x06},
{0x3394,0x40},
{0x3395,0x40},
{0x3015,0x02},
{0x302d,0x00},
{0x302e,0x00},
//AEC/AGC
{0x3013,0xf7},
{0x3018,0x80},
{0x3019,0x70},
{0x301a,0xc4},
//D5060
{0x30af,0x00},
{0x3048,0x1f},
{0x3049,0x4e},
{0x304a,0x40},
{0x304f,0x40},
{0x304b,0x02},
{0x304c,0x00},
{0x304d,0x42},
{0x304f,0x40},
{0x30a3,0x91},
{0x3013,0xf7},
{0x3014,0x84},
{0x3071,0x00},
{0x3070,0x5d},
{0x3073,0x00},
{0x3072,0x5d},
{0x301c,0x07},
{0x301d,0x07},
{0x304d,0x42},
{0x304a,0x40},
{0x304f,0x40},
{0x3095,0x07},
{0x3096,0x16},
{0x3097,0x1d},
//Window Setup
{0x3020,0x01},
{0x3021,0x1a},
{0x3022,0x00},
{0x3023,0x06},
{0x3024,0x06},
{0x3025,0x58},
{0x3026,0x02},
{0x3027,0x5e},
{0x3088,0x03},
{0x3089,0x20},
{0x308a,0x02},
{0x308b,0x58},
{0x3316,0x64},
{0x3317,0x25},
{0x3318,0x80},
{0x3319,0x08},
{0x331a,0x64},
{0x331b,0x4b},
{0x331c,0x00},
{0x331d,0x38},
{0x3100,0x00},
//AWB
{0x3320,0xfa},
{0x3321,0x11},
{0x3322,0x92},
{0x3323,0x01},
{0x3324,0x97},
{0x3325,0x02},
{0x3326,0xff},
{0x3327,0x10},
{0x3328,0x10},
{0x3329,0x1f},
{0x332a,0x56},
{0x332b,0x54},
{0x332c,0xbe},
{0x332d,0xce},
{0x332e,0x2e},
{0x332f,0x30},
{0x3330,0x4d},
{0x3331,0x44},
{0x3332,0xf0},
{0x3333,0x0a},
{0x3334,0xf0},
{0x3335,0xf0},
{0x3336,0xf0},
{0x3337,0x40},
{0x3338,0x40},
{0x3339,0x40},
{0x333a,0x00},
{0x333b,0x00},
//Color Matrix
{0x3380,0x28},
{0x3381,0x48},
{0x3382,0x14},
{0x3383,0x12},
{0x3384,0xb6},
{0x3385,0xc8},
{0x3386,0xc8},
{0x3387,0xc7},
{0x3388,0x01},
{0x3389,0x98},
{0x338a,0x01},
 //Gamma 3
 {0x3340,0x06},
 {0x3341,0x0c},
 {0x3342,0x19},
 {0x3343,0x34},
 {0x3344,0x4a},
 {0x3345,0x5a},
 {0x3346,0x67},
 {0x3347,0x71},
 {0x3348,0x7c},
 {0x3349,0x8c},
 {0x334a,0x9b},
 {0x334b,0xa9},
 {0x334c,0xc0},
 {0x334d,0xd5},
 {0x334e,0xe8},
 {0x334f,0x20},
//Lens correction
{0x3090,0x3b},
{0x307c,0x13},
{0x3350,0x35},
{0x3351,0x28},
{0x3352,0x00},
{0x3353,0x2b},
{0x3354,0x00},
{0x3355,0x85},
{0x3356,0x34},
{0x3357,0x28},
{0x3358,0x00},
{0x3359,0x28},
{0x335a,0x00},
{0x335b,0x85},
{0x335c,0x36},
{0x335d,0x28},
{0x335e,0x00},
{0x335f,0x25},
{0x3360,0x00},
{0x3361,0x85},
{0x3363,0x70},
{0x3364,0x7f},
{0x3365,0x00},
{0x3366,0x00},
{0x3362,0x90},
//UVadjust
{0x3301,0xff},
{0x338B,0x0c},//control color in low level 0c-0x11
{0x338c,0x0c},
{0x338d,0x40},
//Sharpness/De-noise
{0x3370,0xd0},
{0x3371,0x00},
{0x3372,0x00},
{0x3373,0x90},//50
{0x3374,0x10},
{0x3375,0x10},
{0x3376,0x08},//06
{0x3377,0x00},
{0x3378,0x04},
{0x3379,0x80},
//BLC
{0x3069,0x86},
{0x3087,0x02},
//Other functions
{0x3300,0xfc},
{0x3302,0x11},
{0x3400,0x00},
{0x3606,0x20},
{0x3601,0x30},
{0x30f3,0x83},
{0x304e,0x88},
{0x30a8,0x54},
{0x30aa,0x82},
{0x30a3,0x91},
{0x30a1,0x41},
//MIPI
{0x363b,0x01},
{0x309e,0x08},
{0x3606,0x00},
{0x3630,0x35},
{0x3086,0x0f},
{0x3086,0x00},
{0x3011,0x00},//12.5fps
{0x300e,0x34},
{0x3010,0x80},
{0x304e,0x04},
{0x363b,0x01},
{0x309e,0x08},
{0x3606,0x00},
{0x3084,0x01},
{0x3634,0x26},
{0x3086,0x0f},
{0x3086,0x00},
};
#endif


struct msm_camera_i2c_reg_conf ov2675_prev_30fps_settings[] = {
{0x3013,0xf7},
{0x3014,0x8c},

{0x3012,0x00},
{0x302a,0x05},
{0x302b,0xCB},
{0x306f,0x54},
{0x3362,0x80},

{0x3070,0xb9},
{0x3072,0xb9},
{0x301c,0x03},
{0x301d,0x03},

{0x3020,0x01},
{0x3021,0x18},
{0x3022,0x00},
{0x3023,0x0A},
{0x3024,0x06},
{0x3025,0x58},
{0x3026,0x04},
{0x3027,0xbc},
{0x3088,0x06},
{0x3089,0x40},
{0x308A,0x04},
{0x308B,0xB0},
{0x3316,0x64},
{0x3317,0x4B},
{0x3318,0x00},
{0x3319,0x2C},
{0x331A,0x64},
{0x331B,0x4B},
{0x331C,0x00},
{0x331D,0x4C},
{0x3302,0x01},


};



struct msm_camera_i2c_reg_conf ov2675_snap_settings[] = {
//{0x3013,0xf2},
//{0x3014,0x84},

{0x3012,0x00},
{0x302a,0x05},
{0x302b,0xCB},
{0x306f,0x54},
{0x3362,0x80},

{0x3070,0xb9},
{0x3072,0xb9},
{0x301c,0x07},
{0x301d,0x07},

{0x3020,0x01},
{0x3021,0x18},
{0x3022,0x00},
{0x3023,0x0A},
{0x3024,0x06},
{0x3025,0x58},
{0x3026,0x04},
{0x3027,0xbc},
{0x3088,0x06},
{0x3089,0x40},
{0x308A,0x04},
{0x308B,0xB0},
{0x3316,0x64},
{0x3317,0x4B},
{0x3318,0x00},
{0x3319,0x2C},
{0x331A,0x64},
{0x331B,0x4B},
{0x331C,0x00},
{0x331D,0x4C},
{0x3302,0x01},
};



//set sensor init setting
struct msm_camera_i2c_reg_conf ov2675_init_settings[] = {
//@@ 1600x1200 YUV 15 FPS MIPI
//2650 Rev1D reference setting 04092008
//24MHz 		//5FPS
//1600x1200	//YUV	output 
//HREF		positive
//Vsync		positive
//AEC//		Auto//	0x3013[0]=1
//AGC// 		Auto//	0x3013[2]=1	16x ceiling// 	0x3015[2:0]=011
//Banding Filter//Auto//	0x3013[5]=1	
//50/60 Auto detection	ON	0x3014[6]=1
//Night mode 	off//	0x3014[3]=0
//AWB// 		ON//	0x3300[5:4]=11
//LC 		ON//	0x3300[3:2]=11	
//UVadjust 	ON//	0x3301[6]=1
//WBC 		ON//	0x3301[1:0]=11
//UV average	ON//	0x3302[1:0]=01
//Scaling	Off//	0x3302[4]=0
//DNS 		Auto//	0x3306[2]=0  
//Sharpness 	Auto// 	0x3306[3]=0

//IO & Clock & Analog Setup
{0x308c,0x80},
{0x308d,0x0e},
{0x360b,0x00},
{0x30b0,0xff},
{0x30b1,0xff},
{0x30b2,0x24},


{0x300e,0x34},
{0x300f,0xa6},
{0x3010,0x81},
{0x3082,0x01},
{0x30f4,0x01},
{0x3091,0xc0},
{0x30ac,0x42},

{0x30d1,0x08},
{0x30a8,0x54},
{0x3015,0x02},
{0x3093,0x00},
{0x307e,0xe5},
{0x3079,0x00},
{0x30aa,0x82},
{0x3017,0x40},
{0x30f3,0x83},
{0x306a,0x0c},
{0x306d,0x00},
{0x336a,0x3c},
{0x3076,0x6a},
{0x30d9,0x95},
{0x3016,0x52},
{0x3601,0x30},
{0x304e,0x88},
{0x30f1,0x82},
{0x306f,0x14},

{0x3012,0x10},
{0x3011,0x00},
{0x302a,0x02},
{0x302b,0xe6},
{0x3028,0x07},
{0x3029,0x93},

{0x3391,0x06},
{0x3394,0x40},
{0x3395,0x40},

{0x3015,0x22},
{0x302d,0x00},
{0x302e,0x00},

//AEC/AGC
{0x3013,0xf7},
{0x3018,0x80},
{0x3019,0x70},
{0x301a,0xc4},

//D5060
{0x30af,0x00},
{0x3048,0x1f},
{0x3049,0x4e},
{0x304a,0x40},
{0x304f,0x40},
{0x304b,0x02},
{0x304c,0x00},
{0x304d,0x42},
{0x304f,0x40},
{0x30a3,0x91},
{0x3013,0xf7},
{0x3014,0x8c},
{0x3071,0x00},
{0x3070,0xb9},
{0x3073,0x00},
{0x3072,0xb9},
{0x301c,0x03},
{0x301d,0x03},
{0x304d,0x42},
{0x304a,0x40},
{0x304f,0x40},
{0x3095,0x07},
{0x3096,0x16},
{0x3097,0x1d},

//Window Setup
{0x3020,0x01},
{0x3021,0x1a},
{0x3022,0x00},
{0x3023,0x06},
{0x3024,0x06},
{0x3025,0x58},
{0x3026,0x02},
{0x3027,0x5e},
{0x3088,0x03},
{0x3089,0x20},
{0x308a,0x02},
{0x308b,0x58},
{0x3316,0x64},
{0x3317,0x25},
{0x3318,0x80},
{0x3319,0x08},
{0x331a,0x64},
{0x331b,0x4b},
{0x331c,0x00},
{0x331d,0x38},
{0x3100,0x00},

//AWB
{0x3320,0xfa},
{0x3321,0x11},
{0x3322,0x92},
{0x3323,0x01},
{0x3324,0x97},
{0x3325,0x02},
{0x3326,0xff},
{0x3327,0x10},
{0x3328,0x10},
{0x3329,0x1f},
{0x332a,0x56},
{0x332b,0x54},
{0x332c,0xbe},
{0x332d,0xce},
{0x332e,0x2e},
{0x332f,0x30},
{0x3330,0x4d},
{0x3331,0x44},
{0x3332,0xf0},
{0x3333,0x0a},
{0x3334,0xf0},
{0x3335,0xf0},
{0x3336,0xf0},
{0x3337,0x40},
{0x3338,0x40},
{0x3339,0x40},
{0x333a,0x00},
{0x333b,0x00},

//Color Matrix
{0x3380,0x28},
{0x3381,0x48},
{0x3382,0x14},
{0x3383,0x12},
{0x3384,0xb6},
{0x3385,0xc8},
{0x3386,0xc8},
{0x3387,0xc7},
{0x3388,0x01},
{0x3389,0x98},
{0x338a,0x01},

 //Gamma 3
 {0x3340,0x06},
 {0x3341,0x0c},
 {0x3342,0x19},
 {0x3343,0x34},
 {0x3344,0x4a},
 {0x3345,0x5a},
 {0x3346,0x67},
 {0x3347,0x71},
 {0x3348,0x7c},
 {0x3349,0x8c},
 {0x334a,0x9b},
 {0x334b,0xa9},
 {0x334c,0xc0},
 {0x334d,0xd5},
 {0x334e,0xe8},
 {0x334f,0x20},

//Lens correction
{0x3090,0x3b},
{0x307c,0x13},

{0x3350,0x35},
{0x3351,0x28},
{0x3352,0x00},
{0x3353,0x2b},
{0x3354,0x00},
{0x3355,0x85},

{0x3356,0x34},
{0x3357,0x28},
{0x3358,0x00},
{0x3359,0x28},
{0x335a,0x00},
{0x335b,0x85},

{0x335c,0x36},
{0x335d,0x28},
{0x335e,0x00},
{0x335f,0x25},
{0x3360,0x00},
{0x3361,0x85},

{0x3363,0x70},
{0x3364,0x7f},
{0x3365,0x00},
{0x3366,0x00},

{0x3362,0x90},

//UVadjust
{0x3301,0xff},
{0x338B,0x0c},//control color in low level 0c-0x11
{0x338c,0x0c},
{0x338d,0x40},

//Sharpness/De-noise
{0x3370,0xd0},
{0x3371,0x00},
{0x3372,0x00},
{0x3373,0x90},//50
{0x3374,0x10},
{0x3375,0x10},
{0x3376,0x08},//06
{0x3377,0x00},
{0x3378,0x04},
{0x3379,0x80},

//BLC
{0x3069,0x86},
{0x3087,0x02},

//Other functions
{0x3300,0xfc},
{0x3302,0x11},
{0x3400,0x00},
{0x3606,0x20},
{0x3601,0x30},
{0x30f3,0x83},
{0x304e,0x88},

{0x30a8,0x54},
{0x30aa,0x82},
{0x30a3,0x91},
{0x30a1,0x41},

//MIPI
{0x363b,0x01},
{0x309e,0x08},
{0x3606,0x00},
{0x3630,0x35},
{0x3086,0x0f},
{0x3086,0x00},

{0x3011,0x00},//25fps
{0x300e,0x34},
{0x3010,0x80},

{0x304e,0x04},
{0x363b,0x01},
{0x309e,0x08},
{0x3606,0x00},
{0x3084,0x01},
{0x3634,0x26},
{0x3086,0x0f},
{0x3086,0x00},
};

static struct msm_camera_i2c_reg_conf ov2675_saturation[][2] = {
	{{0x3394, 0x34}, {0x3395, 0x34},},	/* SATURATION LEVEL0*/
	{{0x3394, 0x38}, {0x3395, 0x38},},	/* SATURATION LEVEL1*/
	{{0x3394, 0x3c}, {0x3395, 0x3c},},	/* SATURATION LEVEL2*/
	{{0x3394, 0x40}, {0x3395, 0x40},},	/* SATURATION LEVEL3*/
	{{0x3394, 0x44}, {0x3395, 0x44},},	/* SATURATION LEVEL4*/
	{{0x3394, 0x48}, {0x3395, 0x48},},/* SATURATION LEVEL5*/
	{{0x3394, 0x4c}, {0x3395, 0x4c},},	/* SATURATION LEVEL6*/
};


static struct msm_camera_i2c_conf_array ov2675_saturation_confs[][1] = {
	{{ov2675_saturation[0], ARRAY_SIZE(ov2675_saturation[0]), 0,
		MSM_CAMERA_I2C_BYTE_DATA},},
	{{ov2675_saturation[1], ARRAY_SIZE(ov2675_saturation[1]), 0,
		MSM_CAMERA_I2C_BYTE_DATA},},
	{{ov2675_saturation[2], ARRAY_SIZE(ov2675_saturation[2]), 0,
		MSM_CAMERA_I2C_BYTE_DATA},},
	{{ov2675_saturation[3], ARRAY_SIZE(ov2675_saturation[3]), 0,
		MSM_CAMERA_I2C_BYTE_DATA},},
	{{ov2675_saturation[4], ARRAY_SIZE(ov2675_saturation[4]), 0,
		MSM_CAMERA_I2C_BYTE_DATA},},
	{{ov2675_saturation[5], ARRAY_SIZE(ov2675_saturation[5]), 0,
		MSM_CAMERA_I2C_BYTE_DATA},},
	{{ov2675_saturation[6], ARRAY_SIZE(ov2675_saturation[6]), 0,
		MSM_CAMERA_I2C_BYTE_DATA},},
};


static int ov2675_saturation_enum_map[] = {
	MSM_V4L2_SATURATION_L0,
	MSM_V4L2_SATURATION_L1,
	MSM_V4L2_SATURATION_L2,
	MSM_V4L2_SATURATION_L3,
	MSM_V4L2_SATURATION_L4,
	MSM_V4L2_SATURATION_L5,
	MSM_V4L2_SATURATION_L6,
};

static struct msm_camera_i2c_enum_conf_array ov2675_saturation_enum_confs = {
	.conf = &ov2675_saturation_confs[0][0],
	.conf_enum = ov2675_saturation_enum_map,
	.num_enum = ARRAY_SIZE(ov2675_saturation_enum_map),
	.num_index = ARRAY_SIZE(ov2675_saturation_confs),
	.num_conf = ARRAY_SIZE(ov2675_saturation_confs[0]),
	.data_type = MSM_CAMERA_I2C_BYTE_DATA,
};

int ov2675_saturation_msm_sensor_s_ctrl_by_enum(
		struct msm_sensor_ctrl_t *s_ctrl,
		struct msm_sensor_v4l2_ctrl_info_t *ctrl_info, int value)
{
	int rc = 0;
	CDBG("%s value : %d  id : %d \n",__func__, value,ctrl_info->ctrl_id);
	if (effect_value == CAMERA_EFFECT_OFF) {
		rc = msm_sensor_write_enum_conf_array(
			s_ctrl->sensor_i2c_client,
			ctrl_info->enum_cfg_settings, value);
	}
	if (value <= MSM_V4L2_SATURATION_L8)
		SAT_U = SAT_V = value * 0x10;
	CDBG("--CAMERA-- %s ...(End)\n", __func__);
	return rc;
}



static struct msm_camera_i2c_reg_conf ov2675_contrast[][4] = {
	{{0x3399, 0x10},},	/* CONTRAST L0*/
	{{0x3399, 0x14},},	/* CONTRAST L1*/
	{{0x3399, 0x18},},	/* CONTRAST L2*/
	{{0x3399, 0x1c},},	/* CONTRAST L3*/
	{{0x3399, 0x20},},	/* CONTRAST L4*/
	{{0x3399, 0x24},},	/* CONTRAST L5*/
	{{0x3399, 0x28},},	/* CONTRAST L6*/
	{{0x3399, 0x2c},},	/* CONTRAST L7*/
	{{0x3399, 0x30},},	/* CONTRAST L8*/
};

static struct msm_camera_i2c_conf_array ov2675_contrast_confs[][1] = {
	{{ov2675_contrast[0], ARRAY_SIZE(ov2675_contrast[0]), 0,
		MSM_CAMERA_I2C_BYTE_DATA},},
	{{ov2675_contrast[1], ARRAY_SIZE(ov2675_contrast[1]), 0,
		MSM_CAMERA_I2C_BYTE_DATA},},
	{{ov2675_contrast[2], ARRAY_SIZE(ov2675_contrast[2]), 0,
		MSM_CAMERA_I2C_BYTE_DATA},},
	{{ov2675_contrast[3], ARRAY_SIZE(ov2675_contrast[3]), 0,
		MSM_CAMERA_I2C_BYTE_DATA},},
	{{ov2675_contrast[4], ARRAY_SIZE(ov2675_contrast[4]), 0,
		MSM_CAMERA_I2C_BYTE_DATA},},
	{{ov2675_contrast[5], ARRAY_SIZE(ov2675_contrast[5]), 0,
		MSM_CAMERA_I2C_BYTE_DATA},},
	{{ov2675_contrast[6], ARRAY_SIZE(ov2675_contrast[6]), 0,
		MSM_CAMERA_I2C_BYTE_DATA},},
	{{ov2675_contrast[7], ARRAY_SIZE(ov2675_contrast[7]), 0,
		MSM_CAMERA_I2C_BYTE_DATA},},
	{{ov2675_contrast[8], ARRAY_SIZE(ov2675_contrast[8]), 0,
		MSM_CAMERA_I2C_BYTE_DATA},},
};

static int ov2675_contrast_enum_map[] = {
	MSM_V4L2_CONTRAST_L0,
	MSM_V4L2_CONTRAST_L1,
	MSM_V4L2_CONTRAST_L2,
	MSM_V4L2_CONTRAST_L3,
	MSM_V4L2_CONTRAST_L4,
	MSM_V4L2_CONTRAST_L5,
	MSM_V4L2_CONTRAST_L6,
	MSM_V4L2_CONTRAST_L7,
	MSM_V4L2_CONTRAST_L8,
};

static struct msm_camera_i2c_enum_conf_array ov2675_contrast_enum_confs = {
	.conf = &ov2675_contrast_confs[0][0],
	.conf_enum = ov2675_contrast_enum_map,
	.num_enum = ARRAY_SIZE(ov2675_contrast_enum_map),
	.num_index = ARRAY_SIZE(ov2675_contrast_confs),
	.num_conf = ARRAY_SIZE(ov2675_contrast_confs[0]),
	.data_type = MSM_CAMERA_I2C_BYTE_DATA,
};


int ov2675_contrast_msm_sensor_s_ctrl_by_enum(
		struct msm_sensor_ctrl_t *s_ctrl,
		struct msm_sensor_v4l2_ctrl_info_t *ctrl_info, int value)
{
	int rc = 0;
	CDBG("%s value : %d  id : %d \n",__func__, value,ctrl_info->ctrl_id);
	if (effect_value == CAMERA_EFFECT_OFF) {
		rc = msm_sensor_write_enum_conf_array(
			s_ctrl->sensor_i2c_client,
			ctrl_info->enum_cfg_settings, value);
	}
	return rc;
}


static struct msm_camera_i2c_reg_conf ov2675_sharpness[][5] = {
	{{0x3376, 0x02},},    /* SHARPNESS LEVEL 0*/
	{{0x3376, 0x03},},    /* SHARPNESS LEVEL 1*/
	{{0x3376, 0x04},},    /* SHARPNESS LEVEL 2*/
	{{0x3376, 0x05},},    /* SHARPNESS LEVEL 3*/
	{{0x3376, 0x06},},    /* SHARPNESS LEVEL 4*/
	{{0x3376, 0x07},},    /* SHARPNESS LEVEL 5*/
	{{0x3376, 0x08},},    /* SHARPNESS LEVEL 6*/	
	{{0x3376, 0x09},},    /* SHARPNESS LEVEL 7*/
	{{0x3376, 0x0a},},    /* SHARPNESS LEVEL 8*/
	{{0x3376, 0x0b},},    /* SHARPNESS LEVEL 9*/	
};


static struct msm_camera_i2c_conf_array ov2675_sharpness_confs[][1] = {
	{{ov2675_sharpness[0], ARRAY_SIZE(ov2675_sharpness[0]), 0,
		MSM_CAMERA_I2C_BYTE_DATA},},
	{{ov2675_sharpness[1], ARRAY_SIZE(ov2675_sharpness[1]), 0,
		MSM_CAMERA_I2C_BYTE_DATA},},
	{{ov2675_sharpness[2], ARRAY_SIZE(ov2675_sharpness[2]), 0,
		MSM_CAMERA_I2C_BYTE_DATA},},
	{{ov2675_sharpness[3], ARRAY_SIZE(ov2675_sharpness[3]), 0,
		MSM_CAMERA_I2C_BYTE_DATA},},
	{{ov2675_sharpness[4], ARRAY_SIZE(ov2675_sharpness[4]), 0,
		MSM_CAMERA_I2C_BYTE_DATA},},
	{{ov2675_sharpness[5], ARRAY_SIZE(ov2675_sharpness[5]), 0,
		MSM_CAMERA_I2C_BYTE_DATA},},
	{{ov2675_sharpness[6], ARRAY_SIZE(ov2675_sharpness[6]), 0,
		MSM_CAMERA_I2C_BYTE_DATA},},
};

static int ov2675_sharpness_enum_map[] = {
	MSM_V4L2_SHARPNESS_L0,
	MSM_V4L2_SHARPNESS_L1,
	MSM_V4L2_SHARPNESS_L2,
	MSM_V4L2_SHARPNESS_L3,
	MSM_V4L2_SHARPNESS_L4,
	MSM_V4L2_SHARPNESS_L5,
	MSM_V4L2_SHARPNESS_L6,
};

static struct msm_camera_i2c_enum_conf_array ov2675_sharpness_enum_confs = {
	.conf = &ov2675_sharpness_confs[0][0],
	.conf_enum = ov2675_sharpness_enum_map,
	.num_enum = ARRAY_SIZE(ov2675_sharpness_enum_map),
	.num_index = ARRAY_SIZE(ov2675_sharpness_confs),
	.num_conf = ARRAY_SIZE(ov2675_sharpness_confs[0]),
	.data_type = MSM_CAMERA_I2C_BYTE_DATA,
};


int ov2675_sharpness_msm_sensor_s_ctrl_by_enum(
		struct msm_sensor_ctrl_t *s_ctrl,
		struct msm_sensor_v4l2_ctrl_info_t *ctrl_info, int value)
{
	int rc = 0;
	CDBG("%s value : %d  id : %d \n",__func__, value,ctrl_info->ctrl_id);
	if (effect_value == CAMERA_EFFECT_OFF) {
		rc = msm_sensor_write_enum_conf_array(
			s_ctrl->sensor_i2c_client,
			ctrl_info->enum_cfg_settings, value);
	}
	return rc;
}


static struct msm_camera_i2c_reg_conf ov2675_exposure[][3] = {
	{{0x3018, 0x5a}, {0x3019, 0x4a}, {0x301a, 0xd4},}, /*EXPOSURECOMPENSATIONN2*/	
	{{0x3018, 0x6a}, {0x3019, 0x5a}, {0x301a, 0xd4},}, /*EXPOSURECOMPENSATIONN1*/
	{{0x3018, 0x78}, {0x3019, 0x68}, {0x301a, 0xd4},}, /*EXPOSURECOMPENSATIONN*/
	{{0x3018, 0x88}, {0x3019, 0x78}, {0x301a, 0xd4},}, /*EXPOSURECOMPENSATIONp1*/	
	{{0x3018, 0x98}, {0x3019, 0x88}, {0x301a, 0xd4},}, /*EXPOSURECOMPENSATIONp2*/
};

static struct msm_camera_i2c_conf_array ov2675_exposure_confs[][1] = {
	{{ov2675_exposure[0], ARRAY_SIZE(ov2675_exposure[0]), 0,
		MSM_CAMERA_I2C_BYTE_DATA},},
	{{ov2675_exposure[1], ARRAY_SIZE(ov2675_exposure[1]), 0,
		MSM_CAMERA_I2C_BYTE_DATA},},
	{{ov2675_exposure[2], ARRAY_SIZE(ov2675_exposure[2]), 0,
		MSM_CAMERA_I2C_BYTE_DATA},},
	{{ov2675_exposure[3], ARRAY_SIZE(ov2675_exposure[3]), 0,
		MSM_CAMERA_I2C_BYTE_DATA},},
	{{ov2675_exposure[4], ARRAY_SIZE(ov2675_exposure[4]), 0,
		MSM_CAMERA_I2C_BYTE_DATA},},
};

static int ov2675_exposure_enum_map[] = {
	MSM_V4L2_EXPOSURE_N2,
	MSM_V4L2_EXPOSURE_N1,
	MSM_V4L2_EXPOSURE_D,
	MSM_V4L2_EXPOSURE_P1,
	MSM_V4L2_EXPOSURE_P2,
};

static struct msm_camera_i2c_enum_conf_array ov2675_exposure_enum_confs = {
	.conf = &ov2675_exposure_confs[0][0],
	.conf_enum = ov2675_exposure_enum_map,
	.num_enum = ARRAY_SIZE(ov2675_exposure_enum_map),
	.num_index = ARRAY_SIZE(ov2675_exposure_confs),
	.num_conf = ARRAY_SIZE(ov2675_exposure_confs[0]),
	.data_type = MSM_CAMERA_I2C_BYTE_DATA,
};

int ov2675_exposure_msm_sensor_s_ctrl_by_enum(struct msm_sensor_ctrl_t *s_ctrl,
		struct msm_sensor_v4l2_ctrl_info_t *ctrl_info, int value)
{
	int rc = 0;
	CDBG("%s value : %d  id : %d \n",__func__, value,ctrl_info->ctrl_id);
	rc = msm_sensor_write_enum_conf_array(
		s_ctrl->sensor_i2c_client,
		ctrl_info->enum_cfg_settings, value);
	if (rc < 0) {
		CDBG("write faield\n");
		return rc;
	}
	return rc;
}

int ov2675_msm_sensor_s_ctrl_by_enum(struct msm_sensor_ctrl_t *s_ctrl,
		struct msm_sensor_v4l2_ctrl_info_t *ctrl_info, int value)
{
	int rc = 0;
	CDBG("%s value : %d  id : %d \n",__func__, value,ctrl_info->ctrl_id);
	rc = msm_sensor_write_enum_conf_array(
		s_ctrl->sensor_i2c_client,
		ctrl_info->enum_cfg_settings, value);
	if (rc < 0) {
		CDBG("write faield\n");
		return rc;
	}
	return rc;
}


static struct msm_camera_i2c_reg_conf ov2675_special_effect[][3] = {
	{{0x3391, 0x06}, {-1, -1},{-1, -1},},	/*for special effect OFF*/
	{{0x3391, 0x26}, {-1, -1},{-1, -1},},				/*for special effect MONO*/
	{{0x3391, 0x46}, {-1, -1},{-1, -1},},				/*for special efefct Negative*/
	{{0x3391, 0x06}, {0x3396, 0x40},{0x3397, 0x40},},	/*Solarize is not supported by sensor*/
	{{0x3391, 0x1e}, {0x3396, 0x65},{0x3397, 0x90},},	/*for sepia*/
	//{{0x3391, 0x1e}, {0x3396, 0x60},{0x3397, 0x90},},	/*for sepia*/
	{{-1, -1}, {-1, -1},{-1, -1},},						/* Posteraize not supported  SEPIAGREEN */
	{{-1, -1}, {-1, -1},{-1, -1},},	/* White board not supported SEPIABLUE*/
	{{-1, -1}, {-1, -1},{-1, -1},},	/*Blackboard not supported*/
	{{-1, -1}, {-1, -1},{-1, -1},},	/*Aqua not supported*/
	{{-1, -1}, {-1, -1},{-1, -1},},	/*Emboss not supported */
	{{-1, -1}, {-1, -1},{-1, -1},},	/*sketch not supported*/
	{{-1, -1}, {-1, -1},{-1, -1},},	/*Neon not supported*/
	{{-1, -1}, {-1, -1},{-1, -1},},	/*MAX value*/
};

static struct msm_camera_i2c_conf_array ov2675_special_effect_confs[][1] = {
	{{ov2675_special_effect[0],  ARRAY_SIZE(ov2675_special_effect[0]),  0,
		MSM_CAMERA_I2C_BYTE_DATA},},
	{{ov2675_special_effect[1],  ARRAY_SIZE(ov2675_special_effect[1]),  0,
		MSM_CAMERA_I2C_BYTE_DATA},},
	{{ov2675_special_effect[2],  ARRAY_SIZE(ov2675_special_effect[2]),  0,
		MSM_CAMERA_I2C_BYTE_DATA},},
	{{ov2675_special_effect[3],  ARRAY_SIZE(ov2675_special_effect[3]),  0,
		MSM_CAMERA_I2C_BYTE_DATA},},
	{{ov2675_special_effect[4],  ARRAY_SIZE(ov2675_special_effect[4]),  0,
		MSM_CAMERA_I2C_BYTE_DATA},},
	{{ov2675_special_effect[5],  ARRAY_SIZE(ov2675_special_effect[5]),  0,
		MSM_CAMERA_I2C_BYTE_DATA},},
	{{ov2675_special_effect[6],  ARRAY_SIZE(ov2675_special_effect[6]),  0,
		MSM_CAMERA_I2C_BYTE_DATA},},
	{{ov2675_special_effect[7],  ARRAY_SIZE(ov2675_special_effect[7]),  0,
		MSM_CAMERA_I2C_BYTE_DATA},},
	{{ov2675_special_effect[8],  ARRAY_SIZE(ov2675_special_effect[8]),  0,
		MSM_CAMERA_I2C_BYTE_DATA},},
	{{ov2675_special_effect[9],  ARRAY_SIZE(ov2675_special_effect[9]),  0,
		MSM_CAMERA_I2C_BYTE_DATA},},
	{{ov2675_special_effect[10], ARRAY_SIZE(ov2675_special_effect[10]), 0,
		MSM_CAMERA_I2C_BYTE_DATA},},
	{{ov2675_special_effect[11], ARRAY_SIZE(ov2675_special_effect[11]), 0,
		MSM_CAMERA_I2C_BYTE_DATA},},
	{{ov2675_special_effect[12], ARRAY_SIZE(ov2675_special_effect[12]), 0,
		MSM_CAMERA_I2C_BYTE_DATA},},
};

static int ov2675_special_effect_enum_map[] = {
	MSM_V4L2_EFFECT_OFF,
	MSM_V4L2_EFFECT_MONO,
	MSM_V4L2_EFFECT_NEGATIVE,
	MSM_V4L2_EFFECT_SOLARIZE,
	MSM_V4L2_EFFECT_SEPIA,
	MSM_V4L2_EFFECT_POSTERAIZE,
	MSM_V4L2_EFFECT_WHITEBOARD,
	MSM_V4L2_EFFECT_BLACKBOARD,
	MSM_V4L2_EFFECT_AQUA,
	MSM_V4L2_EFFECT_EMBOSS,
	MSM_V4L2_EFFECT_SKETCH,
	MSM_V4L2_EFFECT_NEON,
	MSM_V4L2_EFFECT_MAX,
};

struct msm_camera_i2c_enum_conf_array
		 ov2675_special_effect_enum_confs = {
	.conf = &ov2675_special_effect_confs[0][0],
	.conf_enum = ov2675_special_effect_enum_map,
	.num_enum = ARRAY_SIZE(ov2675_special_effect_enum_map),
	.num_index = ARRAY_SIZE(ov2675_special_effect_confs),
	.num_conf = ARRAY_SIZE(ov2675_special_effect_confs[0]),
	.data_type = MSM_CAMERA_I2C_BYTE_DATA,
};

int ov2675_effect_msm_sensor_s_ctrl_by_enum(struct msm_sensor_ctrl_t *s_ctrl,
		struct msm_sensor_v4l2_ctrl_info_t *ctrl_info, int value)
{
	int rc = 0;
	CDBG("%s value : %d  id : %d \n",__func__, value,ctrl_info->ctrl_id);
	effect_value = value;
	rc = msm_sensor_write_enum_conf_array(
			s_ctrl->sensor_i2c_client,
			ctrl_info->enum_cfg_settings, value);
	return rc;
}


static struct msm_camera_i2c_reg_conf ov2675_antibanding[][1] = {
	{{0x3014, 0xc0},},   /*ANTIBANDING 60HZ*/
	{{0x3014, 0x80},},   /*ANTIBANDING 50HZ*/
	{{0x3014, 0x00},},   /* ANTIBANDING AUTO*/
};


static struct msm_camera_i2c_conf_array ov2675_antibanding_confs[][1] = {
	{{ov2675_antibanding[0], ARRAY_SIZE(ov2675_antibanding[0]),  0,
		MSM_CAMERA_I2C_BYTE_DATA},},
	{{ov2675_antibanding[1], ARRAY_SIZE(ov2675_antibanding[1]),  0,
		MSM_CAMERA_I2C_BYTE_DATA},},
	{{ov2675_antibanding[2], ARRAY_SIZE(ov2675_antibanding[2]),  0,
		MSM_CAMERA_I2C_BYTE_DATA},},
};

static int ov2675_antibanding_enum_map[] = {
	MSM_V4L2_POWER_LINE_60HZ,
	MSM_V4L2_POWER_LINE_50HZ,
	MSM_V4L2_POWER_LINE_AUTO,
};


static struct msm_camera_i2c_enum_conf_array ov2675_antibanding_enum_confs = {
	.conf = &ov2675_antibanding_confs[0][0],
	.conf_enum = ov2675_antibanding_enum_map,
	.num_enum = ARRAY_SIZE(ov2675_antibanding_enum_map),
	.num_index = ARRAY_SIZE(ov2675_antibanding_confs),
	.num_conf = ARRAY_SIZE(ov2675_antibanding_confs[0]),
	.data_type = MSM_CAMERA_I2C_BYTE_DATA,
};

static struct msm_camera_i2c_reg_conf ov2675_wb_oem[][4] = {
	{{0x3306, 0x00}, {0x3337, 0x40}, {0x3338, 0x40},	{0x3339,0x40},}, //WHITEBALNACE AUTO
	{{0x3306, 0x02}, {0x3337, 0x5e}, {0x3338, 0x40},	{0x3339,0x58},}, //INCANDISCENT		//purple
	{{0x3306, 0x02}, {0x3337, 0x5e}, {0x3338, 0x40},	{0x3339,0x46},}, //DAYLIGHT			
	{{0x3306, 0x02}, {0x3337, 0x65}, {0x3338, 0x40},	{0x3339,0x41},}, //FLOURESECT NOT SUPPORTED //yellow
	{{0x3306, 0x02}, {0x3337, 0x68}, {0x3338, 0x40},	{0x3339,0x4e},}, //CLOUDY			//red
	{{0x3306, 0x02}, {0x3337, 0x44}, {0x3338, 0x40},	{0x3339,0x70},}, //blue	
	{{0x3306, 0x02}, {0x3337, 0x44}, {0x3338, 0x40},	{0x3339,0x60},}, //blue	
};

static struct msm_camera_i2c_conf_array ov2675_wb_oem_confs[][1] = {
	{{ov2675_wb_oem[0], ARRAY_SIZE(ov2675_wb_oem[0]),  0,
		MSM_CAMERA_I2C_BYTE_DATA},},	// 0
	{{ov2675_wb_oem[0], ARRAY_SIZE(ov2675_wb_oem[0]),  0,
		MSM_CAMERA_I2C_BYTE_DATA},},	// 1
	{{ov2675_wb_oem[0], ARRAY_SIZE(ov2675_wb_oem[0]),  0,
		MSM_CAMERA_I2C_BYTE_DATA},},	// 2
	{{ov2675_wb_oem[5], ARRAY_SIZE(ov2675_wb_oem[5]),  0,
		MSM_CAMERA_I2C_BYTE_DATA},},	// 3
	{{ov2675_wb_oem[6], ARRAY_SIZE(ov2675_wb_oem[6]),  0,
		MSM_CAMERA_I2C_BYTE_DATA},},	// 4
	{{ov2675_wb_oem[2], ARRAY_SIZE(ov2675_wb_oem[2]),  0,
		MSM_CAMERA_I2C_BYTE_DATA},},	// 5
	{{ov2675_wb_oem[3], ARRAY_SIZE(ov2675_wb_oem[3]),  0,
		MSM_CAMERA_I2C_BYTE_DATA},},	// 6		
};

static int ov2675_wb_oem_enum_map[] = {
	MSM_V4L2_WB_OFF,
	MSM_V4L2_WB_AUTO ,
	MSM_V4L2_WB_CUSTOM,
	MSM_V4L2_WB_INCANDESCENT,
	MSM_V4L2_WB_FLUORESCENT,
	MSM_V4L2_WB_DAYLIGHT,
	MSM_V4L2_WB_CLOUDY_DAYLIGHT,
};

static struct msm_camera_i2c_enum_conf_array ov2675_wb_oem_enum_confs = {
	.conf = &ov2675_wb_oem_confs[0][0],
	.conf_enum = ov2675_wb_oem_enum_map,
	.num_enum = ARRAY_SIZE(ov2675_wb_oem_enum_map),
	.num_index = ARRAY_SIZE(ov2675_wb_oem_confs),
	.num_conf = ARRAY_SIZE(ov2675_wb_oem_confs[0]),
	.data_type = MSM_CAMERA_I2C_BYTE_DATA,
};

int ov2675_msm_sensor_wb_oem_by_enum(struct msm_sensor_ctrl_t *s_ctrl,
		struct msm_sensor_v4l2_ctrl_info_t *ctrl_info, int value)
{
	int rc = 0;
	CDBG("%s value : %d  id : %d \n",__func__, value,ctrl_info->ctrl_id);
	rc = msm_sensor_write_enum_conf_array(
		s_ctrl->sensor_i2c_client,
		ctrl_info->enum_cfg_settings, value);
	if (rc < 0) {
		CDBG("write faield\n");
		return rc;
	}
	return rc;
}

int ov2675_antibanding_msm_sensor_s_ctrl_by_enum(struct msm_sensor_ctrl_t *s_ctrl,
		struct msm_sensor_v4l2_ctrl_info_t *ctrl_info, int value)
{
	int rc = 0;
	CDBG("%s value : %d  id : %d \n",__func__, value,ctrl_info->ctrl_id);
	rc = msm_sensor_write_enum_conf_array(
		s_ctrl->sensor_i2c_client,
		ctrl_info->enum_cfg_settings, value);
	if (rc < 0) {
		CDBG("write faield\n");
		return rc;
	}
	return rc;
}


struct msm_sensor_v4l2_ctrl_info_t ov2675_v4l2_ctrl_info[] = {
	{
		.ctrl_id = V4L2_CID_SATURATION,
		.min = MSM_V4L2_SATURATION_L0,
		.max = MSM_V4L2_SATURATION_L8,
		.step = 1,
		.enum_cfg_settings = &ov2675_saturation_enum_confs,
		.s_v4l2_ctrl = ov2675_saturation_msm_sensor_s_ctrl_by_enum,
	},
	{
		.ctrl_id = V4L2_CID_CONTRAST,
		.min = MSM_V4L2_CONTRAST_L0,
		.max = MSM_V4L2_CONTRAST_L8,
		.step = 1,
		.enum_cfg_settings = &ov2675_contrast_enum_confs,
		.s_v4l2_ctrl = ov2675_contrast_msm_sensor_s_ctrl_by_enum,
	},
	{
		.ctrl_id = V4L2_CID_SHARPNESS,
		.min = MSM_V4L2_SHARPNESS_L0,
		.max = MSM_V4L2_SHARPNESS_L5,
		.step = 1,
		.enum_cfg_settings = &ov2675_sharpness_enum_confs,
		.s_v4l2_ctrl = ov2675_sharpness_msm_sensor_s_ctrl_by_enum,
	},
	{
		.ctrl_id = V4L2_CID_EXPOSURE,
		.min = MSM_V4L2_EXPOSURE_N2,
		.max = MSM_V4L2_EXPOSURE_P2,
		.step = 1,
		.enum_cfg_settings = &ov2675_exposure_enum_confs,
		.s_v4l2_ctrl = ov2675_exposure_msm_sensor_s_ctrl_by_enum,
	},
/*	{
		.ctrl_id = MSM_V4L2_PID_ISO,
		.min = MSM_V4L2_ISO_AUTO,
		.max = MSM_V4L2_ISO_1600,
		.step = 1,
		.enum_cfg_settings = NULL,
		.s_v4l2_ctrl = ov2675_msm_sensor_s_ctrl_by_enum,
	},
*/
	{
		.ctrl_id = V4L2_CID_SPECIAL_EFFECT,
		.min = MSM_V4L2_EFFECT_OFF,
		.max = MSM_V4L2_EFFECT_NEGATIVE,
		.step = 1,
		.enum_cfg_settings = &ov2675_special_effect_enum_confs,
		.s_v4l2_ctrl = ov2675_effect_msm_sensor_s_ctrl_by_enum,
	},
	{
		.ctrl_id = V4L2_CID_POWER_LINE_FREQUENCY,
		.min = MSM_V4L2_POWER_LINE_60HZ,
		.max = MSM_V4L2_POWER_LINE_AUTO,
		.step = 1,
		.enum_cfg_settings = &ov2675_antibanding_enum_confs,
		.s_v4l2_ctrl = ov2675_antibanding_msm_sensor_s_ctrl_by_enum,
	},
	{
		.ctrl_id = V4L2_CID_WHITE_BALANCE_TEMPERATURE,
		.min = MSM_V4L2_WB_OFF,
		.max = MSM_V4L2_WB_CLOUDY_DAYLIGHT,
		.step = 1,
		.enum_cfg_settings = &ov2675_wb_oem_enum_confs,
		.s_v4l2_ctrl = ov2675_msm_sensor_wb_oem_by_enum,
	},

};


//image quality setting
struct msm_camera_i2c_reg_conf ov2675_init_iq_settings[] = {

};


static struct msm_camera_i2c_conf_array ov2675_init_conf[] = {
	{&ov2675_init_settings[0],
	ARRAY_SIZE(ov2675_init_settings), 0, MSM_CAMERA_I2C_BYTE_DATA},
};

static struct msm_camera_i2c_conf_array ov2675_confs[] = {
	{&ov2675_snap_settings[0],
	ARRAY_SIZE(ov2675_snap_settings), 0, MSM_CAMERA_I2C_BYTE_DATA},
	{&ov2675_prev_30fps_settings[0],
	ARRAY_SIZE(ov2675_prev_30fps_settings), 0, MSM_CAMERA_I2C_BYTE_DATA},
};

static struct msm_camera_csi_params ov2675_csi_params = {
	.data_format = CSI_8BIT,
	.lane_cnt    = 1,
	.lane_assign = 0xe4,
	.dpcm_scheme = 0,
	.settle_cnt  = 0x6,
};

static struct v4l2_subdev_info ov2675_subdev_info[] = {
	{
	.code	= V4L2_MBUS_FMT_YUYV8_2X8,
	.colorspace = V4L2_COLORSPACE_JPEG,
	.fmt	= 1,
	.order	  = 0,
	}
	/* more can be supported, to be added later */
};

static struct msm_sensor_output_info_t ov2675_dimensions[] = {
#if 0
	{ /* For SNAPSHOT */
		.x_output = 1600,	/*640*/  
		.y_output = 1200,	/*480*/
		.line_length_pclk = 0x658,//0x320,
		.frame_length_lines = 0x4bc,
		.vt_pixel_clk = 29524320,// 17974530,
		.op_pixel_clk = 29524320,// 17974530,
		.binning_factor = 0x0,
	},
	{ /* For PREVIEW */
		.x_output = 1600,//0x280,	//640 //1280,//
		.y_output = 1200,//0x1e0,	//480 //720,//
		.line_length_pclk = 0x658,//0x320,
		.frame_length_lines = 0x26a,
		.vt_pixel_clk = 29524320,//15054480,// 17974530,
		.op_pixel_clk = 29524320,//15054480,// 17974530,
		.binning_factor = 0x0,
	},	
	{ /* For SNAPSHOT */
		.x_output = 1600,	/*640*/  
		.y_output = 1200,	/*480*/
		.line_length_pclk = 0x793,//0x320,
		.frame_length_lines = 0x5cb,
		.vt_pixel_clk = 71888425,//43133055,// 17974530,
		.op_pixel_clk = 71888425,//43133055,// 17974530,
		.binning_factor = 0x0,
	},
	{ /* For PREVIEW */
		.x_output = 1600,//0x280,	//640 //1280,//
		.y_output = 1200,//0x1e0,	//480 //720,//
		.line_length_pclk = 0x793,//0x320,
		.frame_length_lines = 0x5cb,//0x2e6,
		.vt_pixel_clk = 71888425,//35968450,//21581070,//15054480,// 17974530,
		.op_pixel_clk = 71888425,//35968450,//21581070,//15054480,// 17974530,
		.binning_factor = 0x0,
	},	
#endif

	{ /* For SNAPSHOT */
		.x_output = 1600,	/*640*/  
		.y_output = 1200,	/*480*/
		.line_length_pclk = 0x658,//0x320,
		.frame_length_lines = 0x4bc,
		.vt_pixel_clk = 49207200,//29524320,//43133055,// 17974530,
		.op_pixel_clk = 49207200,//29524320,//43133055,// 17974530,
		.binning_factor = 0x0,
	},
	{ /* For PREVIEW */
		.x_output = 1600,	/*640*/  
		.y_output = 1200,	/*480*/
		.line_length_pclk = 0x658,//0x320,
		.frame_length_lines = 0x4bc,
		.vt_pixel_clk = 49207200,//29524320,//43133055,// 17974530,
		.op_pixel_clk = 49207200,//29524320,//43133055,// 17974530,
		.binning_factor = 0x0,
	},		
};

static struct msm_sensor_output_reg_addr_t ov2675_reg_addr = {
	.x_output = 0x3088,
	.y_output = 0x308A,
	.line_length_pclk = 0x3024,
	.frame_length_lines = 0x3026,
};

static struct msm_camera_csi_params *ov2675_csi_params_array[] = {
	&ov2675_csi_params,
	&ov2675_csi_params,

};

static struct msm_sensor_id_info_t ov2675_id_info = {
	.sensor_id_reg_addr = 0x300a,
	.sensor_id = 0x2656,
};

static struct msm_sensor_exp_gain_info_t ov2675_exp_gain_info = {
	.coarse_int_time_addr = 0x3500,
	.global_gain_addr = 0x3000,
	.vert_offset = 4,
};

static int32_t ov2675_write_exp_gain(struct msm_sensor_ctrl_t *s_ctrl,
		uint16_t gain, uint32_t line)
{
	CDBG("ov2675_write_exp_gain : Not supported\n");
	return 0;
}

int32_t ov2675_sensor_set_fps(struct msm_sensor_ctrl_t *s_ctrl,
		struct fps_cfg *fps)
{
	CDBG("ov2675_sensor_set_fps: Not supported\n");
	return 0;
}

static const struct i2c_device_id ov2675_i2c_id[] = {
	{SENSOR_NAME, (kernel_ulong_t)&ov2675_s_ctrl},
	{ }
};

int32_t ov2675_sensor_i2c_probe(struct i2c_client *client,
		const struct i2c_device_id *id)
{
	int32_t rc = 0;
	struct msm_sensor_ctrl_t *s_ctrl;

	CDBG("%s IN\r\n", __func__);

	s_ctrl = (struct msm_sensor_ctrl_t *)(id->driver_data);
	s_ctrl->sensor_i2c_addr = s_ctrl->sensor_i2c_addr;//offset to avoid i2c conflicts

	rc = msm_sensor_i2c_probe(client, id);

	if (client->dev.platform_data == NULL) {
		CDBG_HIGH("%s: NULL sensor data\n", __func__);
		return -EFAULT;
	}
	usleep_range(5000, 5100);
	return rc;
}

static struct i2c_driver ov2675_i2c_driver = {
	.id_table = ov2675_i2c_id,
	.probe  = ov2675_sensor_i2c_probe,
	.driver = {
		.name = SENSOR_NAME,
	},
};

static struct msm_camera_i2c_client ov2675_sensor_i2c_client = {
	.addr_type = MSM_CAMERA_I2C_WORD_ADDR,
};

static int __init msm_sensor_init_module(void)
{
	return i2c_add_driver(&ov2675_i2c_driver);
}

static struct v4l2_subdev_core_ops ov2675_subdev_core_ops = {
	.s_ctrl = msm_sensor_v4l2_s_ctrl,
	.queryctrl = msm_sensor_v4l2_query_ctrl,	
	.ioctl = msm_sensor_subdev_ioctl,
	.s_power = msm_sensor_power,
};

static struct v4l2_subdev_video_ops ov2675_subdev_video_ops = {
	.enum_mbus_fmt = msm_sensor_v4l2_enum_fmt,
};

static struct v4l2_subdev_ops ov2675_subdev_ops = {
	.core = &ov2675_subdev_core_ops,
	.video  = &ov2675_subdev_video_ops,
};
int32_t ov2675_sensor_power_up(struct msm_sensor_ctrl_t *s_ctrl)
{
	int32_t rc = 0;
	struct msm_camera_sensor_info *info = NULL;
	info = s_ctrl->sensordata;

	CDBG("%s IN\r\n", __func__);
	CDBG("%s, sensor_pwd:%d, sensor_reset:%d\r\n",__func__, info->sensor_pwd, info->sensor_reset);
	gpio_direction_output(info->sensor_pwd, 1);
	gpio_direction_output(info->sensor_reset, 0);
	usleep_range(10000, 11000);
       /*
	if (info->pmic_gpio_enable) {
		lcd_camera_power_onoff(1);
	}
      */
	usleep_range(10000, 11000);

	rc = msm_sensor_power_up(s_ctrl);
	if (rc < 0) {
		CDBG("%s: msm_sensor_power_up failed\n", __func__);
		return rc;
	}

	usleep_range(1000, 1100);
	/* turn on ldo and vreg */
	gpio_direction_output(info->sensor_pwd, 0);
	msleep(20);
	gpio_direction_output(info->sensor_reset, 1);
	msleep(25);

	return rc;

}

int32_t ov2675_sensor_power_down(struct msm_sensor_ctrl_t *s_ctrl)
{
	int32_t rc = 0;

	struct msm_camera_sensor_info *info = NULL;

	CDBG("%s IN\r\n", __func__);
	info = s_ctrl->sensordata;
	
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client,
		0x3086, 0x0f,
		MSM_CAMERA_I2C_BYTE_DATA);
	//msm_sensor_stop_stream(s_ctrl);
	//msleep(100);
	
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client,
		0x30AB, 0x00,
		MSM_CAMERA_I2C_BYTE_DATA);
	//msleep(5);
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client,
		0x30AD, 0x0A,
		MSM_CAMERA_I2C_BYTE_DATA);
	//msleep(5);
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client,
		0x30AE, 0x27,
		MSM_CAMERA_I2C_BYTE_DATA);
	//msleep(5);
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client,
		0x363B, 0x01,
		MSM_CAMERA_I2C_BYTE_DATA);	
	//msleep(5);

	gpio_direction_output(info->sensor_pwd, 1);
	//usleep_range(5000, 5100);
	gpio_direction_output(info->sensor_reset, 0);
	//msleep(10);
	rc = msm_sensor_power_down(s_ctrl);
	msleep(10);
	//if (s_ctrl->sensordata->pmic_gpio_enable){
	//	lcd_camera_power_onoff(0);
	//}
	return rc;
}


int32_t ov2675_sensor_setting(struct msm_sensor_ctrl_t *s_ctrl,
			int update_type, int res)
{
	int32_t rc = 0;
	static int csi_config;
	ov2675_v4l2_ctrl = s_ctrl;
	if (update_type == MSM_SENSOR_REG_INIT) {
		CDBG("Register INIT\n");
		s_ctrl->func_tbl->sensor_stop_stream(s_ctrl);
		msleep(20);
		s_ctrl->curr_csi_params = NULL;
		msm_sensor_enable_debugfs(s_ctrl);
		msm_sensor_write_init_settings(s_ctrl);
		csi_config = 0;
	} else if (update_type == MSM_SENSOR_UPDATE_PERIODIC) {
		CDBG("PERIODIC : %d\n", res);
		if (!csi_config)
		{
			s_ctrl->curr_csic_params = s_ctrl->csic_params[res];
			CDBG("CSI config in progress\n");
			v4l2_subdev_notify(&s_ctrl->sensor_v4l2_subdev,
				NOTIFY_CSIC_CFG, s_ctrl->curr_csic_params);
			CDBG("CSI config is done\n");
			mb();
			msleep(30);
			csi_config = 1;
		}
		
		if (res == MSM_SENSOR_RES_QTR)
		{
			// preview setting 
			msm_sensor_write_conf_array(s_ctrl->sensor_i2c_client,
			s_ctrl->msm_sensor_reg->mode_settings, res);
		}else if (res == MSM_SENSOR_RES_FULL)
		{
			msm_sensor_write_conf_array(s_ctrl->sensor_i2c_client,
						s_ctrl->msm_sensor_reg->mode_settings, res);
			//msleep(30);
		} 		

		s_ctrl->func_tbl->sensor_start_stream(s_ctrl);
		msleep(30);
	}
	return rc;
}

static struct msm_sensor_fn_t ov2675_func_tbl = {
	.sensor_start_stream = msm_sensor_start_stream,
	.sensor_stop_stream = msm_sensor_stop_stream,
	//.sensor_group_hold_on = msm_sensor_group_hold_on,
	//.sensor_group_hold_off = msm_sensor_group_hold_off,
	//.sensor_set_fps = ov2675_sensor_set_fps,
	.sensor_write_exp_gain = msm_sensor_write_exp_gain1,//ov2675_write_exp_gain,
	.sensor_write_snapshot_exp_gain = ov2675_write_exp_gain,
	.sensor_csi_setting = ov2675_sensor_setting,
	.sensor_set_sensor_mode = msm_sensor_set_sensor_mode,
	.sensor_mode_init = msm_sensor_mode_init,
	.sensor_get_output_info = msm_sensor_get_output_info,
	.sensor_config = msm_sensor_config,
	.sensor_power_up = ov2675_sensor_power_up,
	.sensor_power_down = ov2675_sensor_power_down,
};

static struct msm_sensor_reg_t ov2675_regs = {
	.default_data_type = MSM_CAMERA_I2C_BYTE_DATA,
	.start_stream_conf = ov2675_start_settings,
	.start_stream_conf_size = ARRAY_SIZE(ov2675_start_settings),
	.stop_stream_conf = ov2675_stop_settings,
	.stop_stream_conf_size = ARRAY_SIZE(ov2675_stop_settings),
	//.group_hold_on_conf = NULL,
	//.group_hold_on_conf_size = 0,
	//.group_hold_off_conf = NULL,
	//.group_hold_off_conf_size = 0,
	.init_settings = &ov2675_init_conf[0],
	.init_size = ARRAY_SIZE(ov2675_init_conf),
	.mode_settings = &ov2675_confs[0],
	.output_settings = &ov2675_dimensions[0],
	.num_conf = ARRAY_SIZE(ov2675_confs),
};

static struct msm_sensor_ctrl_t ov2675_s_ctrl = {
	.msm_sensor_reg = &ov2675_regs,
	.msm_sensor_v4l2_ctrl_info = ov2675_v4l2_ctrl_info,
	.num_v4l2_ctrl = ARRAY_SIZE(ov2675_v4l2_ctrl_info),
	.sensor_i2c_client = &ov2675_sensor_i2c_client,
	.sensor_i2c_addr =  0x60,
	.sensor_output_reg_addr = &ov2675_reg_addr,
	.sensor_id_info = &ov2675_id_info,
	.sensor_exp_gain_info = &ov2675_exp_gain_info,
	.cam_mode = MSM_SENSOR_MODE_INVALID,
	.csic_params = &ov2675_csi_params_array[0],
	.msm_sensor_mutex = &ov2675_mut,
	.sensor_i2c_driver = &ov2675_i2c_driver,
	.sensor_v4l2_subdev_info = ov2675_subdev_info,
	.sensor_v4l2_subdev_info_size = ARRAY_SIZE(ov2675_subdev_info),
	.sensor_v4l2_subdev_ops = &ov2675_subdev_ops,
	.func_tbl = &ov2675_func_tbl,
	.clk_rate = MSM_SENSOR_MCLK_24HZ,
};

module_init(msm_sensor_init_module);
MODULE_DESCRIPTION("Omnivision WXGA YUV sensor driver");
MODULE_LICENSE("GPL v2");
