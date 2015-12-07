/*************************************************************************************   

	Driver code for himax touch screen control HX8527

       drive code  careat by linxc 2012-07-04 for 5 point panel
      
**************************************************************************************/

#include <linux/module.h>
#include <linux/delay.h>
#include <linux/earlysuspend.h>
#include <linux/hrtimer.h>
#include <linux/i2c.h>
#include <linux/input.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/platform_device.h>

#include <linux/slab.h> // to be compatible with linux kernel 3.2.15

#include <linux/gpio.h>		
#include <mach/gpio.h>		
#include <linux/input/mt.h>

#include <linux/input/himax_ts.h>		//linxc add 2012-08-28


static struct workqueue_struct *himax_wq;


struct himax_ts_data {
    uint16_t addr;
    struct i2c_client *client;
    struct input_dev *input_dev;
    int use_irq;
    struct hrtimer timer;
    struct work_struct  work;
    int (*power)(int on);
    struct early_suspend early_suspend;
    unsigned int total_touchpoint;
    char fw_revision[13];		//linxc 08-15
};

#ifdef REPORT_TYPE_ANONYMOUS_CONTACT	
    uint16_t key_type=0;	
    uint16_t old_key_type;
#endif	

struct touch_info{
	uint16_t x[5];		//ABS_MT_POSITION_X
	uint16_t y[5];		//ABS_MT_POSITION_Y
	int p[5];
	uint16_t area[5];   //ABS_MT_TOUCH_MAJOR
	uint8_t hot_key;
	uint8_t hot_key_pressed;
	int count;
};

int himax_sensor_key[SENSOR_KEY_NUMBER] = {
	KEY_BACK,
	KEY_HOMEPAGE,
	KEY_MENU
};


int is_chg_cmd_on = 0;
int charger_insert = 0;
#define CHG_PLUG_IN  1
#define CHG_PLUG_OUT 0

#ifdef SUPPORT_SYSFS_FW_REVISION	//linxc 12-08-16
static const char flash_fw_revision[13]={0};
#endif

#ifdef CONFIG_HAS_EARLYSUSPEND
static void himax_ts_early_suspend(struct early_suspend *h);
static void himax_ts_late_resume(struct early_suspend *h);
#endif

static uint32_t himax_Multitouch_gpio_initialized = 0;	
#define HIMAX_TP_RESET_GPIO  26

static int fw_update_retries = 1;

static int __init fw_update_retries_setup(char *str)
{
	get_option(&str, &fw_update_retries);
	return 0;
}
early_param("ts_fw", fw_update_retries_setup);


static void himax_ts_reset_ic(void)
{
   int rc = 0; 

	printk("%s\n", __FUNCTION__);

  if(!himax_Multitouch_gpio_initialized)
  {
    if (gpio_request(HIMAX_TP_RESET_GPIO, "Multitouch Reset")) {
      pr_err("linxc say failed to request gpio for sitronix Multitouch Reset\n");
      return;
    }

    rc = gpio_tlmm_config(GPIO_CFG(HIMAX_TP_RESET_GPIO, 0,
          GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP,
          GPIO_CFG_2MA), GPIO_CFG_ENABLE);
    if (rc < 0) {
      	pr_err("%s: linxc say unable to enable sitronix Multitouch Reset!\n", __func__);
  	gpio_free(HIMAX_TP_RESET_GPIO);	
  	himax_Multitouch_gpio_initialized = 0;	
	return;
  	}
	
     himax_Multitouch_gpio_initialized = 1;	
  }

  	msleep(100);
	gpio_set_value(HIMAX_TP_RESET_GPIO, 1);
	mdelay(10);
	gpio_set_value(HIMAX_TP_RESET_GPIO, 0);
	mdelay(10);
	gpio_set_value(HIMAX_TP_RESET_GPIO, 1);
	msleep(100);
}


static int himax_init_panel(struct himax_ts_data *ts)
{
    return 0;
}



static int Get_tpd_touchinfo(struct himax_ts_data *ts, struct touch_info *cinfo)
{
    int i, ret=0;
    struct i2c_msg msg[2];
    uint8_t buf[TOUCHPOINT_STACK_LENGTH];
    uint16_t high_byte, low_byte, x_coord, y_coord;
    static uint16_t I_info_x[5] = {0}, I_info_y[5]={0};	
    uint8_t start_reg;
    #ifdef SUPPORT_CHECKSUM_ON_CHIP
    uint16_t check_sum_cal = 0;
    #endif	
	

    start_reg = 0x86;   //contunt read

    memset(cinfo, 0, sizeof(struct touch_info));

    msg[0].addr = ts->client->addr;
    msg[0].flags = 0;
    msg[0].len = 1;
    msg[0].buf = &start_reg;
    
    msg[1].addr = ts->client->addr;
    msg[1].flags = I2C_M_RD;
    msg[1].len = TOUCHPOINT_STACK_LENGTH;		//read out 28 bytes bytes
    msg[1].buf = &buf[0];
	
    ret = i2c_transfer(ts->client->adapter, msg, 2);
    
    if (ret < 0) {  //clear buffer
      DMsg("read RAW data error(%d), clear buffer \n", ret);
      for(i=0; i<TOUCHPOINT_STACK_LENGTH; i++) 
	  buf[i] = 0xff;
    }

    DMsg("linxc received raw data from touch panel\n"); 

	#ifdef SUPPORT_CHECKSUM_ON_CHIP
       //for RAW Data checksum 
	for(i = 0; i < TOUCHPOINT_STACK_LENGTH-1; i++)
	{
	   	//DMsg("RAW Data[%d]: =%x \n", i, buf[i]);   	
		check_sum_cal += buf[i];
	}	
	   	//DMsg("RAW Data[%d]: =%x \n", i, buf[TOUCHPOINT_STACK_LENGTH-1]);   	
		//DMsg("linxc Checksum = 0x%x [%d]\n",check_sum_cal, check_sum_cal);				
	

	if ((check_sum_cal & 0x00ff) != buf[TOUCHPOINT_STACK_LENGTH-1] )
	{
	   	DMsg("linxc Check sum is error, turn back direct !!! \n");   			
		return false;
	}
      #endif	

     /* Get Hot key from Raw Data */
    cinfo->hot_key = buf[26]; 
   
     /* Get current pressed Point Number from RAW Data*/
    if( buf[24] == 0xff ){
	ts->total_touchpoint = 0;
    }	else{
	ts->total_touchpoint = buf[24] & 0x07;
    }

    DMsg("linxc read the HX8526A touch point number is %d\n", ts->total_touchpoint); 

    /* Tanform data */	
    for(i =0; i<TOUCHPOINT_MAX_NUM; i++)
    {
	 if(buf[4*i] != 0xff)
	 {
			/*get the X coordinate, 2 bytes*/
			high_byte = buf[4*i];
			high_byte <<= 8;
			low_byte = buf[4*i + 1];
			x_coord = high_byte | low_byte;
			if(x_coord<=RESOLUTION_X)		//AREA_DISPLAY
			{
				cinfo->x[i] = x_coord;
				I_info_x[i] = cinfo->x[i];
			}else{
				cinfo->x[i] = 0xFFFF;
				cinfo->y[i] = 0xFFFF;
				cinfo->p[i] = POINT_AREA_NONE;
				continue;
			}	
			
			/*get the Y coordinate, 2 bytes*/
			high_byte = buf[4*i+2];
			high_byte <<= 8;
			low_byte = buf[4*i+3];
			y_coord = high_byte | low_byte;	
			if(y_coord<=RESOLUTION_Y)
			{
			       cinfo->y[i] = y_coord;
				I_info_y[i] = cinfo->y[i];
			}		
			else{
				cinfo->x[i] = 0xFFFF;
				cinfo->y[i] = 0xFFFF;
				cinfo->p[i] = POINT_AREA_NONE;				
				continue;
			}

			cinfo->count++;
			cinfo->p[i] = POINT_AREA_DISPLAY;
	 }
	 else	//buffer dummy
	 {
			if (I_info_x[i] != 0xFFFF)
			{
				cinfo->x[i] = I_info_x[i];
				cinfo->y[i] = I_info_y[i];
				I_info_x[i] = 0xFFFF;
				I_info_y[i] = 0xFFFF;
			}
			else
			{
				cinfo->x[i] = 0xFFFF;
				cinfo->y[i] = 0xFFFF;
			}
			
		cinfo->p[i] = POINT_AREA_NONE;
			
	 }
	 
    }

	return true;
}

