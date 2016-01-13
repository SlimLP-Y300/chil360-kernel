/* ltr558.c
 * LTR-On LTR-558 Proxmity and Light sensor driver
 *
 * Copyright (C) 2011 Lite-On Technology Corp (Singapore)
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
 /*
  *  2011-05-01 Lite-On created base driver.
  */
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/interrupt.h>
#include <linux/sysfs.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/err.h>
#include <linux/input.h>
#include <linux/jiffies.h>
#include <mach/gpio.h>
#include <linux/earlysuspend.h>
#include <linux/mutex.h>
#include <linux/ctype.h>
#include <linux/pm_runtime.h>
#include <linux/device.h>
#include <linux/input/ltr558.h>
#include <linux/irq.h>

#define LTR558_DRV_NAME		"ltr558"
#define LTR558_MANUFAC_ID	0x05

#define VENDOR_NAME		"lite-on"
#define SENSOR_NAME		"ltr558als"
#define DRIVER_VERSION		"1.0"

#define STATE_CHANGED_ALS_ON	(1 << 0)
#define STATE_CHANGED_ALS_OFF	(1 << 1)
#define STATE_CHANGED_PS_ON	(1 << 2)
#define STATE_CHANGED_PS_OFF	(1 << 3)
#define STATE_CHANGED_ERROR	(1 << 4)

static int ps_detected_threshold = PS_DETECTED_THRES;
static int ps_undetected_threshold = PS_UNDETECTED_THRES;

static int ltr558_device_init(void);

struct ltr558_data {

	struct i2c_client *client;
	struct input_dev *input;

	/* interrupt type is level-style */
	struct mutex lockw;
	struct delayed_work workw;
	struct delayed_work works;
	struct kobject *debugfs;

	u8 ps_open_state;
	u8 als_open_state;
	u8 mag;

	u16 irq;
	u16 intr_gpio;

	u32 chg_state;
	u32 ps_state;
	u32 last_lux;
	u32 meas_cycle;
};

struct ltr558_reg {
	const char *name;
	u8 addr;
	u16 defval;
	u16 curval;
};

static  struct ltr558_reg reg_tbl[] = {
	{
		.name   = "ALS_CONTR",
		.addr   = 0x80,
		.defval = 0x00,
		.curval = 0x03,
	},
	{
		.name = "PS_CONTR",
		.addr = 0x81,
		.defval = 0x00,
		.curval = 0xB3,
	},
	{
		.name = "ALS_PS_STATUS",
		.addr = 0x8c,
		.defval = 0x00,
		.curval = 0x00,
	},
	{
		.name = "INTERRUPT",
		.addr = 0x8f,
		.defval = 0x08,
		.curval = 0x03,
	},
	{
		.name = "PS_LED",
		.addr = 0x82,
		.defval = 0x6b,
		.curval = 0x6b,
	},
	{
		.name = "PS_N_PULSES",
		.addr = 0x83,
		.defval = 0x08,
		.curval = 0x08,
	},
	{
		.name = "PS_MEAS_RATE",
		.addr = 0x84,
		.defval = 0x02,
		.curval = 0x02,
	},
	{
		.name = "ALS_MEAS_RATE",
		.addr = 0x85,
		.defval = 0x03,
		.curval = 0x03,
	},
	{
		.name = "MANUFACTURER_ID",
		.addr = 0x87,
		.defval = 0x05,
		.curval = 0x05,
	},
	{
		.name = "INTERRUPT_PERSIST",
		.addr = 0x9e,
		.defval = 0x00,
		.curval = 0x23,
	},
	{
		.name = "PS_THRES_LOW",
		.addr = 0x92,
		.defval = 0x0000,
		.curval = 0x0000,
	},
	{
		.name = "PS_THRES_UP",
		.addr = 0x90,
		.defval = 0x07ff,
		.curval = 0x0000,
	},
	{
		.name = "ALS_THRES_LOW",
		.addr = 0x99,
		.defval = 0x0000,
		.curval = 0x0001,
	},
	{
		.name = "ALS_THRES_UP",
		.addr = 0x97,
		.defval = 0xffff,
		.curval = 0x0000,
	},
	{
		.name = "ALS_DATA_CH0",
		.addr = 0x8a,
		.defval = 0x0000,
		.curval = 0x0000,
	},
	{
		.name = "ALS_DATA_CH1",
		.addr = 0x88,
		.defval = 0x0000,
		.curval = 0x0000,
	},
	{
		.name = "PS_DATA",
		.addr = 0x8d,
		.defval = 0x0000,
		.curval = 0x0000,
	},
};


