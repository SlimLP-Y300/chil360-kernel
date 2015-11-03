/* drivers/input/touchscreen/ektf2k.c - ELAN EKTF2K verions of driver
 *
 * Copyright (C) 2011 Elan Microelectronics Corporation.
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
 * 2011/12/06: The first release, version 0x0001
 * 2012/2/15:  The second release, version 0x0002 for new bootcode
 * 2012/5/8:   Release version 0x0003 for china market
 *             Integrated 2 and 5 fingers driver code together and
 *             auto-mapping resolution.
 */

#include <linux/module.h>
#include <linux/input.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#ifdef CONFIG_HAS_EARLYSUSPEND
#include <linux/earlysuspend.h>
#endif
#include <linux/platform_device.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/device.h>
#include <linux/jiffies.h>
#include <linux/miscdevice.h>
#include <linux/regulator/consumer.h>
#include <mach/socinfo.h>

// for linux 2.6.36.3
#include <linux/cdev.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <asm/ioctl.h>
#include <linux/input/ektf2k.h> 

#if 0
#define DBG(x...)	pr_info(x)
#else
#define DBG(x...)
#endif

#define EKT_TS_MT_RESET_GPIO  26

#define IAP_I2C_SPEED 100*1000

#define PACKET_SIZE				22
#define PWR_STATE_DEEP_SLEEP	0
#define PWR_STATE_NORMAL		1
#define PWR_STATE_MASK			BIT(3)

#define CMD_S_PKT			0x52
#define CMD_R_PKT			0x53
#define CMD_W_PKT			0x54

#define HELLO_PKT			0x55
#define NORMAL_PKT			0x5D

#define TWO_FINGERS_PKT      0x5A
#define FIVE_FINGERS_PKT       0x6D
#define TEN_FINGERS_PKT	0x62

#define RESET_PKT			0x77
#define CALIB_PKT			0xA8

// modify
#define SYSTEM_RESET_PIN_SR 	10
 
//Add these Define
#define IAP_IN_DRIVER_MODE 	1
#define IAP_PORTION        	1
#define PAGERETRY  30
#define IAPRESTART 5


// For Firmware Update 
#define ELAN_IOCTLID	0xD0
#define CUSTOMER_IOCTLID	0xA0

#define IOCTL_I2C_SLAVE	_IOW(ELAN_IOCTLID,  1, int)
#define IOCTL_MAJOR_FW_VER  _IOR(ELAN_IOCTLID, 2, int)
#define IOCTL_MINOR_FW_VER  _IOR(ELAN_IOCTLID, 3, int)
#define IOCTL_RESET  _IOR(ELAN_IOCTLID, 4, int)
#define IOCTL_IAP_MODE_LOCK  _IOR(ELAN_IOCTLID, 5, int)
#define IOCTL_CHECK_RECOVERY_MODE  _IOR(ELAN_IOCTLID, 6, int)
#define IOCTL_FW_VER  _IOR(ELAN_IOCTLID, 7, int)
#define IOCTL_X_RESOLUTION  _IOR(ELAN_IOCTLID, 8, int)
#define IOCTL_Y_RESOLUTION  _IOR(ELAN_IOCTLID, 9, int)
#define IOCTL_FW_ID  _IOR(ELAN_IOCTLID, 10, int)
#define IOCTL_ROUGH_CALIBRATE  _IOR(ELAN_IOCTLID, 11, int)
#define IOCTL_IAP_MODE_UNLOCK  _IOR(ELAN_IOCTLID, 12, int)
#define IOCTL_I2C_INT  _IOR(ELAN_IOCTLID, 13, int)
#define IOCTL_RESUME  _IOR(ELAN_IOCTLID, 14, int)
#define IOCTL_POWER_LOCK  _IOR(ELAN_IOCTLID, 15, int)
#define IOCTL_POWER_UNLOCK  _IOR(ELAN_IOCTLID, 16, int)

#define IOCTL_FW_UPDATE  _IOR(ELAN_IOCTLID, 17, int)
#define IOCTL_GET_UPDATE_PROGREE	_IOR(CUSTOMER_IOCTLID,  2, int)
#define IOCTL_CIRCUIT_CHECK  _IOR(CUSTOMER_IOCTLID, 1, int)

uint8_t RECOVERY=0x00;
int FW_VERSION=0x00;
int X_RESOLUTION=0x00;  
int Y_RESOLUTION=0x00;
int FW_ID=0x00;
int work_lock=0x00;
int power_lock=0x00;
int circuit_ver=0x01;
int file_fops_addr=0x15;
static int elan_Multitouch_gpio_initialized = 0;
static int press_down_in_a_area = 0;
static int y_limit = 0;
static int key_pressed = -1;


#define ELAN_KEY_BACK	0x81
#define ELAN_KEY_HOME	0x41
#define ELAN_KEY_MENU	0x21

struct osd_offset {
	int left_x;
	int right_x;
	unsigned int key_event;
};

//Elan add for OSD bar coordinate
static struct osd_offset OSD_mapping[] = {
	{10,  150, KEY_MENU},
	{250, 390, KEY_HOMEPAGE},
	{490, 630, KEY_BACK},
};


#if IAP_PORTION
uint8_t ic_status=0x00;	//0:OK 1:master fail 2:slave fail
int update_progree=0;
uint8_t I2C_DATA[3] = {0x15, 0x20, 0x21};/*I2C devices address*/  
int is_OldBootCode = 0; // 0:new 1:old


/*The newest firmware, if update must be changed here*/
static uint8_t file_fw_data[] = {
#include "elan_ktf2132_fw.inc"
};


enum {
	PageSize		= 132,
	PageNum		  = 249,
	ACK_Fail		= 0x00,
	ACK_OK			= 0xAA,
	ACK_REWRITE	= 0x55,
};

enum {
	E_FD			= -1,
};
#endif

struct elan_ktf2k_ts_data {
	struct i2c_client *client;
	struct input_dev *input_dev;
	struct workqueue_struct *elan_wq;
	struct work_struct work;
	struct early_suspend early_suspend;
	int (*power)(int on);
	int power_gpio;
	int reset_gpio;
	int intr_gpio;
// Firmware Information
	int fw_ver;
	int fw_id;
	int x_resolution;
	int y_resolution;
// For Firmare Update 
	struct miscdevice firmware;
};

static struct elan_ktf2k_ts_data *private_ts;
static int __fw_packet_handler(struct i2c_client *client);
static int elan_ktf2k_ts_rough_calibrate(struct i2c_client *client);
static int elan_ktf2k_ts_resume(struct i2c_client *client);
static void elan_ktf2k_ts_reset(void);

#if IAP_PORTION
int Update_FW_One(/*struct file *filp,*/ struct i2c_client *client, int recovery);
static int __hello_packet_handler(struct i2c_client *client);
#endif

