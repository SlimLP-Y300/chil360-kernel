
#ifndef __HIMAX_TS_h
#define __HIMAX_TS_h

#include <linux/ioctl.h> /* needed for the _IOW etc stuff used later */



//#define OutPutDbgMsg		//linxc 2012-07-17 update

#define REPORT_TYPE_ANONYMOUS_CONTACT

#define SUPPORT_HIMAX_TP_FW_UPDATE	//linxc upport at 12-08-15

#define SUPPORT_CHECKSUM_ON_CHIP	//linxc 12-08-13

#define SUPPORT_SYSFS_FW_REVISION	//linxc 12-08-16

#ifdef OutPutDbgMsg
#define DMsg(arg...) printk(arg)
#else
#define DMsg(arg...)
#endif

#define TOUCHPOINT_STACK_LENGTH 28
#define TOUCHPOINT_MAX_NUM 5
#define RESOLUTION_X  540
#define RESOLUTION_Y  960

#define SENSOR_KEY_NUMBER  3

enum{
	POINT_AREA_NONE,       //press UP
	POINT_AREA_DISPLAY,	//press Down
};

#define EVENT_DOWN 1
#define EVENT_UP 0

void himax_set_chg_status(int chg_on);



#endif // __HIMAX_TS_h
