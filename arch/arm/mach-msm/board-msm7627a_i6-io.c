/* Copyright (c) 2012, The Linux Foundation. All Rights Reserved.
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

#include <linux/platform_device.h>
#include <linux/regulator/consumer.h>
#include <linux/gpio_event.h>
#include <linux/leds.h>
//#include <linux/i2c/atmel_mxt_ts.h>
#include <linux/i2c.h>
//#include <linux/input/rmi_platformdata.h>
//#include <linux/input/rmi_i2c.h>
#include <linux/delay.h>
//#include <linux/atmel_maxtouch.h>
#include <linux/input/ft5x06_ts.h>
#include <linux/input/ektf2k.h>
#include <linux/leds-msm-tricolor.h>
#include <asm/gpio.h>
#include <asm/mach-types.h>
#include <mach/rpc_server_handset.h>
#include <mach/pmic.h>

#include "devices.h"
#include "board-msm7627a.h"
#include "devices-msm7x2xa.h"

#define MACHINE_IS_JSR_I6  \
 (machine_is_msm7627a_evb() || machine_is_msm8625_evb() || machine_is_msm8625_qrd5() || machine_is_msm7x27a_qrd5a())

#define MACHINE_IS_JSR_I6Q  \
 (machine_is_msm8625_skud())


 
static int tp_id = 0;
static int __init lk_ctpid_setup(char *str)
{
	tp_id = simple_strtol(str, NULL, 0);
	pr_err("get TouchPanel_id = %d\n", tp_id);
	return 1;
}
__setup("ctpid=", lk_ctpid_setup); 


#define KP_INDEX(row, col) ((row)*ARRAY_SIZE(kp_col_gpios) + (col))

static unsigned int kp_row_gpios[] = {31, 32, 33, 34, 35};
static unsigned int kp_col_gpios[] = {36, 37, 38, 39, 40};

static const unsigned short keymap[ARRAY_SIZE(kp_col_gpios) *
					  ARRAY_SIZE(kp_row_gpios)] = {
	[KP_INDEX(0, 0)] = KEY_7,
	[KP_INDEX(0, 1)] = KEY_DOWN,
	[KP_INDEX(0, 2)] = KEY_UP,
	[KP_INDEX(0, 3)] = KEY_RIGHT,
	[KP_INDEX(0, 4)] = KEY_ENTER,

	[KP_INDEX(1, 0)] = KEY_LEFT,
	[KP_INDEX(1, 1)] = KEY_SEND,
	[KP_INDEX(1, 2)] = KEY_1,
	[KP_INDEX(1, 3)] = KEY_4,
	[KP_INDEX(1, 4)] = KEY_CLEAR,

	[KP_INDEX(2, 0)] = KEY_6,
	[KP_INDEX(2, 1)] = KEY_5,
	[KP_INDEX(2, 2)] = KEY_8,
	[KP_INDEX(2, 3)] = KEY_3,
	[KP_INDEX(2, 4)] = KEY_NUMERIC_STAR,

	[KP_INDEX(3, 0)] = KEY_9,
	[KP_INDEX(3, 1)] = KEY_NUMERIC_POUND,
	[KP_INDEX(3, 2)] = KEY_0,
	[KP_INDEX(3, 3)] = KEY_2,
	[KP_INDEX(3, 4)] = KEY_SLEEP,

	[KP_INDEX(4, 0)] = KEY_BACK,
	[KP_INDEX(4, 1)] = KEY_HOME,
	[KP_INDEX(4, 2)] = KEY_MENU,
	[KP_INDEX(4, 3)] = KEY_VOLUMEUP,
	[KP_INDEX(4, 4)] = KEY_VOLUMEDOWN,
};

/* 8625 keypad device information */
static unsigned int kp_row_gpios_8625[] = {31};
static unsigned int kp_col_gpios_8625[] = {36, 37};

static const unsigned short keymap_8625[] = {
    KEY_VOLUMEUP,    
    KEY_VOLUMEDOWN,
      
	
};

static struct gpio_event_matrix_info kp_matrix_info_8625 = {
	.info.func      = gpio_event_matrix_func,
	.keymap         = keymap_8625,
	.output_gpios   = kp_row_gpios_8625,
	.input_gpios    = kp_col_gpios_8625,
	.noutputs       = ARRAY_SIZE(kp_row_gpios_8625),
	.ninputs        = ARRAY_SIZE(kp_col_gpios_8625),
	.settle_time.tv64 = 40 * NSEC_PER_USEC,
	.poll_time.tv64 = 20 * NSEC_PER_MSEC,
	.flags          = GPIOKPF_LEVEL_TRIGGERED_IRQ | GPIOKPF_DRIVE_INACTIVE |
			  GPIOKPF_PRINT_UNMAPPED_KEYS,
};