// For Firmware Update 
int elan_iap_open(struct inode *inode, struct file *filp){ 
	printk("[ELAN]into elan_iap_open\n");
	if (private_ts == NULL)
		printk("private_ts is NULL~~~\n");
		
	return 0;
}

int elan_iap_release(struct inode *inode, struct file *filp){    
	return 0;
}

static ssize_t elan_iap_write(struct file *filp,
	const char *buff, size_t count, loff_t *offp)
{
	int ret;
	char *tmp;
	struct i2c_adapter *adap = private_ts->client->adapter;    	
	struct i2c_msg msg;

	DBG("[ELAN] %s \n", __func__);
	if (count > 8192)
		count = 8192;

	tmp = kmalloc(count, GFP_KERNEL);
	
	if (tmp == NULL)
		return -ENOMEM;

	if (copy_from_user(tmp, buff, count)) {
		kfree(tmp);
		return -EFAULT;
	}

#if 1
	msg.addr = file_fops_addr;
	msg.flags = 0x00;// 0x00
	msg.len = count;
	msg.buf = (char *)tmp;
	//msg.scl_rate = IAP_I2C_SPEED;
	ret = i2c_transfer(adap, &msg, 1);
#else
	ret = i2c_master_send(private_ts->client, tmp, count);
#endif	

	kfree(tmp);

	return (ret == 1) ? count : ret;
}

ssize_t elan_iap_read(struct file *filp, 
	char *buff, size_t count, loff_t *offp)
{    
	char *tmp;
	int ret;  
	long rc;
	struct i2c_adapter *adap = private_ts->client->adapter;
	struct i2c_msg msg;

	DBG("[ELAN] %s \n", __func__);
	if (count > 8192)
		count = 8192;

	tmp = kmalloc(count, GFP_KERNEL);

	if (tmp == NULL)
		return -ENOMEM;
#if 1
	msg.addr = file_fops_addr;
	//msg.flags |= I2C_M_RD;
	msg.flags = 0x00;
	msg.flags |= I2C_M_RD;
	msg.len = count;
	msg.buf = tmp;
	//msg.scl_rate = IAP_I2C_SPEED;
	ret = i2c_transfer(adap, &msg, 1);
#else
	ret = i2c_master_recv(private_ts->client, tmp, count);
#endif
	if (ret >= 0)
		rc = copy_to_user(buff, tmp, count);
	
	kfree(tmp);

	return (ret == 1) ? count : ret;
}

static long elan_iap_ioctl(/*struct inode *inode,*/ struct file *filp,
	unsigned int cmd, unsigned long arg)
{
	int __user *ip = (int __user *)arg;
	DBG("[ELAN]into elan_iap_ioctl\n");
	DBG("cmd value %x\n",cmd);
	
	switch (cmd) {        
		case IOCTL_I2C_SLAVE: 
			//private_ts->client->addr = (int __user)arg;
			file_fops_addr = 0x15;
			break;   
		case IOCTL_MAJOR_FW_VER:
			break;        
		case IOCTL_MINOR_FW_VER:            
			break;        
		case IOCTL_RESET:
      gpio_set_value(SYSTEM_RESET_PIN_SR, 0);
      msleep(20);
		  gpio_set_value(SYSTEM_RESET_PIN_SR, 1);
      msleep(100);
			break;
		case IOCTL_IAP_MODE_LOCK:
			if (work_lock==0) {
				work_lock=1;
				disable_irq(private_ts->client->irq);
				cancel_work_sync(&private_ts->work);
			}
			break;
		case IOCTL_IAP_MODE_UNLOCK:
			if (work_lock==1) {			
				work_lock=0;
				enable_irq(private_ts->client->irq);
			}
			break;
		case IOCTL_CHECK_RECOVERY_MODE:
			return RECOVERY;
			break;
		case IOCTL_FW_VER:
			__fw_packet_handler(private_ts->client);
			return FW_VERSION;
			break;
		case IOCTL_X_RESOLUTION:
			__fw_packet_handler(private_ts->client);
			return X_RESOLUTION;
			break;
		case IOCTL_Y_RESOLUTION:
			__fw_packet_handler(private_ts->client);
			return Y_RESOLUTION;
			break;
		case IOCTL_FW_ID:
			__fw_packet_handler(private_ts->client);
			return FW_ID;
			break;
		case IOCTL_ROUGH_CALIBRATE:
			return elan_ktf2k_ts_rough_calibrate(private_ts->client);
		case IOCTL_I2C_INT:
			put_user(gpio_get_value(private_ts->intr_gpio), ip);
			break;	
		case IOCTL_RESUME:
			elan_ktf2k_ts_resume(private_ts->client);
			break;	
		case IOCTL_CIRCUIT_CHECK:
			return circuit_ver;
			break;
		case IOCTL_POWER_LOCK:
			power_lock=1;
			break;
		case IOCTL_POWER_UNLOCK:
			power_lock=0;
			break;
#if IAP_PORTION		
		case IOCTL_GET_UPDATE_PROGREE:
			update_progree=(int __user)arg;
			break; 

		case IOCTL_FW_UPDATE:
			Update_FW_One(private_ts->client, 0);
#endif
		default:            
			break;   
	}       
	return 0;
}

struct file_operations elan_touch_fops = {    
        .open =         elan_iap_open,    
        .write =        elan_iap_write,    
        .read = 	elan_iap_read,    
        .release =	elan_iap_release,    
	.unlocked_ioctl=elan_iap_ioctl, 
 };
 
#if IAP_PORTION
int EnterISPMode(struct i2c_client *client, uint8_t  *isp_cmd)
{
	char buff[4] = {0};
	int len = 0;
	
	len = i2c_master_send(private_ts->client, isp_cmd,  sizeof(isp_cmd));
	if (len != sizeof(buff)) {
		printk("[ELAN] ERROR: EnterISPMode fail! len=%d\r\n", len);
		return -1;
	}
	else
		printk("[ELAN] IAPMode write data successfully! cmd = [%2x, %2x, %2x, %2x]\n", 
			isp_cmd[0], isp_cmd[1], isp_cmd[2], isp_cmd[3]);
	return 0;
}

int ExtractPage(struct file *filp, uint8_t * szPage, int byte)
{
	int len = 0;

	len = filp->f_op->read(filp, szPage,byte, &filp->f_pos);
	if (len != byte) {
		printk("[ELAN] ExtractPage ERROR: read page error, read error. len=%d\r\n", len);
		return -1;
	}

	return 0;
}

int WritePage(uint8_t * szPage, int byte)
{
	int len = 0;

	len = i2c_master_send(private_ts->client, szPage,  byte);
	if (len != byte) {
		printk("[ELAN] ERROR: write page error, write error. len=%d\r\n", len);
		return -1;
	}

	return 0;
}