static struct ltr558_data *g_ltr558_data = NULL;

static int ltr558_i2c_read_reg(u8 regnum)
{
	return i2c_smbus_read_byte_data(g_ltr558_data->client, regnum);
}

static int ltr558_i2c_write_reg(u8 regnum, u8 value)
{
	int writeerror;

	writeerror =
	    i2c_smbus_write_byte_data(g_ltr558_data->client, regnum, value);
	if (writeerror < 0)
		return writeerror;
	else
		return 0;
}

static void ltr558_set_ps_threshold(u8 addr, u16 value)
{
	ltr558_i2c_write_reg(addr, (value & 0xff));
	ltr558_i2c_write_reg(addr + 1, (value >> 8));
}

/*
 *  Proximity Sensor Configure
 */
static int ltr558_ps_enable(int on)
{
	int ret;

	if (1 == on) {
		ret = ltr558_i2c_write_reg(LTR558_PS_CONTR, reg_tbl[1].curval);
		ret |= ltr558_i2c_write_reg(LTR558_INTERRUPT_PERSIST, 0x03);
		ret |= ltr558_i2c_write_reg(LTR558_PS_THRES_UP_0, 0x00);
		ret |= ltr558_i2c_write_reg(LTR558_PS_THRES_UP_1, 0x00);
		g_ltr558_data->ps_state = 2;
		mdelay(WAKEUP_DELAY);
	} else {
		ret = ltr558_i2c_write_reg(LTR558_PS_CONTR, MODE_PS_StdBy);
	}
	return ret;
}

static int ltr558_ps_read(void)
{
	int psval_lo, psval_hi, psdata;

	psval_lo = ltr558_i2c_read_reg(LTR558_PS_DATA_0);
	if (psval_lo < 0) {
		psdata = psval_lo;
		goto out;
	}

	psval_hi = ltr558_i2c_read_reg(LTR558_PS_DATA_1);
	if (psval_hi < 0) {
		psdata = psval_hi;
		goto out;
	}

	psdata = ((psval_hi & 7) << 8) + psval_lo;
 out:
	return psdata;
}

/*
 * Absent Light Sensor Congfig
 */
static int ltr558_als_enable(int on)
{
	int error;

	if (0 == on) {
		error = ltr558_i2c_write_reg(LTR558_ALS_CONTR, MODE_ALS_StdBy);

	} else {
		error = ltr558_i2c_write_reg(LTR558_ALS_CONTR, reg_tbl[0].curval);
		mdelay(WAKEUP_DELAY);
		error |= ltr558_i2c_read_reg(LTR558_ALS_DATA_CH0_1);
	}

	return error;
}

static int ltr558_als_start(void)
{
	if (g_ltr558_data->als_open_state == 1)
		return 0;

	g_ltr558_data->als_open_state = 1;
	g_ltr558_data->chg_state |= STATE_CHANGED_ALS_ON;
	schedule_delayed_work(&g_ltr558_data->works, 0);
	return 0;
}

static int ltr558_als_stop(void)
{
	if (g_ltr558_data->als_open_state == 0)
		return 0;
	g_ltr558_data->als_open_state = 0;
	g_ltr558_data->chg_state |= STATE_CHANGED_ALS_OFF;
	schedule_delayed_work(&g_ltr558_data->works, 0);
	return 0;
}

static int ltr558_ps_start(void)
{
	if (g_ltr558_data->ps_open_state == 1)
		return 0;
	g_ltr558_data->ps_open_state = 1;
	g_ltr558_data->chg_state |= STATE_CHANGED_PS_ON;
	schedule_delayed_work(&g_ltr558_data->works, 0);
	return 0;
}

static int ltr558_ps_stop(void)
{
	if (g_ltr558_data->ps_open_state == 0)
		return 0;
	g_ltr558_data->ps_open_state = 0;
	g_ltr558_data->chg_state |= STATE_CHANGED_PS_OFF;
	schedule_delayed_work(&g_ltr558_data->works, 0);
	return 0;
}