static struct gpio_event_info *kp_info_8625[] = {
	&kp_matrix_info_8625.info,
};

static struct gpio_event_platform_data kp_pdata_8625 = {
	.name           = "7x27a_kp",
	.info           = kp_info_8625,
	.info_count     = ARRAY_SIZE(kp_info_8625)
};

static struct platform_device kp_pdev_8625 = {
	.name   = GPIO_EVENT_DEV_NAME,
	.id     = -1,
	.dev    = {
		.platform_data  = &kp_pdata_8625,
	},
};

#define MAX_VKEY_LEN		100


static struct elan_ktf2k_i2c_platform_data ts_elan_ktf2k_data = {
	.version = 1,
	.abs_x_min = 0,
	.abs_x_max = ELAN_X_MAX,
	.abs_y_min = 0,
	.abs_y_max = ELAN_Y_MAX,
	.intr_gpio = ELAN_TS_GPIO,
	.power = NULL,
};

static struct i2c_board_info etf2k_ts_i2c_info[] = {
	{
		I2C_BOARD_INFO(ELAN_KTF2K_NAME, 0x15),
		.platform_data = &ts_elan_ktf2k_data,
		.irq = MSM_GPIO_TO_INT(ELAN_TS_GPIO),
	},
};


static struct msm_handset_platform_data hs_platform_data = {
	.hs_name = "7k_handset",
	.pwr_key_delay_ms = 500, /* 0 will disable end key */
};

static struct platform_device hs_pdev = {
	.name   = "msm-handset",
	.id     = -1,
	.dev    = {
		.platform_data = &hs_platform_data,
	},
};