int GetAckData(struct i2c_client *client)
{
	int len = 0;

	char buff[2] = {0};
	
	len = i2c_master_recv(private_ts->client, buff, sizeof(buff));
	if (len != sizeof(buff)) {
		pr_err("[ELAN] ERROR: read data error, write 50 times error. len=%d\n", len);
		return -1;
	}

	pr_info("[ELAN] GetAckData: %x,%x\n",buff[0],buff[1]);
	if (buff[0] == 0xAA /* && buff[1] == 0xaa*/) 
		return ACK_OK;
	else if (buff[0] == 0x55 && buff[1] == 0x55)
		return ACK_REWRITE;
	else
		return ACK_Fail;

	return 0;
}

void print_progress(int page, int ic_num, int j)
{
	int i, percent,page_tatol,percent_tatol;
	char str[256];
	str[0] = '\0';
	for (i=0; i<((page)/10); i++) {
		str[i] = '#';
		str[i+1] = '\0';
	}
	
	page_tatol=page+249*(ic_num-j);
	percent = ((100*page)/(249));
	percent_tatol = ((100*page_tatol)/(249*ic_num));

	if ((page) == (249))
		percent = 100;

	if ((page_tatol) == (249*ic_num))
		percent_tatol = 100;		

	printk("\rprogress %s| %d%%", str, percent);
	
	if (page == (249))
		printk("\n");
}
/* 
* Restet and (Send normal_command ?)
* Get Hello Packet
*/
int IAPReset(struct i2c_client *client)
{
	int res;
	int retry = 10;
	uint8_t buf[4] = {0};

	elan_ktf2k_ts_reset();

	DBG("[ELAN] %s: read Hello packet data!\n", __func__);
	res = __hello_packet_handler(client);
	msleep(40);
	if (res == 0x80) {
		while (retry) {
			retry--;
			if (retry <= 0) break;
			res = i2c_master_recv(private_ts->client, buf, 4);
			if (buf[0] == 0x55 && buf[1] == 0xAA && buf[2] == 0x33 && buf[3] == 0xCC) {
				return res;
			}
		}
	}
	pr_err("[ELAN] %s: iap read response error!\n", __func__);
	return res;
}


int Update_FW_One(struct i2c_client *client, int recovery)
{
	int res = 0,ic_num = 1;
	int iPage = 0, rewriteCnt = 0; //rewriteCnt for PAGE_REWRITE
	int i = 0;
	uint8_t data;
	//struct timeval tv1, tv2;
	int restartCnt = 0; // For IAP_RESTART
	
	uint8_t r_buf[4] = {0};
	int byte_count;
	uint8_t *szBuff = NULL;
	int curIndex = 0;
	uint8_t isp_cmd[] = {0x54, 0x00, 0x12, 0x34}; //{0x45, 0x49, 0x41, 0x50};

	pr_info("[ELAN] %s: ic_num=%d\n", __func__, ic_num);
	
IAP_RESTART:	
	data=I2C_DATA[0];//Master
	elan_ktf2k_ts_reset();
	DBG("[ELAN] %s: address data=0x%x \r\n", __func__, data);

	if (recovery != 0x80) {
    printk("[ELAN] Firmware upgrade normal mode !\n");
		res = EnterISPMode(private_ts->client, isp_cmd);	 //enter ISP mode
	} else {
    printk("[ELAN] Firmware upgrade recovery mode !\n");
		msleep(100);
		for (i=0; i < 5; i++) {
			msleep(100);
			res = i2c_master_recv(private_ts->client, r_buf, 4);   //55 aa 33 cc 
			DBG("[ELAN] recovery byte data:%x,%x,%x,%x \n",
				r_buf[0],r_buf[1],r_buf[2],r_buf[3]);
			if (r_buf[0] == 0x55 && r_buf[1] == 0xAA && r_buf[2] == 0x33 && r_buf[3] == 0xCC)
				break;
		}
	}
	// Send Dummy Byte	
	printk("[ELAN] send one byte data:%x,%x",private_ts->client->addr,data);
	res = i2c_master_send(private_ts->client, &data,  sizeof(data));
	if(res!=sizeof(data)) {
		printk("[ELAN] dummy error code = %d\n",res);
	}
	mdelay(10);

	// Start IAP
	for (iPage = 1; iPage <= PageNum; iPage++) {
PAGE_REWRITE:
		for (byte_count=1; byte_count<=17; byte_count++) {
			szBuff = file_fw_data + curIndex;
			if (byte_count != 17) {		
				curIndex += 8;
				res = WritePage(szBuff, 8);
			} else {
				curIndex += 4;
				res = WritePage(szBuff, 4); 
			}
		}
		if (iPage==249 || iPage==1) {
			mdelay(600); 			 
		} else {
			mdelay(50); 			 
		}
		res = GetAckData(private_ts->client);

		if (ACK_OK != res) {
			mdelay(50); 
			pr_err("[ELAN] ERROR: GetAckData fail! res=%d\r\n", res);
			if ( res == ACK_REWRITE ) {
				rewriteCnt = rewriteCnt + 1;
				if (rewriteCnt == PAGERETRY) {
					DBG("[ELAN] ID 0x%02x %dth page ReWrite %d times fails!\n"
						, data, iPage, PAGERETRY);
					return E_FD;
				} else {
					DBG("[ELAN] ---%d--- page ReWrite %d times!\n",
						iPage, rewriteCnt);
					goto PAGE_REWRITE;
				}
			} else {
				restartCnt = restartCnt + 1;
				if (restartCnt >= 5) {
					DBG("[ELAN] ID 0x%02x ReStart %d times fails!\n",
						data, IAPRESTART);
					return E_FD;
				} else {
					DBG("[ELAN] ===%d=== page ReStart %d times!\n",
						iPage, restartCnt);
					goto IAP_RESTART;
				}
			}
		} else {
			DBG("[ELAN]  data : 0x%02x ", data);
			rewriteCnt=0;
			//print_progress(iPage,ic_num,i);
			break;
		}

		mdelay(10);
	} // end of for(iPage = 1; iPage <= PageNum; iPage++)

	if (IAPReset(client)) {
		pr_err("[ELAN] Update Firmware Error!\n");
		return -22;
	}
	__fw_packet_handler(client);
	pr_info("[ELAN] Update Firmware successfully!\n");
	return 0;
}

#endif
// End Firmware Update

// Start sysfs
#ifdef CONFIG_HAS_EARLYSUSPEND
static void elan_ktf2k_ts_early_suspend(struct early_suspend *h);
static void elan_ktf2k_ts_late_resume(struct early_suspend *h);
#endif

static ssize_t elan_ktf2k_gpio_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	int ret = 0;
	struct elan_ktf2k_ts_data *ts = private_ts;

	ret = gpio_get_value(ts->intr_gpio);
	//printk(KERN_DEBUG "GPIO_TP_INT_N=%d\n", ts->intr_gpio);
	sprintf(buf, "GPIO_TP_INT_N=%d\n", ret);
	ret = strlen(buf) + 1;
	return ret;
}