static int ltr558_als_read(void)
{
	int alsval_ch0_lo, alsval_ch0_hi, alsval_ch0;
	int alsval_ch1_lo, alsval_ch1_hi, alsval_ch1;
	int luxdata;
	int ch1_co, ch0_co, ratio;

	alsval_ch1_lo = ltr558_i2c_read_reg(LTR558_ALS_DATA_CH1_0);
	alsval_ch1_hi = ltr558_i2c_read_reg(LTR558_ALS_DATA_CH1_1);
	if (alsval_ch1_lo < 0 || alsval_ch1_hi < 0)
		return -1;
	alsval_ch1 = (alsval_ch1_hi << 8) + alsval_ch1_lo;

	alsval_ch0_lo = ltr558_i2c_read_reg(LTR558_ALS_DATA_CH0_0);
	alsval_ch0_hi = ltr558_i2c_read_reg(LTR558_ALS_DATA_CH0_1);
	if (alsval_ch0_lo < 0 || alsval_ch0_hi < 0)
		return -1;
	alsval_ch0 = (alsval_ch0_hi << 8) + alsval_ch0_lo;

	if ((alsval_ch0 | alsval_ch1) == 0)
		return -1;

	ratio = alsval_ch1 * 1000 / (alsval_ch1 + alsval_ch0);
	if (ratio < 450) {
		ch0_co = 17743;
		ch1_co = -11059;
	} else if ((ratio >= 450) && (ratio < 640)) {
		ch0_co = 37725;
		ch1_co = 13363;
	} else if ((ratio >= 640) && (ratio < 850)) {
		ch0_co = 16900;
		ch1_co = 1690;
	} else if (ratio >= 850) {
		ch0_co = 0;
		ch1_co = 0;
	}
	luxdata = (alsval_ch0 * ch0_co - alsval_ch1 * ch1_co) / 10000;

	return luxdata;
}

static void ltr558_state_work(struct work_struct *work)
{
	int res;
	mutex_lock(&g_ltr558_data->lockw);

	if (g_ltr558_data->chg_state & STATE_CHANGED_ALS_ON) {
		res = ltr558_als_enable(1);
		if (res)
			goto error_exit;
		else
			g_ltr558_data->chg_state &= ~STATE_CHANGED_ALS_ON;
	}

	if (g_ltr558_data->chg_state & STATE_CHANGED_ALS_OFF) {
		res = ltr558_als_enable(0);
		if (res)
			goto error_exit;
		else
			g_ltr558_data->chg_state &= ~STATE_CHANGED_ALS_OFF;
	}

	if (g_ltr558_data->chg_state & STATE_CHANGED_PS_ON) {
		res = ltr558_ps_enable(1);
		if (res)
			goto error_exit;
		else
			g_ltr558_data->chg_state &= ~STATE_CHANGED_PS_ON;
	}

	if (g_ltr558_data->chg_state & STATE_CHANGED_PS_OFF) {
		res = ltr558_ps_enable(0);
		if (res)
			goto error_exit;
		else
			g_ltr558_data->chg_state &= ~STATE_CHANGED_PS_OFF;
	}

	mutex_unlock(&g_ltr558_data->lockw);
	return;

error_exit:
	schedule_delayed_work(&g_ltr558_data->works, msecs_to_jiffies(500));
	mutex_unlock(&g_ltr558_data->lockw);
}
int data_ltr558;
static void ltr558_work_func(struct work_struct *work)
{
	int als_ps_status;
	
	int tmp_data;
	int ps_changed = 1;

	mutex_lock(&g_ltr558_data->lockw);

	als_ps_status = ltr558_i2c_read_reg(LTR558_ALS_PS_STATUS);
	if (als_ps_status < 0)
		goto workout;
	/* Here should check data status,ignore interrupt status. */
	/* Bit 0: PS Data
	 * Bit 1: PS interrupt
	 * Bit 2: ASL Data
	 * Bit 3: ASL interrupt
	 * Bit 4: ASL Gain 0: ALS measurement data is in dynamic range 2 (2 to 64k lux)
	 *                 1: ALS measurement data is in dynamic range 1 (0.01 to 320 lux)
	 */
	if ((g_ltr558_data->ps_open_state == 1) && (als_ps_status & 0x02)) {
		tmp_data = ltr558_ps_read();
		if (g_ltr558_data->ps_state == 2)
			ltr558_i2c_write_reg(LTR558_INTERRUPT_PERSIST, 0x13);

		if ((tmp_data >= ps_detected_threshold) && (g_ltr558_data->ps_state < 4)) {
			tmp_data = g_ltr558_data->ps_state;
			g_ltr558_data->ps_state = 1;
			g_ltr558_data->mag ^= 0x06;
			g_ltr558_data->ps_state = g_ltr558_data->mag & 0x0c;
			ltr558_set_ps_threshold(LTR558_PS_THRES_LOW_0, ps_undetected_threshold);
			ltr558_set_ps_threshold(LTR558_PS_THRES_UP_0, 0x07ff);
		} else if (((tmp_data <= ps_undetected_threshold) && (g_ltr558_data->ps_state > 2)) ||
				(g_ltr558_data->ps_state == 2)) {
			ltr558_set_ps_threshold(LTR558_PS_THRES_LOW_0, 0);
			ltr558_set_ps_threshold(LTR558_PS_THRES_UP_0, ps_detected_threshold);
			g_ltr558_data->mag ^= 0x01;
			g_ltr558_data->ps_state = g_ltr558_data->mag & 0x01;
		} else {
			ps_changed = 0;
		}
#if 1 //change by romandevel for jsr i6/i3
	if ((((g_ltr558_data->ps_state < 2) && (g_ltr558_data->ps_state > 0)) || (g_ltr558_data->ps_state == 0)) || (g_ltr558_data->ps_state == 2)) {
	  data_ltr558 = 5; 
	}
	
	if (g_ltr558_data->ps_state > 3 ) {
	  data_ltr558 = 0; 
	}
		if (ps_changed) {
			input_report_abs(g_ltr558_data->input,
					ABS_DISTANCE,
					data_ltr558);
			input_sync(g_ltr558_data->input);
		}
	}
#endif
	if ((g_ltr558_data->als_open_state == 1) && (als_ps_status & 0x04)) {
		tmp_data = ltr558_als_read();
		if (tmp_data > 10000)
			tmp_data = 10000;
		if ((tmp_data >= 0) && (tmp_data != g_ltr558_data->last_lux)) {
			g_ltr558_data->last_lux = tmp_data;
			input_report_abs(g_ltr558_data->input,
					ABS_MISC,
					tmp_data);
			input_sync(g_ltr558_data->input);
		}
	}
 workout:
	enable_irq(g_ltr558_data->irq);
	mutex_unlock(&g_ltr558_data->lockw);
}