/*
#define FT5X06_IRQ_GPIO		48
#define FT5X06_RESET_GPIO	26

#define FT5X06_IRQ_GPIO_QPR_SKUD	122
#define FT5X06_RESET_GPIO_QPR_SKUD	26

#define FT5X06_IRQ_GPIO_QPR_SKUE	121
#define FT5X06_RESET_GPIO_QPR_SKUE	26

static ssize_t
ft5x06_virtual_keys_register(struct kobject *kobj,
			     struct kobj_attribute *attr,
			     char *buf)
{
	if (machine_is_msm8625q_skud()) {
		return snprintf(buf, 200,
			__stringify(EV_KEY) ":" __stringify(KEY_HOME)  ":67:1000:135:60"
			":" __stringify(EV_KEY) ":" __stringify(KEY_MENU)   ":202:1000:135:60"
			":" __stringify(EV_KEY) ":" __stringify(KEY_BACK) ":337:1000:135:60"
			":" __stringify(EV_KEY) ":" __stringify(KEY_SEARCH)   ":472:1000:135:60"
			"\n");
	} if (machine_is_msm8625q_skue()) {
		return snprintf(buf, 200,
			__stringify(EV_KEY) ":" __stringify(KEY_MENU)  ":90:1020:170:40"
			":" __stringify(EV_KEY) ":" __stringify(KEY_HOME)   ":270:1020:170:40"
			":" __stringify(EV_KEY) ":" __stringify(KEY_BACK) ":450:1020:170:40"
			"\n");
	} else {
		return snprintf(buf, 200,
			__stringify(EV_KEY) ":" __stringify(KEY_MENU)  ":40:510:80:60"
			":" __stringify(EV_KEY) ":" __stringify(KEY_HOME)   ":120:510:80:60"
			":" __stringify(EV_KEY) ":" __stringify(KEY_SEARCH) ":200:510:80:60"
			":" __stringify(EV_KEY) ":" __stringify(KEY_BACK)   ":280:510:80:60"
			"\n");
	}
}

static struct kobj_attribute ft5x06_virtual_keys_attr = {
	.attr = {
		.name = "virtualkeys.ft5x06_ts",
		.mode = S_IRUGO,
	},
	.show = &ft5x06_virtual_keys_register,
};

static struct attribute *ft5x06_virtual_key_properties_attrs[] = {
	&ft5x06_virtual_keys_attr.attr,
	NULL,
};

static struct attribute_group ft5x06_virtual_key_properties_attr_group = {
	.attrs = ft5x06_virtual_key_properties_attrs,
};

struct kobject *ft5x06_virtual_key_properties_kobj;

static struct regulator_bulk_data regs_ft5x06[] = {
	{ .supply = "ldo12", .min_uV = 2700000, .max_uV = 3300000 },
	{ .supply = "smps3", .min_uV = 1800000, .max_uV = 1800000 },
};

static int ft5x06_ts_power_on(bool on)
{
	int rc;

	rc = regulator_bulk_get(NULL, ARRAY_SIZE(regs_ft5x06), regs_ft5x06);
	if (rc) {
		printk("%s: could not get regulators: %d\n",
				__func__, rc);
	}

	rc = regulator_bulk_set_voltage(ARRAY_SIZE(regs_ft5x06), regs_ft5x06);
	if (rc) {
		printk("%s: could not set voltages: %d\n",
				__func__, rc);
	}

	rc = on ?
		regulator_bulk_enable(ARRAY_SIZE(regs_ft5x06), regs_ft5x06) :
		regulator_bulk_disable(ARRAY_SIZE(regs_ft5x06), regs_ft5x06);

	if (rc)
		pr_err("%s: could not %sable regulators: %d\n",
				__func__, on ? "en" : "dis", rc);
	else
		msleep(50);

	return rc;
}

static struct ft5x06_ts_platform_data ft5x06_platformdata = {
	.x_max		= 320,
	.y_max		= 480,
	.reset_gpio	= FT5X06_RESET_GPIO,
	.irq_gpio	= FT5X06_IRQ_GPIO,
	.irqflags	= IRQF_TRIGGER_FALLING | IRQF_ONESHOT,
	.power_on	= ft5x06_ts_power_on,
};

static struct i2c_board_info ft5x06_device_info[] __initdata = {
	{
		I2C_BOARD_INFO("ft5x06_ts", 0x38),
		.platform_data = &ft5x06_platformdata,
		.irq = MSM_GPIO_TO_INT(FT5X06_IRQ_GPIO),
	},
};

static void __init ft5x06_touchpad_setup(void)
{
	int rc;

	if (machine_is_msm8625q_skud()) {
		ft5x06_platformdata.irq_gpio = FT5X06_IRQ_GPIO_QPR_SKUD;
		ft5x06_platformdata.reset_gpio = FT5X06_RESET_GPIO_QPR_SKUD;
		ft5x06_platformdata.x_max = 540;
		ft5x06_platformdata.y_max = 960;
		ft5x06_device_info[0].irq = MSM_GPIO_TO_INT(FT5X06_IRQ_GPIO_QPR_SKUD);
	} else if (machine_is_msm8625q_skue()) {
		ft5x06_platformdata.irq_gpio = FT5X06_IRQ_GPIO_QPR_SKUE;
		ft5x06_platformdata.reset_gpio = FT5X06_RESET_GPIO_QPR_SKUE;
		ft5x06_platformdata.x_max = 540;
		ft5x06_platformdata.y_max = 960;
		ft5x06_device_info[0].irq = MSM_GPIO_TO_INT(FT5X06_IRQ_GPIO_QPR_SKUE);
	}

	rc = gpio_tlmm_config(GPIO_CFG(ft5x06_platformdata.irq_gpio, 0,
			GPIO_CFG_INPUT, GPIO_CFG_PULL_UP,
			GPIO_CFG_8MA), GPIO_CFG_ENABLE);
	if (rc)
		pr_err("%s: gpio_tlmm_config for %d failed\n",
			__func__, ft5x06_platformdata.irq_gpio);

	rc = gpio_tlmm_config(GPIO_CFG(ft5x06_platformdata.reset_gpio, 0,
			GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN,
			GPIO_CFG_8MA), GPIO_CFG_ENABLE);
	if (rc)
		pr_err("%s: gpio_tlmm_config for %d failed\n",
			__func__, ft5x06_platformdata.reset_gpio);

	ft5x06_virtual_key_properties_kobj =
			kobject_create_and_add("board_properties", NULL);

	if (ft5x06_virtual_key_properties_kobj)
		rc = sysfs_create_group(ft5x06_virtual_key_properties_kobj,
				&ft5x06_virtual_key_properties_attr_group);

	if (!ft5x06_virtual_key_properties_kobj || rc)
		pr_err("%s: failed to create board_properties\n", __func__);

	i2c_register_board_info(MSM_GSBI1_QUP_I2C_BUS_ID,
				ft5x06_device_info,
				ARRAY_SIZE(ft5x06_device_info));
}
*/

#ifdef CONFIG_LEDS_TRICOLOR_FLAHSLIGHT


#define LED_FLASH_EN1 13
#define QRD7_LED_FLASH_EN 96