static DEVICE_ATTR(gpio, S_IRUGO, elan_ktf2k_gpio_show, NULL);

static ssize_t elan_ktf2k_vendor_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	ssize_t ret = 0;
	struct elan_ktf2k_ts_data *ts = private_ts;

	sprintf(buf, "%s_x%4.4x\n", "ELAN_KTF2K", ts->fw_ver);
	ret = strlen(buf) + 1;
	return ret;
}

static DEVICE_ATTR(vendor, S_IRUGO, elan_ktf2k_vendor_show, NULL);

static struct kobject *android_touch_kobj;

static int elan_ktf2k_touch_sysfs_init(void)
{
	int ret ;

	android_touch_kobj = kobject_create_and_add("android_touch", NULL) ;
	if (android_touch_kobj == NULL) {
		printk(KERN_ERR "[elan]%s: subsystem_register failed\n", __func__);
		ret = -ENOMEM;
		return ret;
	}
	ret = sysfs_create_file(android_touch_kobj, &dev_attr_gpio.attr);
	if (ret) {
		printk(KERN_ERR "[elan]%s: sysfs_create_file failed\n", __func__);
		return ret;
	}
	ret = sysfs_create_file(android_touch_kobj, &dev_attr_vendor.attr);
	if (ret) {
		printk(KERN_ERR "[elan]%s: sysfs_create_group failed\n", __func__);
		return ret;
	}
	return 0 ;
}

static void elan_touch_sysfs_deinit(void)
{
	sysfs_remove_file(android_touch_kobj, &dev_attr_vendor.attr);
	sysfs_remove_file(android_touch_kobj, &dev_attr_gpio.attr);
	kobject_del(android_touch_kobj);
}	

// end sysfs

static int __elan_ktf2k_ts_poll(struct i2c_client *client)
{
	struct elan_ktf2k_ts_data *ts = i2c_get_clientdata(client);
	int status = 0, retry = 10;

	do {
		status = gpio_get_value(ts->intr_gpio);
		DBG("%s: status = %d\n", __func__, status);
		retry--;
		mdelay(20);
	} while (status == 1 && retry > 0);

	DBG("[elan]%s: poll interrupt status %s\n",
			__func__, status == 1 ? "high" : "low");
	return (status == 0 ? 0 : -ETIMEDOUT);
}

static int elan_ktf2k_ts_poll(struct i2c_client *client)
{
	return __elan_ktf2k_ts_poll(client);
}

static int elan_ktf2k_ts_get_data(struct i2c_client *client, uint8_t *cmd,
			uint8_t *buf, size_t size)
{
	int rc;

	dev_dbg(&client->dev, "[elan]%s: enter\n", __func__);

	if (buf == NULL)
		return -EINVAL;

	if ((i2c_master_send(client, cmd, 4)) != 4) {
		dev_err(&client->dev,
			"[elan]%s: i2c_master_send failed\n", __func__);
		return -EINVAL;
	}

	rc = elan_ktf2k_ts_poll(client);
	if (rc < 0)
		return -EINVAL;
	else {
		if (i2c_master_recv(client, buf, size) != size ||
		    buf[0] != CMD_S_PKT)
			return -EINVAL;
	}

	return 0;
}

static int __hello_packet_handler(struct i2c_client *client)
{
	int rc;
	uint8_t buf_recv[8] = { 0 };
	//uint8_t buf_recv1[4] = { 0 };

	//mdelay(500);
	rc = elan_ktf2k_ts_poll(client);
	if (rc < 0) {
		printk( "[elan] %s: Int poll failed!\n", __func__);
		RECOVERY=0x80;
		return RECOVERY;
		//return -EINVAL;
	}

	rc = i2c_master_recv(client, buf_recv, 8);
	printk("[elan]%s: hello packet %2x:%2X:%2x:%2x:%2x:%2X:%2x:%2x\n", 
		__func__, buf_recv[0], buf_recv[1], buf_recv[2], buf_recv[3] , 
		buf_recv[4], buf_recv[5], buf_recv[6], buf_recv[7]);

	if (buf_recv[0]==0x55 && buf_recv[1]==0x55 && buf_recv[2]==0x80 && buf_recv[3]==0x80) {
		RECOVERY=0x80;
		return RECOVERY; 
	}
	return 0;
}

static int __fw_packet_handler(struct i2c_client *client)
{
	struct elan_ktf2k_ts_data *ts = i2c_get_clientdata(client);
	int rc;
	int major, minor;
	uint8_t cmd[] = {CMD_R_PKT, 0x00, 0x00, 0x01};/*Get firmware ver*/
	uint8_t cmd_x[] = {0x53, 0x60, 0x00, 0x00}; /*Get x resolution*/
	uint8_t cmd_y[] = {0x53, 0x63, 0x00, 0x00}; /*Get y resolution*/
	uint8_t cmd_id[] = {0x53, 0xf0, 0x00, 0x01}; /*Get firmware ID*/
	//uint8_t cmd_bc[] = {0x53, 0x10, 0x00, 0x01}; /*Get firmware BC*/
	uint8_t buf_recv[4] = {0};

// Firmware version
	rc = elan_ktf2k_ts_get_data(client, cmd, buf_recv, 4);
	if (rc < 0)
		return rc;
	major = ((buf_recv[1] & 0x0f) << 4) | ((buf_recv[2] & 0xf0) >> 4);
	minor = ((buf_recv[2] & 0x0f) << 4) | ((buf_recv[3] & 0xf0) >> 4);
	ts->fw_ver = major << 8 | minor;
	FW_VERSION = ts->fw_ver;

// X Resolution
	rc = elan_ktf2k_ts_get_data(client, cmd_x, buf_recv, 4);
	if (rc < 0)
		return rc;
	minor = ((buf_recv[2])) | ((buf_recv[3] & 0xf0) << 4);
	ts->x_resolution =minor;
	X_RESOLUTION = ts->x_resolution;

// Y Resolution	
	rc = elan_ktf2k_ts_get_data(client, cmd_y, buf_recv, 4);
	if (rc < 0)
		return rc;
	minor = ((buf_recv[2])) | ((buf_recv[3] & 0xf0) << 4);
	ts->y_resolution =minor;
	Y_RESOLUTION = ts->y_resolution;

// Firmware ID
	rc = elan_ktf2k_ts_get_data(client, cmd_id, buf_recv, 4);
	if (rc < 0)
		return rc;
	major = ((buf_recv[1] & 0x0f) << 4) | ((buf_recv[2] & 0xf0) >> 4);
	minor = ((buf_recv[2] & 0x0f) << 4) | ((buf_recv[3] & 0xf0) >> 4);
	ts->fw_id = major << 8 | minor;
	FW_ID = ts->fw_id;

// Firmware BC
	//rc = elan_ktf2k_ts_get_data(client, cmd_bc, buf_recv, 4);
	//	if (rc < 0)
	//return rc;
	//major = ((buf_recv[1] & 0x0f) << 4) | ((buf_recv[2] & 0xf0) >> 4);
	//minor = ((buf_recv[2] & 0x0f) << 4) | ((buf_recv[3] & 0xf0) >> 4);
	//ts->fw_bc = major << 8 | minor;
	//FW_BC = ts->fw_bc;

	printk(KERN_INFO "[elan] %s: firmware version: 0x%4.4x\n",
			__func__, ts->fw_ver);
	printk(KERN_INFO "[elan] %s: firmware ID: 0x%4.4x\n",
			__func__, ts->fw_id);
	//printk(KERN_INFO "[elan] %s: firmware BC: 0x%4.4x\n",
	//		__func__, ts->fw_bc);
	printk(KERN_INFO "[elan] %s: x resolution: %d, y resolution: %d\n",
			__func__, ts->x_resolution, ts->y_resolution);
	
	return 0;
}