static irqreturn_t ltr558_irq_handler(int irq, void *arg)
{
	struct ltr558_data *ltr558_data = (struct ltr558_data *)arg;

	if (NULL == ltr558_data)
		return IRQ_HANDLED;
	disable_irq_nosync(g_ltr558_data->irq);
	schedule_delayed_work(&g_ltr558_data->workw, 0);
	return IRQ_HANDLED;
}

static int ltr558_gpio_irq(void)
{
	int ret;

	ret = gpio_request(g_ltr558_data->intr_gpio, LTR558_DRV_NAME);
	if (ret) {
		printk(KERN_ALERT "%s: LTR-558ALS request gpio failed.\n",
		       __func__);
		return ret;
	}
	gpio_tlmm_config(GPIO_CFG(g_ltr558_data->intr_gpio, 0, GPIO_CFG_INPUT,
				GPIO_CFG_PULL_UP, GPIO_CFG_8MA),
			GPIO_CFG_ENABLE);
	ret = request_irq(g_ltr558_data->irq, ltr558_irq_handler,
			IRQ_TYPE_LEVEL_LOW, LTR558_DRV_NAME, g_ltr558_data);
	if (ret) {
		printk(KERN_ALERT "%s: LTR-558ALS request irq failed.\n",
		       __func__);
		gpio_free(g_ltr558_data->intr_gpio);
		return ret;
	}
	return 0;
}

static void ltr558_gpio_irq_free(void)
{
	free_irq(g_ltr558_data->irq, g_ltr558_data);
	gpio_free(g_ltr558_data->intr_gpio);
}

/*
 * Sys File system support
 */

static ssize_t ltr558_show_enable_ps(struct device *dev,
				     struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%u\n", g_ltr558_data->ps_open_state);
}