#ifdef REPORT_TYPE_ANONYMOUS_CONTACT
void hx8527a_release_old_key(struct input_dev *dev)
{
	if(old_key_type)
	{
		input_report_key(dev,KEY_UNKNOWN,EVENT_DOWN);
		input_report_key(dev,KEY_UNKNOWN,EVENT_UP);		
		old_key_type = 0;
	}
}
#endif

static void himax_ts_work_func(struct work_struct *work)
{
    int i;
    struct touch_info cinfo;
    struct himax_ts_data *ts = container_of(work, struct himax_ts_data, work);

     unsigned char buf0[3];
	
    if(ts==NULL){
		pr_err("%s: data is NULL\n", __func__);
		return;
    }

    if(Get_tpd_touchinfo(ts, &cinfo)) 
    {
           DMsg("himax_ts_work_func real count is cinfo.count=%d, hot_key=0x%x\n", cinfo.count, cinfo.hot_key); 

#ifdef REPORT_TYPE_ANONYMOUS_CONTACT

		switch(cinfo.hot_key){
		case 0x01:	
			key_type = himax_sensor_key[0];
			cinfo.hot_key_pressed = EVENT_DOWN;
			break;
		case 0x02:
			key_type = himax_sensor_key[1];
			cinfo.hot_key_pressed = EVENT_DOWN;
			break;			
		case 0x03:
			key_type = himax_sensor_key[2];
			cinfo.hot_key_pressed = EVENT_DOWN;
			break;		
		case 0xff:
			cinfo.hot_key_pressed = EVENT_UP;
			break;
		default:
			break;
		}

		if(cinfo.hot_key_pressed == EVENT_DOWN){
			old_key_type = key_type;
		}else{
			if(old_key_type != key_type)				
				hx8527a_release_old_key(ts->input_dev);
		}
		
		if(key_type)
		{
			DMsg(" Report Key key_type=%d, key_pressed=%d\n", key_type, cinfo.hot_key_pressed);		
		       input_report_key(ts->input_dev, key_type, cinfo.hot_key_pressed);	
		} 

		if(cinfo.hot_key_pressed==EVENT_UP){
			key_type=0;
		}
			
	 	for(i=0; i<TOUCHPOINT_MAX_NUM; i++)
	 	{
			//skill the error data  by linxc 12-08-13
			if((cinfo.x[i] == 0xffff) ||(cinfo.y[i] == 0xffff))
				continue;

		//for X start	
		if((cinfo.x[i] > 0) && (cinfo.x[i] <= 15))
		{
			cinfo.x[i] = 15;
		}	
		//X end
		if((cinfo.x[i] > 525) && (cinfo.x[i] <= 540))
		{
			cinfo.x[i] = 525;
		}	

		//for Y start
		if((cinfo.y[i] > 0) && (cinfo.y[i] <= 18))
		{
			cinfo.y[i] = 18;
		}
		//for Y end
		if((cinfo.y[i] > 940) && (cinfo.y[i] <= 960))
		{
			cinfo.y[i] = 940;
		}			
					
		       input_report_abs(ts->input_dev,  ABS_MT_TRACKING_ID, i);
			input_report_abs(ts->input_dev,  ABS_MT_POSITION_X, cinfo.x[i]);
			input_report_abs(ts->input_dev,  ABS_MT_POSITION_Y, cinfo.y[i]);						   
			if(cinfo.p[i] == POINT_AREA_DISPLAY)	  
			{
				DMsg("[%d](%d, %d)  pressure\n", i, cinfo.x[i], cinfo.y[i]);
	
				input_report_abs(ts->input_dev,  ABS_MT_TOUCH_MAJOR, 1);		
				input_report_abs(ts->input_dev,  ABS_MT_WIDTH_MAJOR, 5);	
				input_report_abs(ts->input_dev, ABS_MT_PRESSURE, 10);						
			}
			else
			{
				DMsg("[%d](%d, %d)  release \n", i, cinfo.x[i], cinfo.y[i]);				
				input_report_abs(ts->input_dev,  ABS_MT_TOUCH_MAJOR, 0);		
				input_report_abs(ts->input_dev,  ABS_MT_WIDTH_MAJOR, 0);		
				input_report_abs(ts->input_dev, ABS_MT_PRESSURE, 0);						
			}
			input_mt_sync(ts->input_dev);
		}		
		
		input_report_key(ts->input_dev, BTN_TOUCH, cinfo.count > 0);					
		input_sync(ts->input_dev);
#else
	    //Report Sensor Key by linxc 	
	    for(i=0; i<SENSOR_KEY_NUMBER; i++)
	    {
		  if(cinfo.hot_key== (i+1)){
			DMsg("& key[%d] down\n", i);
			input_report_key(ts->input_dev, himax_sensor_key[i], 1);
			input_sync(ts->input_dev);				
		  }else{
			DMsg("& key[%d] up\n", i);
			input_report_key(ts->input_dev, himax_sensor_key[i], 0);
			input_sync(ts->input_dev);		
		  }
	    }

	   //Report Touch points' location by linxc  
	    for(i=0; i<TOUCHPOINT_MAX_NUM; i++)
	    {
		   if(cinfo.p[i] != POINT_AREA_NONE)		
		   {
			input_mt_slot(ts->input_dev, i);	
		       input_mt_report_slot_state(ts->input_dev, MT_TOOL_FINGER, true);
			input_report_abs(ts->input_dev,  ABS_MT_POSITION_X, cinfo.x[i]);
			input_report_abs(ts->input_dev,  ABS_MT_POSITION_Y, cinfo.y[i]);
			input_report_abs(ts->input_dev,  ABS_MT_PRESSURE, 127);			
			DMsg("[%d](%d, %d)+\n", i, cinfo.x[i], cinfo.y[i]);	
		   }else{			
			input_mt_slot(ts->input_dev, i);	
			input_mt_report_slot_state(ts->input_dev, MT_TOOL_FINGER, false);	
			input_report_abs(ts->input_dev,  ABS_MT_POSITION_X, cinfo.x[i]);
			input_report_abs(ts->input_dev,  ABS_MT_POSITION_Y, cinfo.y[i]);
			input_report_abs(ts->input_dev,  ABS_MT_PRESSURE, 0);						
			DMsg("[%d](%d, %d)-\n", i, cinfo.x[i], cinfo.y[i]);	
		   }
	    }
		input_report_key(ts->input_dev, BTN_TOUCH, cinfo.count > 0);					
		input_sync(ts->input_dev);		
#endif

    }

//linxc 2012-08-29  for charger handle
	if( charger_insert == CHG_PLUG_IN)
	{
	     if(is_chg_cmd_on == 0)
	    {	    
		buf0[0] = 0x90; 	
		buf0[1] = 0x01; 
		
		if(i2c_master_send(ts->client, buf0, 2) < 0){
    			printk("[%s] I2C Write chg plug in cmd error\n",__func__);
    		}else{
			printk("linxc  charge on ,send cammand to TP! \n");
    		}
		is_chg_cmd_on = 1;
		msleep(5);					
	    }
	}
	else if(charger_insert == CHG_PLUG_OUT)
	{
	   if(is_chg_cmd_on == 1)
	   {
		buf0[0] = 0x90; 
		buf0[1] = 0x00;

		if(i2c_master_send(ts->client, buf0, 2) < 0){
    			printk("[%s] I2C Write chg plug out cmd error\n",__func__);
    		}else{
			printk("linxc : charge off ,send cammand to TP! \n");	
    		}
		is_chg_cmd_on = 0;		
		msleep(5);	
	   }	
	}	
//linxc 2012-08-29  for charger handle end
	
    
    if (ts->use_irq) 
    	enable_irq(ts->client->irq);
}