#if 0
static int __fw_packet_handler2(struct i2c_client *client)
{
	struct elan_ktf2k_ts_data *ts = i2c_get_clientdata(client);
	int rc;
	int major, minor;
	uint8_t cmd[] = {CMD_R_PKT, 0x00, 0x00, 0x01};/* Get Firmware Version*/
	uint8_t cmd_x[] = {0x53, 0x60, 0x00, 0x00}; /*Get x resolution*/
	uint8_t cmd_y[] = {0x53, 0x63, 0x00, 0x00}; /*Get y resolution*/
	uint8_t cmd_id[] = {0x53, 0xf0, 0x00, 0x01}; /*Get firmware ID*/
	//uint8_t cmd_bc[] = {CMD_R_PKT, 0x01, 0x00, 0x01};/* Get BootCode Version*/
	uint8_t buf_recv[4] = {0};
// Firmware version
	rc = elan_ktf2k_ts_get_data(client, cmd, buf_recv, 4);
	if (rc < 0)
		return rc;
	major = ((buf_recv[1] & 0x0f) << 4) | ((buf_recv[2] & 0xf0) >> 4);
	minor = ((buf_recv[2] & 0x0f) << 4) | ((buf_recv[3] & 0xf0) >> 4);
	ts->fw_ver = major << 8 | minor;
	FW_VERSION = ts->fw_ver;
// Firmware ID
	rc = elan_ktf2k_ts_get_data(client, cmd_id, buf_recv, 4);
	if (rc < 0)
		return rc;
	major = ((buf_recv[1] & 0x0f) << 4) | ((buf_recv[2] & 0xf0) >> 4);
	minor = ((buf_recv[2] & 0x0f) << 4) | ((buf_recv[3] & 0xf0) >> 4);
	ts->fw_id = major << 8 | minor;
	FW_ID = ts->fw_id;
// X Resolution
	rc = elan_ktf2k_ts_get_data(client, cmd_x, buf_recv, 4);
	if (rc < 0)
		return rc;
	minor = ((buf_recv[2])) | ((buf_recv[3] & 0xf0) << 4);
	ts->x_resolution =minor;
	X_RESOLUTION = ts->x_resolution;
	
// Y Resolution	
	rc = elan_ktf2k_ts_get_data(client, cmd_y, buf_recv, 4);
	if (rc < 0)
		return rc;
	minor = ((buf_recv[2])) | ((buf_recv[3] & 0xf0) << 4);
	ts->y_resolution =minor;
	Y_RESOLUTION = ts->y_resolution;
	
	printk(KERN_INFO "[elan] %s: firmware version: 0x%4.4x\n",
			__func__, ts->fw_ver);
	printk(KERN_INFO "[elan] %s: firmware ID: 0x%4.4x\n",
			__func__, ts->fw_id);
	printk(KERN_INFO "[elan] %s: x resolution: %d, y resolution: %d\n",
			__func__, X_RESOLUTION, Y_RESOLUTION);
	
	return 0;
}
#endif

static inline int elan_ktf2k_ts_parse_xy(uint8_t *data,
			uint16_t *x, uint16_t *y)
{
	*x = *y = 0;

	*x = (data[0] & 0xf0);
	*x <<= 4;
	*x |= data[1];

	*y = (data[0] & 0x0f);
	*y <<= 8;
	*y |= data[2];

	return 0;
}

static int elan_ktf2k_ts_setup(struct i2c_client *client)
{
	int rc;

	rc = __hello_packet_handler(client);
	printk("[elan] hellopacket's rc = %d\n",rc);

	mdelay(10);
	if (rc != 0x80){
		rc = __fw_packet_handler(client);
		if (rc < 0)
			pr_err("[elan] %s, fw_packet_handler fail, rc = %d", __func__, rc);
		pr_info("[elan] %s: firmware checking done.\n", __func__);
		//Check for FW_VERSION, if 0x0000 means FW update fail!
		if (FW_VERSION == 0x00) {
			rc = 0x80;
			pr_err("[elan] FW_VERSION = %d, last FW update fail\n", FW_VERSION);
		}
	}
	return rc;
}

static int elan_ktf2k_ts_rough_calibrate(struct i2c_client *client) 
{
	uint8_t cmd[] = {CMD_W_PKT, 0x29, 0x00, 0x01};

	pr_info("[elan] %s: enter\n", __func__);
	//DBG("[elan] dump cmd: %02x, %02x, %02x, %02x\n",
	//	cmd[0], cmd[1], cmd[2], cmd[3]);

	if ((i2c_master_send(client, cmd, sizeof(cmd))) != sizeof(cmd)) {
		DBG("[elan] %s: i2c_master_send failed\n", __func__);
		return -EINVAL;
	}

	return 0;
}

static int elan_ktf2k_ts_set_power_state(struct i2c_client *client, int state)
{
	uint8_t cmd[] = {CMD_W_PKT, 0x50, 0x00, 0x01};
	DBG("[elan] %s: enter\n", __func__);
	cmd[1] |= (state << 3);
	DBG("[elan] dump cmd: %02x, %02x, %02x, %02x\n",
		cmd[0], cmd[1], cmd[2], cmd[3]);

	if ((i2c_master_send(client, cmd, sizeof(cmd))) != sizeof(cmd)) {
		DBG("[elan] %s: i2c_master_send failed\n", __func__);
		return -EINVAL;
	}

	return 0;
}

static int elan_ktf2k_ts_get_power_state(struct i2c_client *client)
{
	int rc = 0;
	uint8_t cmd[] = {CMD_R_PKT, 0x50, 0x00, 0x01};
	uint8_t buf[4], power_state;

	rc = elan_ktf2k_ts_get_data(client, cmd, buf, 4);
	if (rc)
		return rc;

	power_state = buf[1];
	DBG("[elan] dump repsponse: %0x\n", power_state);
	power_state = (power_state & PWR_STATE_MASK) >> 3;
	DBG("[elan] power state = %s\n",
		power_state == PWR_STATE_DEEP_SLEEP ?
		"Deep Sleep" : "Normal/Idle");

	return power_state;
}