static ssize_t ltr558_store_enable_ps(struct device *dev,
				      struct device_attribute *attr,
				      const char *buf, size_t size)
{
	/* If proximity work,then ALS must be enable */
	unsigned long val;
	char *after;
	ssize_t count;

	val = simple_strtoul(buf, &after, 10);
	count = after - buf;
	if (*after && isspace(*after))
		count++;
	if (count != size)
		return -EINVAL;

	mutex_lock(&g_ltr558_data->lockw);
	if (1 == val) {
		ltr558_ps_start();
	} else if (0 == val) {
		ltr558_ps_stop();
	} else {
		count = -EINVAL;
	}
	mutex_unlock(&g_ltr558_data->lockw);
	return count;
}

static ssize_t ltr558_show_enable_als(struct device *dev,
				      struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%u\n", g_ltr558_data->als_open_state);
}

static ssize_t ltr558_store_enable_als(struct device *dev,
				       struct device_attribute *attr,
				       const char *buf, size_t size)
{
	/* If proximity work,then ALS must be enable */
	unsigned long val;
	char *after;
	ssize_t count;

	val = simple_strtoul(buf, &after, 10);
	count = after - buf;
	if (*after && isspace(*after))
		count++;
	if (count != size)
		return -EINVAL;

	mutex_lock(&g_ltr558_data->lockw);
	if (1 == val) {
		ltr558_als_start();
	} else if (0 == val) {
		ltr558_als_stop();
	} else {
		count = -EINVAL;
	}
	mutex_unlock(&g_ltr558_data->lockw);
	return count;
}

static ssize_t ltr558_driver_info_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "Chip: %s %s\nVersion: %s\n",
			VENDOR_NAME, SENSOR_NAME, DRIVER_VERSION);
}

static ssize_t ltr558_show_debug_regs(struct device *dev,
				      struct device_attribute *attr, char *buf)
{
	u8 val,high,low;
	int i;
	char *after;

	after = buf;

	after += sprintf(after, "%-17s%5s%14s%16s\n", "Register Name", "address", "default", "current");
	for (i = 0; i < sizeof(reg_tbl)/sizeof(reg_tbl[0]); i++) {
		if (reg_tbl[i].name == NULL || reg_tbl[i].addr == 0) {
			break;
		}
		if (i < 10) {
			val = ltr558_i2c_read_reg(reg_tbl[i].addr);
			after += sprintf(after, "%-20s0x%02x\t  0x%02x\t\t  0x%02x\n", reg_tbl[i].name, reg_tbl[i].addr, reg_tbl[i].defval, val);
		} else {
			low = ltr558_i2c_read_reg(reg_tbl[i].addr);
			high = ltr558_i2c_read_reg(reg_tbl[i].addr+1);
			after += sprintf(after, "%-20s0x%02x\t0x%04x\t\t0x%04x\n", reg_tbl[i].name, reg_tbl[i].addr, reg_tbl[i].defval,(high << 8) + low);
		}
	}
	after += sprintf(after, "\nYou can echo '0xaa=0xbb' to set the value 0xbb to the register of address 0xaa.\n ");

	return (after - buf);
}

static ssize_t ltr558_store_debug_regs(struct device *dev,
				       struct device_attribute *attr,
				       const char *buf, size_t size)
{
	/* If proximity work,then ALS must be enable */
	char *after, direct;
	u8 addr, val;

	addr = simple_strtoul(buf, &after, 16);
	direct = *after;
	val = simple_strtoul((after+1), &after, 16);

	if (!((addr >= 0x80 && addr <= 0x93)
	    	|| (addr >= 0x97 && addr <= 0x9e)))
		return -EINVAL;

	mutex_lock(&g_ltr558_data->lockw);
	if (direct == '=')
		ltr558_i2c_write_reg(addr, val);
	else
		printk("%s: register(0x%02x) is: 0x%02x\n", __func__, addr, ltr558_i2c_read_reg(addr));
	mutex_unlock(&g_ltr558_data->lockw);

	return (after - buf);
}

static ssize_t ltr558_show_adc_data(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	u8 high,low;
	char *after;
	after = buf;

	low = ltr558_i2c_read_reg(LTR558_PS_DATA_0);
	high = ltr558_i2c_read_reg(LTR558_PS_DATA_1);
	if (low < 0 || high < 0)
		after += sprintf(after, "Failed to read PS adc data.\n");
	else
		after += sprintf(after, "%d\n", (high << 8) + low);

	return (after - buf);
}

static DEVICE_ATTR(debug_regs, S_IRUGO | S_IWUGO, ltr558_show_debug_regs,
		   ltr558_store_debug_regs);