static enum hrtimer_restart himax_ts_timer_func(struct hrtimer *timer)
{
    struct himax_ts_data *ts = container_of(timer, struct himax_ts_data, timer);
	
    DMsg("himax_ts_timer_func\n"); 

    queue_work(himax_wq, &ts->work);

    hrtimer_start(&ts->timer, ktime_set(0, 12500000), HRTIMER_MODE_REL);
    return HRTIMER_NORESTART;
}

static irqreturn_t himax_ts_irq_handler(int irq, void *dev_id)
{
    struct himax_ts_data *ts = dev_id;

    DMsg("linxc trig himax_ts_irq_handle   ^..^  ^..^ ^..^ \n");
   
    disable_irq_nosync(ts->client->irq); //disable_irq(ts->client->irq);
    queue_work(himax_wq, &ts->work);
    return IRQ_HANDLED;
}


static int himax_ts_poweron(struct himax_ts_data *ts)
{
    uint8_t buf0[6];
    int ret = 0;

    buf0[0] = 0x42;
    buf0[1] = 0x02;
    ret = i2c_master_send(ts->client, buf0, 2);//Reload Disable
    if(ret < 0) {
        printk(KERN_ERR "i2c_master_send failed addr = 0x%x\n",ts->client->addr);
    }

    buf0[0] = 0x35;
    buf0[1] = 0x02;
    ret = i2c_master_send(ts->client, buf0, 2);//enable MCU
    if(ret < 0) {
        printk(KERN_ERR "i2c_master_send failed addr = 0x%x\n",ts->client->addr);
    }
	udelay(300);

    buf0[0] = 0x36;
    buf0[1] = 0x0F;
    buf0[2] = 0x53;
    ret = i2c_master_send(ts->client, buf0, 3);//enable flash
    if(ret < 0) {
        printk(KERN_ERR "i2c_master_send failed addr = 0x%x\n",ts->client->addr);
    } 
    udelay(300);

    buf0[0] = 0xDD;
    buf0[1] = 0x06;   //0x05;  linxc 0918
    buf0[2] = 0x02;
    ret = i2c_master_send(ts->client, buf0, 3);//prefetch
    if(ret < 0) {
        printk(KERN_ERR "i2c_master_send failed addr = 0x%x\n",ts->client->addr);
    } 

		//linxc modified for FW Himax_FW_Wintek_D9_Innos_Sharp_V13-D6-02_82HZ_2012-11-13_1006.i
    buf0[0] = 0x76;   //set Vdd
    buf0[1] = 0x01;  
    buf0[2] = 0x1B;
    ret = i2c_master_send(ts->client, buf0, 3);
    if(ret < 0) {
        printk(KERN_ERR "i2c_master_send failed addr = 0x%x\n",ts->client->addr);
    } 

    buf0[0] = 0x83;
    ret = i2c_master_send(ts->client, buf0, 1);//sense on
    if(ret < 0) {
        printk(KERN_ERR "i2c_master_send failed addr = 0x%x\n",ts->client->addr);
    } 
    msleep(200);

    buf0[0] = 0x81;
    ret = i2c_master_send(ts->client, buf0, 1);//sleep out
    if(ret < 0) {
        printk(KERN_ERR "i2c_master_send failed addr = 0x%x\n",ts->client->addr);
    }
    else{
	 printk(KERN_ERR "linxc write himax TP,  I2C addr is OK = 0x%x\n",ts->client->addr);
    }
    msleep(200);	

   return ret;
}


//2012-08-27  for chg   by linxc
void himax_set_chg_status(int chg_on)
{
	if(chg_on)
		charger_insert = CHG_PLUG_IN;
	else
		charger_insert = CHG_PLUG_OUT;
}

#ifdef SUPPORT_HIMAX_TP_FW_UPDATE	//linxc 12-08-15
uint8_t is_TP_Updated = 0;

//linxc update TP FW 2012-11-14
static unsigned char CTPM_FW[]=
{
	#include "Himax_FW_Wintek_D9_Innos_Sharp_V13-D6-03_82HZ_2012-11-13_1006.i"
};