static struct msm_gpio tricolor_leds_gpio_cfg_data[] = {
{
	GPIO_CFG(-1, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
		"flashlight"},
};


static int tricolor_leds_gpio_setup(void) {
	int ret = 0;
	if(machine_is_msm8625_qrd5() || machine_is_msm7x27a_qrd5a())
	{
		tricolor_leds_gpio_cfg_data[0].gpio_cfg = GPIO_CFG(LED_FLASH_EN1, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA);
	}
	else if(machine_is_msm8625_qrd7())
	{
		tricolor_leds_gpio_cfg_data[0].gpio_cfg = GPIO_CFG(QRD7_LED_FLASH_EN, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA);
	}

	ret = msm_gpios_request_enable(tricolor_leds_gpio_cfg_data,
			sizeof(tricolor_leds_gpio_cfg_data)/sizeof(struct msm_gpio));
	if( ret<0 )
		printk(KERN_ERR "%s: Failed to obtain tricolor_leds GPIO . Code: %d\n",
				__func__, ret);
	return ret;
}


static struct platform_device msm_device_tricolor_leds = {
	.name   = "tricolor leds and flashlight",
	.id = -1,
};
#endif

#if 0
static struct pmic8029_led_platform_data leds_data[] = {
	{
		.name = "button-backlight",
		.which = PM_MPP_7,
		.type = PMIC8029_DRV_TYPE_CUR,
		.max.cur = PM_MPP__I_SINK__LEVEL_40mA,
	},
};

static struct pmic8029_leds_platform_data pmic8029_leds_pdata = {
	.leds = leds_data,
	.num_leds = 1,
};

static struct platform_device pmic_mpp_leds_pdev = {
	.name   = "pmic-mpp-leds",
	.id     = -1,
	.dev    = {
		.platform_data	= &pmic8029_leds_pdata,
	},
};
#endif
#if 0
static struct led_info tricolor_led_info[] = {
	[0] = {
		.name           = "red",
		.flags          = LED_COLOR_RED,
	},
	[1] = {
		.name           = "green",
		.flags          = LED_COLOR_GREEN,
	},
	[2] = {
		.name           = "blue",
		.flags          = LED_COLOR_BLUE,
	},
};

static struct led_platform_data tricolor_led_pdata = {
	.leds = tricolor_led_info,
	.num_leds = ARRAY_SIZE(tricolor_led_info),
};

static struct platform_device tricolor_leds_pdev = {
	.name   = "msm-tricolor-leds",
	.id     = -1,
	.dev    = {
		.platform_data  = &tricolor_led_pdata,
	},
};

#endif
void __init msm7627a_add_io_devices(void)
{
	return;
}

void __init qrd7627a_add_io_devices(void)
{
	/* touchscreen */
	if (machine_is_msm7627a_qrd1()) {
		//i2c_register_board_info(MSM_GSBI1_QUP_I2C_BUS_ID, synaptic_i2c_clearpad3k, ARRAY_SIZE(synaptic_i2c_clearpad3k));
	} else 
	if (MACHINE_IS_JSR_I6) {
		if (machine_is_msm8625_qrd5()) {
			pr_err("%s: tp_id=%d\n", __func__, tp_id);
			if (tp_id == 2) {
				//i2c_register_board_info(MSM_GSBI1_QUP_I2C_BUS_ID, ft5306_i2c_device, ARRAY_SIZE(ft5306_i2c_device));
			} else {
				i2c_register_board_info(MSM_GSBI1_QUP_I2C_BUS_ID, etf2k_ts_i2c_info, ARRAY_SIZE(etf2k_ts_i2c_info));
				if (gpio_request(ELAN_TS_GPIO, "Touch IRQ") < 0)
					pr_err("Failed to request GPIO %d for Touch IRQ\n", ELAN_TS_GPIO);
			}
		}
	} else
	if (machine_is_msm7627a_qrd3() || machine_is_msm8625_qrd7() || machine_is_msm8625q_skud() || machine_is_msm8625q_skue()) {
		//ft5x06_touchpad_setup();
	}

	/* headset */
	platform_device_register(&hs_pdev);

	/* vibrator */
#ifdef CONFIG_MSM_RPC_VIBRATOR
	msm_init_pmic_vibrator();
#endif
#if 0
	/* keypad */
	if (machine_is_msm8625_qrd5() || machine_is_msm7x27a_qrd5a())
		kp_matrix_info_8625.keymap = keymap_8625_qrd5;
#endif
	if (MACHINE_IS_JSR_I6)
		platform_device_register(&kp_pdev_8625);
/*
	else if (machine_is_msm7627a_qrd3() || machine_is_msm8625_qrd7())
		platform_device_register(&kp_pdev_qrd3);
	else if (machine_is_msm8625q_skud())
		platform_device_register(&kp_pdev_skud);
*/
	/* leds */


		//platform_device_register(&pmic_mpp_leds_pdev);
		//platform_device_register(&tricolor_leds_pdev);
	
#ifdef CONFIG_LEDS_TRICOLOR_FLAHSLIGHT
	    /*tricolor leds init*/
	if (machine_is_msm7627a_evb() || machine_is_msm8625_evb()
            || machine_is_msm8625_qrd5() || machine_is_msm7x27a_qrd5a()) {
		platform_device_register(&msm_device_tricolor_leds);
		tricolor_leds_gpio_setup();
	}
#endif  
	

