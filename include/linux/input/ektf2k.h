#ifndef _LINUX_ELAN_KTF2K_H
#define _LINUX_ELAN_KTF2K_H

#define ELAN_X_MAX  640
#define ELAN_Y_MAX  1195
#define ELAN_Y_FULL 1280

#define ELAN_TS_GPIO  48

#define ELAN_KTF2K_NAME "ekt2132"

struct elan_ktf2k_i2c_platform_data {
	uint16_t version;
	int abs_x_min;
	int abs_x_max;
	int abs_y_min;
	int abs_y_max;
	int intr_gpio;
	int (*power)(int on);
};

#endif /* _LINUX_ELAN_KTF2K_H */