unsigned char SFR_3u_1[16][2] = {{0x18,0x06},{0x18,0x16},{0x18,0x26},{0x18,0x36},{0x18,0x46},
                           				    {0x18,0x56},{0x18,0x66},{0x18,0x76},{0x18,0x86},{0x18,0x96},
							    {0x18,0xA6},{0x18,0xB6},{0x18,0xC6},{0x18,0xD6},{0x18,0xE6},
							    {0x18,0xF6}};

unsigned char SFR_6u_1[16][2] = {{0x98,0x04},{0x98,0x14},{0x98,0x24},{0x98,0x34},{0x98,0x44},
                                			    {0x98,0x54},{0x98,0x64},{0x98,0x74},{0x98,0x84},{0x98,0x94},
							    {0x98,0xA4},{0x98,0xB4},{0x98,0xC4},{0x98,0xD4},{0x98,0xE4},
							    {0x98,0xF4}};

void himax_ManualMode(struct himax_ts_data *ts, unsigned char enter)
{	
	uint8_t buf0[2];
	
	buf0[0] = 0x42;
    	buf0[1] = enter;
		
    	i2c_master_send(ts->client, buf0, 2);
}


void himax_FlashMode(struct himax_ts_data *ts, unsigned char enter)
{
	uint8_t buf0[2];
	buf0[0] = 0x43;
    	buf0[1] = enter;

	DMsg("linxc himax_FlashMode enter=%d\n", enter);
		
	i2c_master_send(ts->client, buf0, 2);
}

void himax_HW_reset(void)
{
	himax_ts_reset_ic();
}


void himax_lock_flash(struct himax_ts_data *ts)
{
	uint8_t buf0[6];

	printk("linxc himax_lock_flash\n");	
	/* lock sequence start */
	buf0[0] = 0x43; 
	buf0[1] = 0x01; 
	buf0[2] = 0x00; 
	buf0[3] = 0x06;
	i2c_master_send(ts->client, buf0, 4);

	buf0[0] = 0x44; 
	buf0[1] = 0x03; 
	buf0[2] = 0x00; 
	buf0[3] = 0x00;
	i2c_master_send(ts->client, buf0, 4);

	buf0[0] = 0x45; 
	buf0[1] = 0x00; 
	buf0[2] = 0x00; 
	buf0[3] = 0x7D; 
	buf0[4] = 0x03;
	i2c_master_send(ts->client, buf0, 5);

	buf0[0] = 0x4A;
	i2c_master_send(ts->client, buf0, 1);
	mdelay(50);
	/* lock sequence stop */
}

static int himax_unlock_flash(struct himax_ts_data *ts)
{
	int ret = 0;
	unsigned char buf0[6];

	DMsg("linxc himax_unlock_flash\n");
	
	/* unlock sequence start */
	buf0[0] = 0x43; 
	buf0[1] = 0x01; 
	buf0[2] = 0x00; 
	buf0[3] = 0x06;
	if(i2c_master_send(ts->client, buf0, 4) < 0){
		ret = -1;
		goto i2c_err;
    	}
	udelay(10);

	buf0[0] = 0x44;	
	buf0[1] = 0x03; 
	buf0[2] = 0x00; 
	buf0[3] = 0x00;
	if(i2c_master_send(ts->client, buf0, 4) < 0){
		ret = -1;
		goto i2c_err;
    	}
	udelay(10);

	buf0[0] = 0x45; 
	buf0[1] = 0x00; 
	buf0[2] = 0x00; 
	buf0[3] = 0x3D; 
	buf0[4] = 0x03;
	if(i2c_master_send(ts->client, buf0, 5) < 0){
		ret = -1;
		goto i2c_err;
    	}
	msleep(50);
	
	buf0[0] = 0x4A; 
	if(i2c_master_send(ts->client, buf0, 1) < 0){
		ret = -1;
		goto i2c_err;
    	}
	msleep(50);

	return ret;
	/* unlock sequence stop */

i2c_err:
    printk("[%s]  error exit\n",__func__);
    return ret;	
}


static int himax_modifyIref(struct himax_ts_data *ts)
{
	unsigned char i;
	unsigned char Iref[2] = {0x00,0x00};
	unsigned char buf0[6];
	
	struct i2c_msg msg[2];
    	uint8_t start_reg;
    	uint8_t buf[6];	

	printk("linxc himax_modifyIref\n");

	buf0[0] = 0x43; 
	buf0[1] = 0x01; 
	buf0[2] = 0x00; 
	buf0[3] = 0x08;
	if((i2c_master_send(ts->client, buf0, 4))< 0) return 0;
	
	buf0[0] = 0x44; 
	buf0[1] = 0x00; 
	buf0[2] = 0x00; 
	buf0[3] = 0x00;
	if((i2c_master_send(ts->client, buf0, 4))< 0) return 0;
	
	buf0[0] = 0x46; 
	if((i2c_master_send(ts->client, buf0, 1))< 0) return 0;

	start_reg = 0x59;	
    	msg[0].addr = ts->client->addr;
    	msg[0].flags = 0;
    	msg[0].len = 1;
    	msg[0].buf = &start_reg;
		
    	msg[1].addr = ts->client->addr;
    	msg[1].flags = I2C_M_RD;
    	msg[1].len = 4;
    	msg[1].buf = &buf[0];
		
    	buf[0] = 0;
       if((i2c_transfer(ts->client->adapter, msg, 2))< 0) return 0;
			 
	mdelay(5);
	for(i=0;i<16;i++)
	{
		if(buf[1] == SFR_3u_1[i][0] && buf[2] == SFR_3u_1[i][1])
		{
			Iref[0]= SFR_6u_1[i][0];
			Iref[1]= SFR_6u_1[i][1];
		} 
	}
	
	buf0[0] = 0x43; 
	buf0[1] = 0x01; 
	buf0[2] = 0x00; 
	buf0[3] = 0x06;
	if((i2c_master_send(ts->client, buf0, 4))< 0) return 0;
	
	buf0[0] = 0x44; 
	buf0[1] = 0x00; 
	buf0[2] = 0x00; 
	buf0[3] = 0x00;
	if((i2c_master_send(ts->client, buf0, 4))< 0) return 0;
	
	buf0[0] = 0x45; 
	buf0[1] = Iref[0]; 
	buf0[2] = Iref[1]; 
	buf0[3] = 0x27; 
	buf0[4] = 0x27;
	if((i2c_master_send(ts->client, buf0, 5))< 0) return 0;

	buf0[0] = 0x4A;
	if((i2c_master_send(ts->client, buf0, 1))< 0) return 0;

		
	return 1;	
}