static DEVICE_ATTR(enable_als, S_IRUGO | S_IWUGO, ltr558_show_enable_als,
		   ltr558_store_enable_als);
static DEVICE_ATTR(enable_ps, S_IRUGO | S_IWUGO, ltr558_show_enable_ps,
		   ltr558_store_enable_ps);
static DEVICE_ATTR(info, S_IRUGO, ltr558_driver_info_show, NULL);
static DEVICE_ATTR(raw_adc, S_IRUGO, ltr558_show_adc_data, NULL);

static struct attribute *ltr558_attributes[] = {
	&dev_attr_enable_ps.attr,
	&dev_attr_info.attr,
	&dev_attr_enable_als.attr,
	&dev_attr_debug_regs.attr,
	&dev_attr_raw_adc.attr,
	NULL,
};

static const struct attribute_group ltr558_attr_group = {
	.attrs = ltr558_attributes,
};

static int ltr558_suspend(struct device *dev)
{
	int ret;

	mutex_lock(&g_ltr558_data->lockw);
	ret = ltr558_ps_enable(0);
	ret = ltr558_als_enable(0);
	mutex_unlock(&g_ltr558_data->lockw);
	return 0;
}

static int ltr558_resume(struct device *dev)
{
	mutex_lock(&g_ltr558_data->lockw);
	if (g_ltr558_data->als_open_state == 1)
		g_ltr558_data->chg_state |= STATE_CHANGED_ALS_ON;
	if (g_ltr558_data->ps_open_state == 1)
		g_ltr558_data->chg_state |= STATE_CHANGED_PS_ON;
	mutex_unlock(&g_ltr558_data->lockw);
	schedule_delayed_work(&g_ltr558_data->works, 0);
	return 0;
}

int ltr558_device_init(void)
{
	int retval = 0;
	int i;

	mdelay(PON_DELAY);
	ltr558_i2c_write_reg(LTR558_ALS_CONTR, 0x04);
	mdelay(WAKEUP_DELAY);
	for (i = 2; i < sizeof(reg_tbl)/sizeof(reg_tbl[0]); i++) {
		if (reg_tbl[i].name == NULL || reg_tbl[i].addr == 0) {
			break;
		}
		if (reg_tbl[i].defval != reg_tbl[i].curval) {
			if (i < 10) {
				retval = ltr558_i2c_write_reg(reg_tbl[i].addr, reg_tbl[i].curval);
			} else {
				retval = ltr558_i2c_write_reg(reg_tbl[i].addr, reg_tbl[i].curval & 0xff);
				retval = ltr558_i2c_write_reg(reg_tbl[i].addr + 1, reg_tbl[i].curval >> 8);
			}
		}
	}
	return retval;
}