static int elan_ktf2k_ts_recv_data(struct i2c_client *client, uint8_t *buf)
{
	int rc, bytes_to_recv = PACKET_SIZE;

	DBG("%s: enter\n", __func__);
	if (buf == NULL)
		return -EINVAL;

	memset(buf, 0, bytes_to_recv);
	
	rc = i2c_master_recv(client, buf, 18);
	if (rc != 18)
		 pr_err("[elan] The first package error.\n");
	DBG("[elan] recv_data: %x %x %x %x %x %x %x %x\n", 
		buf[0], buf[1], buf[2], buf[3], buf[4], buf[5], buf[6], buf[7]);

	return rc;
}

static void elan_ktf2k_ts_report_data(struct i2c_client *client, uint8_t *buf)
{
	struct elan_ktf2k_ts_data *ts = i2c_get_clientdata(client);
	struct input_dev *idev = ts->input_dev;
	uint16_t x, y;
	uint16_t fbits=0;
	uint8_t i, num, reported = 0;
	uint8_t idx;
	int finger_num;
	int v;

	// for 5 fingers	
	if ((buf[0] == NORMAL_PKT) || (buf[0] == FIVE_FINGERS_PKT)){
		finger_num = 5;
		num = buf[1] & 0x07; 
		fbits = buf[1] >>3;
		idx=2;
	} else {
	// for 2 fingers      
		finger_num = 2;
		num = buf[7] & 0x03; 
		fbits = buf[7] & 0x03;
		idx=1;
	}
		
	switch (buf[0]) {
	case NORMAL_PKT:
	case TWO_FINGERS_PKT:
	case FIVE_FINGERS_PKT:	
	case TEN_FINGERS_PKT:
		//input_report_key(idev, BTN_TOUCH, 1);
		if (num == 0) {
			DBG("no press\n");
			if (key_pressed >= 0) {
				input_report_key(idev, OSD_mapping[key_pressed].key_event, 0);
				key_pressed = -1;
			} else {
				input_report_key(idev, BTN_TOUCH, 0);
				input_report_abs(idev, ABS_PRESSURE, 0);
				input_report_abs(idev, ABS_MT_TOUCH_MAJOR, 0);
				input_report_abs(idev, ABS_MT_WIDTH_MAJOR, 0);
				//input_report_key(idev, KEY_M, 0);
				input_mt_sync(idev);
			}
			press_down_in_a_area = 0;
		} else {
			DBG("[elan] %s: %d fingers\n", __func__, num);
			//input_report_key(idev, BTN_TOUCH, 1);
			for (i = 0; i < finger_num; i++) {	
				if ((fbits & 0x01)) {
					elan_ktf2k_ts_parse_xy(&buf[idx], &x, &y);
					DBG("[elan_debug] %s, x=%d, y=%d\n", __func__, x , y);
					if (x <= X_RESOLUTION && y <= Y_RESOLUTION) {
						if (x == ELAN_X_MAX) x--;
						if (x == 0) x = 1;
						if (y == ELAN_Y_FULL) y--;
						if (y == 0) y = 1;
						if (y >= y_limit) {
							if (y > y_limit + 15) {
								if (press_down_in_a_area == 0) {
								  for (v=0; v < 3; v++) {
										if (x > OSD_mapping[v].left_x && x < OSD_mapping[v].right_x) {
											input_report_key(idev, OSD_mapping[v].key_event, 1);
											key_pressed = v;
										}
									}
								}
							}
						} else {
							if (!reported)
								input_report_key(idev, BTN_TOUCH, 1);
							input_report_abs(idev, ABS_PRESSURE, 255);
							input_report_abs(idev, ABS_MT_TOUCH_MAJOR, 127);
							input_report_abs(idev, ABS_MT_WIDTH_MAJOR, 127);
							input_report_abs(idev, ABS_MT_POSITION_X, x);
							input_report_abs(idev, ABS_MT_POSITION_Y, y);
							input_report_abs(idev, ABS_MT_TRACKING_ID, i);
							input_mt_sync(idev);
							reported++;
							press_down_in_a_area = 1;
						}
					} // end if border
				} // end if finger status
				fbits = fbits >> 1;
				idx += 3;
			} // end for
		}
		if (reported) {
			input_sync(idev);
		} else {
			input_mt_sync(idev);
			input_sync(idev);
		}
		break;
	default:
		DBG("[elan] %s: unknown packet type: 0x%x\n", __func__, buf[0]);
		break;
	} // end switch

	return;
}

static void elan_ktf2k_ts_work_func(struct work_struct *work)
{
	int rc;
	struct elan_ktf2k_ts_data *ts =
	container_of(work, struct elan_ktf2k_ts_data, work);
	uint8_t buf[PACKET_SIZE] = { 0 };

	if (gpio_get_value(ts->intr_gpio)) {
		enable_irq(ts->client->irq);
		return;
	}
	
	rc = elan_ktf2k_ts_recv_data(ts->client, buf);
	if (rc < 0) {
		enable_irq(ts->client->irq);
		return;
	}

	DBG("[elan] work_func: %2x,%2x,%2x,%2x,%2x,%2x\n",
		buf[0],buf[1],buf[2],buf[3],buf[5],buf[6]);
	elan_ktf2k_ts_report_data(ts->client, buf);
	enable_irq(ts->client->irq);

	return;
}

static irqreturn_t elan_ktf2k_ts_irq_handler(int irq, void *dev_id)
{
	struct elan_ktf2k_ts_data *ts = dev_id;
	//struct i2c_client *client = ts->client;

	DBG("[elan] %s\n", __func__);
	disable_irq_nosync(ts->client->irq);
	queue_work(ts->elan_wq, &ts->work);

	return IRQ_HANDLED;
}

static int elan_ktf2k_ts_register_interrupt(struct i2c_client *client)
{
	struct elan_ktf2k_ts_data *ts = i2c_get_clientdata(client);
	int err = 0;

	err = request_irq(client->irq, elan_ktf2k_ts_irq_handler,
		IRQF_TRIGGER_LOW, client->name, ts);

	if (err)
		DBG("[elan] %s: request_irq %d failed (rc = %d)\n",
				__func__, client->irq, err);

	return err;
}