/*
0x029C  Wintek
0x02A8   D9
0x02B4  Innos
0x02C0  V03L-D
0x02CC  2012/08/11
*/
#define FLASH_VERNO_ADDR 176	//Word Address
//if FW Ver is the same as flash, return 1, else return 0
int ctpm_read_FW_ver(struct himax_ts_data *ts)
{
	int ret = 0;
	uint16_t i;
    	unsigned char index_byte,index_page,index_sector;	
	const uint16_t FLASH_VER_START_ADDR = FLASH_VERNO_ADDR;
	const uint16_t FLASH_VER_END_ADDR = FLASH_VERNO_ADDR+3;		//12 bytes
	const uint16_t FW_VER_END_ADDR = FLASH_VERNO_ADDR*4;	
	unsigned short j = FW_VER_END_ADDR;	
	uint8_t buf0[6];

	struct i2c_msg msg[2];
	uint8_t start_reg;
	uint8_t buf[6];		

	//printk("ctpm_read_FW_ver  enter line: %d\n", __LINE__);	

	buf0[0] = 0x81;		//sleep out
       if((i2c_master_send(ts->client, buf0, 1))< 0){ 
		ret = -1;
		goto i2c_err;
	} 
	msleep(120);
	
	himax_FlashMode(ts,1);	//SET FLASH_EN=1

	for (i = FLASH_VER_START_ADDR; i < FLASH_VER_END_ADDR; i++)
	{
		//---------------------------------------------
		//Setting the flash address 
		//---------------------------------------------
		index_byte = i & 0x1F;				//Word offset
		index_page = (i >> 5) & 0x1F;		//Page number
		index_sector = (i >> 10) & 0x1F;		//sector number

		buf0[0] = 0x44;				 //SET FLASH ADDR 
		buf0[1] = index_byte;
		buf0[2] = index_page;
		buf0[3] = index_sector;
		if((i2c_master_send(ts->client, buf0, 4))< 0){ 
			ret = -1;
			goto i2c_err;
		}
		udelay(10);		

		buf0[0] = 0x46;				//Set Flash Reading Mode 
		if((i2c_master_send(ts->client, buf0, 1))< 0)	{ 
			ret = -1;
			goto i2c_err;
		}
		udelay(10);		

		//read a word (4 bytes) at a time from the flash memory 		
		start_reg = 0x59;
		msg[0].addr = ts->client->addr;
		msg[0].flags = 0;
		msg[0].len = 1;
		msg[0].buf = &start_reg;
    
		msg[1].addr = ts->client->addr;
		msg[1].flags = I2C_M_RD;
		msg[1].len = 4;
		msg[1].buf = &buf[0];
		buf[0] = 0;
		if((i2c_transfer(ts->client->adapter, msg, 2))< 0){ 
			ret = -1;
			goto i2c_err;
		}

		printk("linxc  version check, CTPW[%d]:0x%2x, buf[0]:0x%2x\n", j, CTPM_FW[j], buf[0]);				
		if (CTPM_FW[j] != buf[0])	goto fw_not_mach;	
		j++;

		printk("linxc  version check, CTPW[%d]:0x%2x, buf[1]:0x%2x\n", j, CTPM_FW[j], buf[1]);				
		if (CTPM_FW[j] != buf[1])	goto fw_not_mach;	
		j++;

		printk("linxc  version check, CTPW[%d]:0x%2x, buf[2]:0x%2x\n", j, CTPM_FW[j], buf[2]);				
		if (CTPM_FW[j] != buf[2])	goto fw_not_mach;
		j++;

		printk("linxc  version check, CTPW[%d]:0x%2x, buf[3]:0x%2x\n", j, CTPM_FW[j], buf[3]);				
		if (CTPM_FW[j] != buf[3])	goto fw_not_mach;
		j++;

		//copy 12 bytes version number
		if(i-FLASH_VERNO_ADDR == 0)
		{
			memcpy(&ts->fw_revision[0], buf, 4);
			//printk("fw revision (hex) = 0x%2x 0x%2x 0x%2x 0x%2x\n", ts->fw_revision[0], ts->fw_revision[1], ts->fw_revision[2], ts->fw_revision[3]);			
		}	
		else if(i-FLASH_VERNO_ADDR == 1)
		{
			memcpy(&ts->fw_revision[4], buf, 4);
			//printk("fw revision (hex) = 0x%2x 0x%2x 0x%2x 0x%2x\n", ts->fw_revision[4], ts->fw_revision[5], ts->fw_revision[6], ts->fw_revision[7]);				
		}	
		else if(i-FLASH_VERNO_ADDR == 2)
		{
			memcpy(&ts->fw_revision[8], buf, 4);
			//printk("fw revision (hex) = 0x%2x 0x%2x 0x%2x 0x%2x\n", ts->fw_revision[8], ts->fw_revision[9], ts->fw_revision[10], ts->fw_revision[11]);				
		}	
			
	}
	
	himax_FlashMode(ts,0);	

	printk("linxc &&& check firmware Ver Mach, don't need update again !!\n");		
	return 1;
	

fw_not_mach:
	printk("linxc &&& check firmware Ver no Mach, need update FW!\n");			
	return 0;

 i2c_err:
    	printk("[%s] error exit\n",__func__);
	return ret;		
}