int ltr558_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	int ret = 0;
	struct ltr558_platform_data *pdata;
	struct i2c_adapter *adapter = to_i2c_adapter(client->dev.parent);

	/* Return 1 if adapter supports everything we need, 0 if not. */
	if (!i2c_check_functionality
	    (adapter,
	     I2C_FUNC_SMBUS_WRITE_BYTE | I2C_FUNC_SMBUS_READ_BYTE_DATA)) {
		printk(KERN_ALERT
		       "%s: LTR-558ALS functionality check failed.\n",
		       __func__);
		ret = -EIO;
		goto out;
	}

	/* data memory allocation */
	g_ltr558_data = kzalloc(sizeof(struct ltr558_data), GFP_KERNEL);
	if (NULL == g_ltr558_data) {
		printk(KERN_ALERT "%s: LTR-558ALS kzalloc failed.\n", __func__);
		ret = -ENOMEM;
		goto out;
	}
	g_ltr558_data->client = client;
	pdata = client->dev.platform_data;
	g_ltr558_data->intr_gpio = pdata->intr;
	g_ltr558_data->irq = client->irq;
	g_ltr558_data->meas_cycle = 500;	//50 msecs.
	g_ltr558_data->mag = 0x08;
	printk("%s: int gpio is %d, irq is %d", __func__, g_ltr558_data->intr_gpio, g_ltr558_data->irq);
	i2c_set_clientdata(client, g_ltr558_data);

	ret = ltr558_device_init();
	if (ret) {
		printk(KERN_ALERT "%s: LTR-558ALS device init failed.\n",
		       __func__);
		goto out;
	}

	ret = ltr558_i2c_read_reg(LTR558_MANUFACTURER_ID);
	if (ret != LTR558_MANUFAC_ID) {
		printk(KERN_ALERT
			"%s: LTR-558ALS the manufacture id is not match.\n",
			__func__);
		ret = -EINVAL;
		goto out;
	}

	ret = ltr558_gpio_irq();
	if (ret) {
		printk(KERN_ALERT "%s: LTR-558ALS gpio_irq failed.\n",
		       __func__);
		goto out;
	}

	g_ltr558_data->input = input_allocate_device();
	if (g_ltr558_data->input == NULL)
		goto free_irq;

	g_ltr558_data->input->name = LTR558_DRV_NAME;
	g_ltr558_data->input->id.bustype = BUS_I2C;
	input_set_drvdata(g_ltr558_data->input, g_ltr558_data);
	g_ltr558_data->input->evbit[0] = BIT_MASK(EV_ABS);
	g_ltr558_data->input->absbit[0] = BIT_MASK(ABS_DISTANCE);
	//g_ltr558_data->input->evbit[1] = BIT_MASK(EV_ABS);
	g_ltr558_data->input->absbit[1] = BIT_MASK(ABS_MISC);
	input_set_abs_params(g_ltr558_data->input, ABS_DISTANCE, 0, 2047, 0, 0);
	input_set_abs_params(g_ltr558_data->input, ABS_MISC, 0, 10000, 0, 0);

	if (input_register_device(g_ltr558_data->input))
		goto free_irq;

	INIT_DELAYED_WORK(&g_ltr558_data->workw, ltr558_work_func);
	INIT_DELAYED_WORK(&g_ltr558_data->works, ltr558_state_work);
	mutex_init(&g_ltr558_data->lockw);

	ret = sysfs_create_group(&g_ltr558_data->input->dev.kobj,
			&ltr558_attr_group);
	if (ret)
		goto exit_unregister_dev;
	printk("%s: succeed\n", __func__);

	return 0;

exit_unregister_dev:
	input_unregister_device(g_ltr558_data->input);
free_irq:
	ltr558_gpio_irq_free();
	if (g_ltr558_data->input)
		input_free_device(g_ltr558_data->input);
	kfree(g_ltr558_data);
out:
	return ret;
}

static int ltr558_remove(struct i2c_client *client)
{
	if (g_ltr558_data == NULL) {
		return 0;
	}

	ltr558_ps_enable(0);
	ltr558_als_enable(0);
	free_irq(g_ltr558_data->irq, g_ltr558_data);
	gpio_free(g_ltr558_data->intr_gpio);
	sysfs_remove_group(&g_ltr558_data->input->dev.kobj,
			&ltr558_attr_group);
	cancel_delayed_work_sync(&g_ltr558_data->workw);
	cancel_delayed_work_sync(&g_ltr558_data->works);
	input_unregister_device(g_ltr558_data->input);
	input_free_device(g_ltr558_data->input);
	kfree(g_ltr558_data);
	g_ltr558_data = NULL;

	return 0;
}

static void __devexit ltr558_shutdown(struct i2c_client *client)
{
	ltr558_remove(client);
}

static struct i2c_device_id ltr558_id[] = {
	{"ltr558", 0},
	{}
};

MODULE_DEVICE_TABLE(i2c, ltr558_id);
static SIMPLE_DEV_PM_OPS(ltr558_pm_ops, ltr558_suspend, ltr558_resume);
static struct i2c_driver ltr558_driver = {
	.driver = {
		   .name = LTR558_DRV_NAME,
		   .owner = THIS_MODULE,
		   .pm = &ltr558_pm_ops,
	},
	.probe = ltr558_probe,
	.remove = ltr558_remove,
	.shutdown = ltr558_shutdown,
	.id_table = ltr558_id,
};

static int ltr558_driver_init(void)
{
	pr_devel("Driver ltr5580 init.\n");
	return i2c_add_driver(&ltr558_driver);
};

static void ltr558_driver_exit(void)
{
	pr_info("Unload ltr558 module...\n");
	i2c_del_driver(&ltr558_driver);
}

module_init(ltr558_driver_init);
module_exit(ltr558_driver_exit);
MODULE_AUTHOR("Lite-On Technology Corp.");
MODULE_DESCRIPTION("Lite-On LTR-558 Proximity and Light Sensor Driver");
MODULE_LICENSE("GPL v2");
MODULE_VERSION("1.0");