static void elan_ktf2k_ts_reset(void)
{
	int conf;
	if (!elan_Multitouch_gpio_initialized) {
		if (gpio_request(EKT_TS_MT_RESET_GPIO, "Multitouch Reset")) {
			pr_err("%s: failed to request gpio for Elan Multitouch Reset\n", __func__);
			return;
		}
		conf = GPIO_CFG(EKT_TS_MT_RESET_GPIO, 0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_2MA);
		if (gpio_tlmm_config(conf, GPIO_CFG_ENABLE) < 0) {
			pr_err("%s: unable to enable Elan Multitouch Reset!\n", __func__);
			gpio_free(EKT_TS_MT_RESET_GPIO);
			elan_Multitouch_gpio_initialized = 0;
			return;
		}
		elan_Multitouch_gpio_initialized = 1;
	}
	msleep(100);
	gpio_set_value(EKT_TS_MT_RESET_GPIO, 1);
	mdelay(10);
	gpio_set_value(EKT_TS_MT_RESET_GPIO, 0);
	mdelay(20);
	gpio_set_value(EKT_TS_MT_RESET_GPIO, 1);
	msleep(500);
}

static struct regulator *reg_ext_2v8 = 0;

static int lcd_elan_ktf2k_power_onoff(int on)
{
	int rc;
	pr_err("%s: on = %d\n", __func__, on);
	if (!reg_ext_2v8) {
		reg_ext_2v8 = regulator_get(NULL, "ext_2v8");
		if (IS_ERR(reg_ext_2v8)) {
			rc = PTR_ERR(reg_ext_2v8);
			pr_err("%s: '%s' regulator not found, rc=%d\n", __func__, "ext_2v8", rc);
			reg_ext_2v8 = NULL;
			return -19;
		}
	}
	if (on) {
		rc = regulator_enable(reg_ext_2v8);
		if (rc) {
			pr_err("%s: regulator enable failed, rc=%d\n", __func__, rc);
		} else {
			pr_err("%s: (on) success\n", __func__);
		}
	} else {
		rc = regulator_disable(reg_ext_2v8);
		if (rc) {
			pr_err("%s: regulator disable failed, rc=%d\n", __func__, rc);
		} else {
			pr_err("%s: (off) success\n", __func__);
		}
	}
	return rc;
}

static int elan_ktf2k_ts_probe(struct i2c_client *client,
			const struct i2c_device_id *id)
{
	int err = 0;
	int v;
	int fw_err = 0;
	struct elan_ktf2k_i2c_platform_data *pdata;
	struct elan_ktf2k_ts_data *ts;
	int New_FW_ID;	
	int New_FW_VER;	

	y_limit = ELAN_Y_MAX;
	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		printk(KERN_ERR "[elan] %s: i2c check functionality error\n", __func__);
		err = -ENODEV;
		goto err_check_functionality_failed;
	}

	ts = kzalloc(sizeof(struct elan_ktf2k_ts_data), GFP_KERNEL);
	if (ts == NULL) {
		printk(KERN_ERR "[elan] %s: allocate elan_ktf2k_ts_data failed\n", __func__);
		err = -ENOMEM;
		goto err_alloc_data_failed;
	}

	lcd_elan_ktf2k_power_onoff(1);

	ts->elan_wq = create_singlethread_workqueue("elan_wq");
	if (!ts->elan_wq) {
		printk(KERN_ERR "[elan] %s: create workqueue failed\n", __func__);
		err = -ENOMEM;
		goto err_create_wq_failed;
	}

	//ts->y_resolution = ELAN_Y_FULL;
	INIT_WORK(&ts->work, elan_ktf2k_ts_work_func);
	ts->client = client;
	i2c_set_clientdata(client, ts);
	elan_ktf2k_ts_reset();

	pdata = client->dev.platform_data;
	if (likely(pdata != NULL)) {
		ts->power = pdata->power;
		ts->intr_gpio = pdata->intr_gpio;
	}

	if (ts->power)
		ts->power(1);

	fw_err = elan_ktf2k_ts_setup(client);
	if (fw_err < 0) {
		printk(KERN_INFO "No Elan chip inside\n");
		//fw_err = -ENODEV;  
	}

	ts->input_dev = input_allocate_device();
	if (ts->input_dev == NULL) {
		err = -ENOMEM;
		DBG("[elan] Failed to allocate input device\n");
		goto err_input_dev_alloc_failed;
	}
	ts->input_dev->name = "elan-touchscreen";   // for andorid2.2 Froyo  

	pr_err("[ELAN] RESOLUTION: %d x %d \n", X_RESOLUTION, Y_RESOLUTION);
	
	set_bit(BTN_TOUCH, ts->input_dev->keybit);

	input_set_abs_params(ts->input_dev, ABS_X, 0, ELAN_X_MAX, 0, 0);
	input_set_abs_params(ts->input_dev, ABS_Y, 0, ELAN_Y_MAX, 0, 0);
	input_set_abs_params(ts->input_dev, ABS_PRESSURE, 0, 255, 0, 0);
	input_set_abs_params(ts->input_dev, ABS_MT_POSITION_X, 0, ELAN_X_MAX, 0, 0);
	input_set_abs_params(ts->input_dev, ABS_MT_POSITION_Y, 0, ELAN_Y_MAX, 0, 0);
	input_set_abs_params(ts->input_dev, ABS_MT_TOUCH_MAJOR, 0, 255, 0, 0);
	input_set_abs_params(ts->input_dev, ABS_MT_WIDTH_MAJOR, 0, 200, 0, 0);
				 
	input_set_abs_params(ts->input_dev, ABS_MT_TRACKING_ID, 0, 4, 0, 0);

	__set_bit(EV_ABS, ts->input_dev->evbit);
	__set_bit(EV_SYN, ts->input_dev->evbit);
	__set_bit(EV_KEY, ts->input_dev->evbit);
	__set_bit(INPUT_PROP_DIRECT, ts->input_dev->propbit);
	for (v=0; v < 3; v++) {
		set_bit(OSD_mapping[v].key_event, ts->input_dev->keybit);
	}

	// james: check input_register_devices & input_set_abs_params sequence
	err = input_register_device(ts->input_dev);
	if (err) {
		DBG("[elan]%s: unable to register %s input device\n",
			__func__, ts->input_dev->name);
		goto err_input_register_device_failed;
	}

	elan_ktf2k_ts_register_interrupt(ts->client);

	if (gpio_get_value(ts->intr_gpio) == 0) {
		pr_info("[elan] %s: handle missed interrupt\n", __func__);
		elan_ktf2k_ts_irq_handler(client->irq, ts);
	}

// james: check earlysuspend
#ifdef CONFIG_HAS_EARLYSUSPEND
	ts->early_suspend.level = EARLY_SUSPEND_LEVEL_STOP_DRAWING - 1;
	ts->early_suspend.suspend = elan_ktf2k_ts_early_suspend;
	ts->early_suspend.resume = elan_ktf2k_ts_late_resume;
	register_early_suspend(&ts->early_suspend);
#endif

	private_ts = ts; 

	elan_ktf2k_touch_sysfs_init();

	DBG("[elan] Start touchscreen %s in interrupt mode\n",
		ts->input_dev->name);