//Success: return 1       fail: return 0
static uint8_t himax_calculateChecksum(struct himax_ts_data *ts, char *ImageBuffer, int fullLength)
{
	uint16_t checksum = 0;
	unsigned char  buf0[6], last_byte;
	int FileLength, i, k, lastLength;
	
	struct i2c_msg msg[2];
	uint8_t start_reg;
	uint8_t buf[6];

	printk("linxc himax_calculateChecksum\n");
		
	FileLength = fullLength - 2;	
	memset(buf0, 0x00, sizeof(buf0));

	//himax_HW_reset();

	if(himax_modifyIref(ts) == 0)
		return 0;
	
	himax_FlashMode(ts,1);

	FileLength = (FileLength + 3) / 4;
	for (i = 0; i < FileLength; i++) 
	{
		last_byte = 0;

		//Set Flash address (0x44H)
		buf0[0] = 0x44;
		buf0[1] = i & 0x1F;
		if (buf0[1] == 0x1F || i == FileLength - 1)
			last_byte = 1;			//is the last byte
		buf0[2] = (i >> 5) & 0x1F;
		buf0[3] = (i >> 10) & 0x1F;
		if((i2c_master_send(ts->client, buf0, 4))< 0)		return 0;

		buf0[0] = 0x46;
		if((i2c_master_send(ts->client, buf0, 1))< 0)		return 0;

		start_reg = 0x59;
		msg[0].addr = ts->client->addr;
		msg[0].flags = 0;
		msg[0].len = 1;
		msg[0].buf = &start_reg;
    
		msg[1].addr = ts->client->addr;
		msg[1].flags = I2C_M_RD;
		msg[1].len = 4;
		msg[1].buf = &buf[0];
		buf[0] = 0;
		if((i2c_transfer(ts->client->adapter, msg, 2))< 0)		return 0;

		if (i < (FileLength - 1))
		{
			checksum += buf[0] + buf[1] + buf[2] + buf[3];			
			if (i == 0)
				printk("linxc marked buf 0 to 3 (first): 0x%x, 0x%x, 0x%x, 0x%x\n", buf[0], buf[1], buf[2], buf[3]);			
		}
		else 
		{			
			printk("linxc marked buf 0 to 3 (last): 0x%x, 0x%x, 0x%x, 0x%x\n", buf[0], buf[1], buf[2], buf[3]);
			printk("linxc marked, checksum (not last): %d\n", checksum);
		
			lastLength = (((fullLength - 2) % 4) > 0)?((fullLength - 2) % 4):4;
			
			for (k = 0; k < lastLength; k++) 
				checksum += buf[k];		
	
			if (ImageBuffer[fullLength - 1] == (uint8_t)(0xFF & (checksum >> 8)) && ImageBuffer[fullLength - 2] == (uint8_t)(0xFF & checksum)) 			
			{	//check Success
				himax_FlashMode(ts, 0);
   				printk("linxc CTP calculate Checksum pass !!! \n");				
				goto check_success;
			} 
			else //Check Fail
			{
   				printk("linxc CTP calculate Checksum fail !!! \n");							
				himax_FlashMode(ts, 0);
				return 0;
			}
		}
	}

check_success:
   	return 1;
}

 
//return 1:Success, 0:Fail
int ctpm_FW_upgrade_with_i_file(struct himax_ts_data *ts)
{ 
	int ret = 0;
	//Get the Firmware.bin and Length
	unsigned char* ImageBuffer = CTPM_FW;
	int fullFileLength = sizeof(CTPM_FW);

	int i, j;
	uint8_t buf0[6];
	unsigned char last_byte, prePage;
	int fileLength;
	uint8_t checksumResult = 0;

	printk("linxc ctpm_FW_upgrade_with_i_file\n");

	//Try  Times
	for (j = 0; j < fw_update_retries; j++) 
	{
		fileLength = fullFileLength - 2;

		himax_HW_reset();

		buf0[0] = 0x81;		//sleep out
		if((i2c_master_send(ts->client, buf0, 1))< 0){ 
			ret = -1;
			goto i2c_err;
		}	
		mdelay(120);

		himax_unlock_flash(ts);

		//Set MASS_Erase_EN = 1
		buf0[0] = 0x43; 
		buf0[1] = 0x05; 
		buf0[2] = 0x00; 
		buf0[3] = 0x02;
		if((i2c_master_send(ts->client, buf0, 4))< 0){ 
			ret = -1;
			goto i2c_err;
		} 

		//Write MASS Erase CMD
		buf0[0] = 0x4F;
		if((i2c_master_send(ts->client, buf0, 1))< 0){ 
			ret = -1;
			goto i2c_err;
		} 
		mdelay(50);
		printk("linxc Write MASS Erase CMD pass\n");
		

		himax_ManualMode(ts, 1);
		//Set MASS_Erase_EN = 0
		himax_FlashMode(ts, 1);	

		fileLength = (fileLength + 3) / 4;		//Calculate flash address counter
		for (i = 0, prePage = 0; i < fileLength; i++) 
		{
			last_byte = 0;

			//Set Flash address (0x44H)
			buf0[0] = 0x44;
			buf0[1] = i & 0x1F;			//   Column
			if (buf0[1] == 0x1F || i == fileLength - 1)
				last_byte = 1;		//is the last byte
			buf0[2] = (i >> 5) & 0x1F;	//   page
			buf0[3] = (i >> 10) & 0x1F;	//   sector
			if(i2c_master_send(ts->client, buf0, 4)< 0){ 
				ret = -1;
				goto i2c_err;
			}

			if (prePage != buf0[1] || i == 0) 
			{
				prePage = buf0[1];

				//manual mode for 0x47  begin
				buf0[0] = 0x43; 
				buf0[1] = 0x01; 
				buf0[2] = 0x09; 
				buf0[3] = 0x02;
				if((i2c_master_send(ts->client, buf0, 4))< 0){ 
					ret = -1;
					goto i2c_err;
				}
				udelay(5);		//linxc
		   		
				buf0[0] = 0x43; 
				buf0[1] = 0x01; 
				buf0[2] = 0x0D; 
				buf0[3] = 0x02;
				if((i2c_master_send(ts->client, buf0, 4))< 0){ 
					ret = -1;
					goto i2c_err;
				} 

				buf0[0] = 0x43; 
				buf0[1] = 0x01; 
				buf0[2] = 0x09; 
				buf0[3] = 0x02;
				if((i2c_master_send(ts->client, buf0, 4))< 0){ 
					ret = -1;
					goto i2c_err;
				}
				//manual mode for 0x47  end

			}

			//Write 4 bytes data to Register 0x45H
			buf0[0] = 0x45;
			memcpy(&buf0[1], &ImageBuffer[4*i], 4);
			if((i2c_master_send(ts->client, buf0, 5))< 0){ 
				ret = -1;
				goto i2c_err;
			}

			buf0[0] = 0x43; 
			buf0[1] = 0x01;
			buf0[2] = 0x0D; 
			buf0[3] = 0x02;
			if((i2c_master_send(ts->client, buf0, 4))< 0){ 
				ret = -1;
				goto i2c_err;
			}
		   		
			buf0[0] = 0x43; 
			buf0[1] = 0x01; 
			buf0[2] = 0x09; 
			buf0[3] = 0x02;
			if((i2c_master_send(ts->client, buf0, 4))< 0){ 
				ret = -1;
				goto i2c_err;
			}
			
			if (last_byte == 1) 
			{
				buf0[0] = 0x43; 
				buf0[1] = 0x01; 
				buf0[2] = 0x01; 
				buf0[3] = 0x02;
				if((i2c_master_send(ts->client, buf0, 4))< 0){ 
					ret = -1;
					goto i2c_err;
				}
		   		
				buf0[0] = 0x43; 
				buf0[1] = 0x01; 
				buf0[2] = 0x05; 
				buf0[3] = 0x02;
				if((i2c_master_send(ts->client, buf0, 4))< 0){ 
					ret = -1;
					goto i2c_err;
				}

				buf0[0] = 0x43; 
				buf0[1] = 0x01; 
				buf0[2] = 0x01; 
				buf0[3] = 0x02;
				if((i2c_master_send(ts->client, buf0, 4))< 0){ 
					ret = -1;
					goto i2c_err;
				}
		   		
				buf0[0] = 0x43; 
				buf0[1] = 0x01; 
				buf0[2] = 0x00; 
				buf0[3] = 0x02;
				if((i2c_master_send(ts->client, buf0, 4))< 0){ 
					ret = -1;
					goto i2c_err;
				}

				mdelay(10);
				if (i == (fileLength - 1)) 
				{	
					printk("linxc Finish Program flash try time j=%d\n",j);

					himax_FlashMode(ts, 0);		//Set FLASH_EN = 0
					himax_ManualMode(ts, 0);	//Leave Manual Mode
					
					checksumResult = himax_calculateChecksum(ts,ImageBuffer, fullFileLength);	
					
					himax_lock_flash(ts);
					
					if (checksumResult) //Success
					{
						himax_HW_reset();
						goto update_success;
					} 
					else if ((j == 4) && (!checksumResult)) //Fail
					{
						himax_HW_reset();
						goto update_fail;
					} 
					else //Retry
					{
						printk("linxc Finish Program no success, Retry now \n");					
						himax_FlashMode(ts, 0);
						himax_ManualMode(ts, 0);
					}
				}
			}
		}
	}

 update_success:
   	printk("linxc CTP update FW With i file   success !!! \n");				
	return 1;

 update_fail:
   	printk("linxc CTP update FW With i file   fail  !!! \n");				
 	return 0;

 i2c_err:
    	printk("[%s] error exit\n",__func__);
	return ret;	
 	
}

#endif


#ifdef SUPPORT_SYSFS_FW_REVISION
static ssize_t hx8527_flash_fw_revision_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%s\n", flash_fw_revision);
}

static DEVICE_ATTR(fw_revision, S_IRUGO, hx8527_flash_fw_revision_show, NULL);

static struct attribute *hx8527_attributes[] = {
	&dev_attr_fw_revision.attr,
	NULL,			
};

static const struct attribute_group  himax_hx8527_attr_group = {
	.attrs = hx8527_attributes,
};
#endif

static int himax_ts_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
    struct himax_ts_data *ts;
    int ret = 0;
    int i;	
	
    printk("Himax TS probe\n"); 

    if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
        printk(KERN_ERR "himax_ts_probe: need I2C_FUNC_I2C\n");
        ret = -ENODEV;
        goto err_check_functionality_failed;
    }
	
    ts = kzalloc(sizeof(*ts), GFP_KERNEL);
    if (ts == NULL) {
        ret = -ENOMEM;
        goto err_alloc_data_failed;
    }
	
    INIT_WORK(&ts->work, himax_ts_work_func);
    ts->client = client;
    i2c_set_clientdata(client, ts);
	
    himax_ts_reset_ic();		

#ifdef SUPPORT_HIMAX_TP_FW_UPDATE
	if(is_TP_Updated == 0 && fw_update_retries >= 0)
	{
		ret = ctpm_read_FW_ver(ts);
		if (fw_update_retries == 0) {
			printk("JSR TP upgrade SKIP\n");
		} else
		if (ret == 0)
		{	//if not mach 
			if(ctpm_FW_upgrade_with_i_file(ts) == 0)	//update
				printk("JSR TP upgrade error, line: %d\n", __LINE__);
			else
				printk("JSR TP upgrade OK, line: %d\n", __LINE__);
			
			is_TP_Updated = 1;

			msleep(100);

			ctpm_read_FW_ver(ts);		//after fw update, ReGet FW Ver. 
		}
	}
#endif	
	
    himax_ts_poweron(ts);
	
    if(!client->irq) {
        msleep(300);
    }

    /* allocate input device */
    ts->input_dev = input_allocate_device();
    if (ts->input_dev == NULL) {
        ret = -ENOMEM;
        printk(KERN_ERR "himax_ts_probe: Failed to allocate input device\n");
        goto err_input_dev_alloc_failed;
    }
    ts->input_dev->name = "himax-touchscreen";

    ts->input_dev->id.bustype = BUS_I2C;
    ts->input_dev->dev.parent = &client->dev;	

#ifdef REPORT_TYPE_ANONYMOUS_CONTACT
	ts->input_dev->evbit[0] = BIT_MASK(EV_ABS) | BIT_MASK(EV_KEY);
	ts->input_dev->absbit[0] = BIT_MASK(ABS_MT_POSITION_X) | \
				 BIT_MASK(ABS_MT_POSITION_Y) | \
				 BIT_MASK(ABS_MT_TOUCH_MAJOR) | \
				 BIT_MASK(ABS_MT_WIDTH_MAJOR) | \
				 BIT_MASK(ABS_MT_TRACKING_ID) | \
				 BIT_MASK(ABS_PRESSURE);

	set_bit(KEY_UNKNOWN,ts->input_dev->keybit);
	set_bit(BTN_TOUCH, ts->input_dev->keybit);
	set_bit(INPUT_PROP_DIRECT, ts->input_dev->propbit);	//linxc 2012-12-22  +++
	

    for(i=0; i<SENSOR_KEY_NUMBER; i++)
    {
	set_bit(himax_sensor_key[i], ts->input_dev->keybit);
    }		

	input_set_abs_params(ts->input_dev, ABS_MT_POSITION_X, 0, RESOLUTION_X, 0, 0);
	input_set_abs_params(ts->input_dev, ABS_MT_POSITION_Y, 0, RESOLUTION_Y, 0, 0);
	input_set_abs_params(ts->input_dev, ABS_MT_TOUCH_MAJOR, 0, 255, 0, 0);
	input_set_abs_params(ts->input_dev, ABS_MT_WIDTH_MAJOR, 0, 255, 0, 0);
	input_set_abs_params(ts->input_dev, ABS_MT_TRACKING_ID, 0, 5, 0, 0);
	input_set_abs_params(ts->input_dev, ABS_PRESSURE, 0, 255, 0, 0);
#else
    set_bit(EV_KEY, ts->input_dev->evbit);
    set_bit(BTN_TOUCH, ts->input_dev->keybit);
    set_bit(EV_ABS, ts->input_dev->evbit);
    for(i=0; i<SENSOR_KEY_NUMBER; i++)
    {
	set_bit(himax_sensor_key[i], ts->input_dev->keybit);
    }	

    input_mt_init_slots(ts->input_dev, TOUCHPOINT_MAX_NUM);
	
    input_set_abs_params(ts->input_dev, ABS_MT_POSITION_X, 0, RESOLUTION_X, 0, 0);
    input_set_abs_params(ts->input_dev, ABS_MT_POSITION_Y, 0, RESOLUTION_Y, 0, 0);
    input_set_abs_params(ts->input_dev, ABS_MT_PRESSURE, 0, 255, 0, 0);