// Firmware Update
	ts->firmware.minor = MISC_DYNAMIC_MINOR;
	ts->firmware.name = "elan-iap";
	ts->firmware.fops = &elan_touch_fops;
	ts->firmware.mode = S_IFREG|S_IRWXUGO; 

	if (misc_register(&ts->firmware) < 0)
 		printk("[ELAN] misc_register failed!!\n");
 	else
		printk("[ELAN] misc_register finished!!\n");

// End Firmware Update	
#if IAP_PORTION
	printk("[ELAN] misc_register finished!!\n");
	work_lock=1;
	disable_irq(ts->client->irq);
	cancel_work_sync(&ts->work);
	power_lock = 1;
/* FW ID & FW VER*/
	DBG("[EKAN] [7bd0]=0x%02x,  [7bd1]=0x%02x, [7bd2]=0x%02x, [7bd3]=0x%02x\n",
		file_fw_data[31696],file_fw_data[31697],file_fw_data[31698],file_fw_data[31699]);
	New_FW_ID = file_fw_data[31699]<<8  | file_fw_data[31698];
	New_FW_VER = file_fw_data[31697]<<8  | file_fw_data[31696] ;
	printk("[ELAN] FW_ID=0x%x, New_FW_ID=0x%x \n",  FW_ID, New_FW_ID);
	printk("[ELAN] FW_VERSION=0x%x, New_FW_VER=0x%x \n", FW_VERSION, New_FW_VER);

// for firmware auto-upgrade

	if ((New_FW_ID != FW_ID) || (New_FW_VER != FW_VERSION))
		Update_FW_One(client, RECOVERY);
	
	//if (FW_ID == 0) {
	//	RECOVERY=0x80;
	//	Update_FW_One(client, RECOVERY);
	//}
	power_lock = 0;

	work_lock=0;
	enable_irq(ts->client->irq);
	pr_info("[ELAN] start iap update end\n");
#endif
	lcd_elan_ktf2k_power_onoff(0);
	pr_info("[ELAN] %s: end!\n", __func__);
	return 0;

err_input_register_device_failed:
	if (ts->input_dev)
		input_free_device(ts->input_dev);

err_input_dev_alloc_failed: 
	if (ts->elan_wq)
		destroy_workqueue(ts->elan_wq);

err_create_wq_failed:
	kfree(ts);

err_alloc_data_failed:
err_check_functionality_failed:

	return err;
}

static int elan_ktf2k_ts_remove(struct i2c_client *client)
{
	struct elan_ktf2k_ts_data *ts = i2c_get_clientdata(client);

	elan_touch_sysfs_deinit();

	unregister_early_suspend(&ts->early_suspend);
	free_irq(client->irq, ts);

	if (ts->elan_wq)
		destroy_workqueue(ts->elan_wq);
	input_unregister_device(ts->input_dev);
	kfree(ts);

	return 0;
}

static int elan_ktf2k_ts_suspend(struct i2c_client *client, pm_message_t mesg)
{
	struct elan_ktf2k_ts_data *ts = i2c_get_clientdata(client);
	int rc = 0;
	/* The power_lock can be removed when firmware upgrade procedure will not be enter into suspend mode.  */
	if (power_lock==0) {
		pr_info("[elan] %s: enter\n", __func__);
		disable_irq(client->irq);
		rc = cancel_work_sync(&ts->work);
		if (rc)
			enable_irq(client->irq);

		rc = elan_ktf2k_ts_set_power_state(client, PWR_STATE_DEEP_SLEEP);
	}
	if (ts->power)
		ts->power(0);
	return 0;
}

static int elan_ktf2k_ts_resume(struct i2c_client *client)
{
	int rc = 0, retry = 5;
	/* The power_lock can be removed when firmware upgrade procedure will not be enter into suspend mode.  */
	if (power_lock==0) {
		pr_info("[elan] %s: enter\n", __func__);
		if (socinfo_get_platform_version() == 0x20000) {
			gpio_set_value(EKT_TS_MT_RESET_GPIO, 1);
			msleep(10);
			gpio_set_value(EKT_TS_MT_RESET_GPIO, 0);
			msleep(10);
			gpio_set_value(EKT_TS_MT_RESET_GPIO, 1);
			enable_irq(client->irq);
		}
		do {
			rc = elan_ktf2k_ts_set_power_state(client, PWR_STATE_NORMAL);
			rc = elan_ktf2k_ts_get_power_state(client);
			if (rc == PWR_STATE_NORMAL)
				break;
			pr_err("[elan] %s: wake up tp failed! err = %d\n",
				__func__, rc);
			gpio_set_value(EKT_TS_MT_RESET_GPIO, 0);
			msleep(10);
			gpio_set_value(EKT_TS_MT_RESET_GPIO, 1);
		} while (--retry);
		enable_irq(client->irq);
	}
	return 0;
}

#ifdef CONFIG_HAS_EARLYSUSPEND
static void elan_ktf2k_ts_early_suspend(struct early_suspend *h)
{
	struct elan_ktf2k_ts_data *ts;
	ts = container_of(h, struct elan_ktf2k_ts_data, early_suspend);
	elan_ktf2k_ts_suspend(ts->client, PMSG_SUSPEND);
}

static void elan_ktf2k_ts_late_resume(struct early_suspend *h)
{
	struct elan_ktf2k_ts_data *ts;
	ts = container_of(h, struct elan_ktf2k_ts_data, early_suspend);
	elan_ktf2k_ts_resume(ts->client);
}
#endif

static const struct i2c_device_id elan_ktf2k_ts_id[] = {
	{ ELAN_KTF2K_NAME, 0 },
	{ }
};

static struct i2c_driver ektf2k_ts_driver = {
	.probe		= elan_ktf2k_ts_probe,
	.remove		= elan_ktf2k_ts_remove,
#ifndef CONFIG_HAS_EARLYSUSPEND
	//.suspend	= elan_ktf2k_ts_suspend,
	//.resume		= elan_ktf2k_ts_resume,
#endif
	.id_table	= elan_ktf2k_ts_id,
	.driver		= {
		.name = ELAN_KTF2K_NAME,
		.owner = THIS_MODULE,
	},
};

static int __devinit elan_ktf2k_ts_init(void)
{
	printk(KERN_INFO "[elan] %s: driver version 0x0003: Integrated 2 and 5 fingers together and auto-mapping resolution\n", __func__);
	return i2c_add_driver(&ektf2k_ts_driver);
}

static void __exit elan_ktf2k_ts_exit(void)
{
	i2c_del_driver(&ektf2k_ts_driver);
	return;
}

module_init(elan_ktf2k_ts_init);
module_exit(elan_ktf2k_ts_exit);

MODULE_DESCRIPTION("ELAN KTF2K Touchscreen Driver");
MODULE_LICENSE("GPL");