#endif
	
    ret = input_register_device(ts->input_dev);
    if (ret) {
        printk(KERN_ERR "himax_ts_probe: Unable to register %s input device\n", ts->input_dev->name);
        goto err_input_register_device_failed;
    }

    if (client->irq) {
	//himax driver ic need interrupt trign by low level
        ret = request_irq(client->irq, himax_ts_irq_handler, IRQF_DISABLED | IRQF_TRIGGER_LOW, client->name, ts);
	
        if (ret == 0) {
            printk("************************************\n");
            printk("*******Himax Touch Panel Driver********\n");
            printk("*******Creat by linxc 2012-07   *********\n");			
            printk("************************************\n");
            ts->use_irq = 1;
        }
        else 
	     dev_err(&client->dev, "request_irq failed\n");
    }
    
    if (!ts->use_irq) {
        hrtimer_init(&ts->timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
        ts->timer.function = himax_ts_timer_func;
        hrtimer_start(&ts->timer, ktime_set(1, 0), HRTIMER_MODE_REL);
    }
#ifdef CONFIG_HAS_EARLYSUSPEND
    ts->early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN + 1;
    ts->early_suspend.suspend = himax_ts_early_suspend;
    ts->early_suspend.resume = himax_ts_late_resume;
    register_early_suspend(&ts->early_suspend);
#endif

    printk(KERN_INFO "himax_ts_probe: Start touchscreen %s in %s mode\n", ts->input_dev->name, ts->use_irq ? "interrupt" : "polling");

#ifdef SUPPORT_SYSFS_FW_REVISION
	memcpy(&flash_fw_revision, &ts->fw_revision[0], sizeof(flash_fw_revision));

	ret = sysfs_create_group(&ts->input_dev->dev.kobj, 
			&himax_hx8527_attr_group);
	if(ret)
		goto err_unregister_dev;
#endif

    return 0;

#ifdef SUPPORT_SYSFS_FW_REVISION
err_unregister_dev:
	input_unregister_device(ts->input_dev);
#endif	
err_input_register_device_failed:
    input_free_device(ts->input_dev);

err_input_dev_alloc_failed:
//err_detect_failed:
err_alloc_data_failed:
err_check_functionality_failed:
    return ret;
}

static int himax_ts_remove(struct i2c_client *client)
{
    struct himax_ts_data *ts = i2c_get_clientdata(client);
#ifdef CONFIG_HAS_EARLYSUSPEND	
    unregister_early_suspend(&ts->early_suspend);
#endif
    if (ts->use_irq)
        free_irq(client->irq, ts);
    else
        hrtimer_cancel(&ts->timer);

   #ifdef SUPPORT_SYSFS_FW_REVISION
	sysfs_remove_group(&ts->input_dev->dev.kobj, 
			&himax_hx8527_attr_group);
   #endif
	
    input_unregister_device(ts->input_dev);
    kfree(ts);
    return 0;
}

static int himax_ts_suspend(struct i2c_client *client, pm_message_t mesg)
{
    int ret;
    uint8_t buffer[6];
	
    struct himax_ts_data *ts = i2c_get_clientdata(client);

    printk("Himax TouchScreen Suspend\n");

    /* TS sense off */
    buffer[0] = 0x82;
    ret = i2c_master_send(ts->client, buffer, 1);
    if(ret < 0) {
        printk(KERN_ERR "linxc say I2C send TS sense off Command ERROR = %d\n", ret);
    }
    msleep(120);

   /* TS Sleep in */	
    buffer[0] = 0x80;
    ret = i2c_master_send(ts->client, buffer, 1);
    if(ret < 0) {
        printk(KERN_ERR "linxc say I2C send TS sleep in  ERROR = %d\n", ret);
    }
    msleep(120);   

    /* ???  */
    buffer[0] = 0xD7;
    buffer[1] = 0x01;	
    ret = i2c_master_send(ts->client, buffer, 2);
    if(ret < 0) {
        printk(KERN_ERR "linxc say I2C send REG 0xD7  ERROR = %d\n", ret);
    }
    msleep(100);   	
    
    if (ts->use_irq)
        disable_irq(client->irq);
    else
        hrtimer_cancel(&ts->timer);

    ret = cancel_work_sync(&ts->work);
    if (ret && ts->use_irq) // if work was pending disable-count is now 2
        enable_irq(client->irq);

    return 0;
}

static int himax_ts_resume(struct i2c_client *client)
{
    int ret;
    uint8_t buffer[6];
	
    struct himax_ts_data *ts = i2c_get_clientdata(client);

    printk("Himax TS Resume\n");

    himax_init_panel(ts);
    
    /* ???  */
    buffer[0] = 0xD7;
    buffer[1] = 0x00;    	
    ret = i2c_master_send(ts->client, buffer, 2);
    if(ret < 0) {
        printk(KERN_ERR "himax_ts_resume:  linxc say I2C Write REG 0xD7  ERROR = %d\n", ret);
    }
    msleep(1);	

    himax_ts_poweron(ts);
    
    if (ts->use_irq) {
        printk("enabling IRQ %d\n", client->irq);
        enable_irq(client->irq);
    }
    else
        hrtimer_start(&ts->timer, ktime_set(1, 0), HRTIMER_MODE_REL);

    return 0;
}

#ifdef CONFIG_HAS_EARLYSUSPEND
static void himax_ts_early_suspend(struct early_suspend *h)
{
    struct himax_ts_data *ts;
    ts = container_of(h, struct himax_ts_data, early_suspend);
    himax_ts_suspend(ts->client, PMSG_SUSPEND);
}

static void himax_ts_late_resume(struct early_suspend *h)
{
    struct himax_ts_data *ts;
    ts = container_of(h, struct himax_ts_data, early_suspend);
    himax_ts_resume(ts->client);
}
#endif

#define HIMAX_TS_NAME "himax_ts"

static const struct i2c_device_id himax_ts_id[] = {
    { HIMAX_TS_NAME, 0 },
    { }
};

static struct i2c_driver himax_ts_driver = {
    .probe      = himax_ts_probe,
    .remove     = himax_ts_remove,
#ifndef CONFIG_HAS_EARLYSUSPEND
    .suspend    = himax_ts_suspend,
    .resume     = himax_ts_resume,
#endif
    .id_table   = himax_ts_id,
    .driver = {
        .name   = HIMAX_TS_NAME,
    },
};

static int __devinit himax_ts_init(void)
{
    printk("Himax TS init\n"); //[Step 0]
    himax_wq = create_singlethread_workqueue("himax_wq");
    if (!himax_wq)
        return -ENOMEM;
    return i2c_add_driver(&himax_ts_driver);
}

static void __exit himax_ts_exit(void)
{
    printk("Himax TS exit\n");
    i2c_del_driver(&himax_ts_driver);
    if (himax_wq)
        destroy_workqueue(himax_wq);
}

module_init(himax_ts_init);
module_exit(himax_ts_exit);

MODULE_DESCRIPTION("Himax Touchscreen Driver");
MODULE_LICENSE("GPL");
