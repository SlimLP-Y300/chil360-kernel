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
#define SENSOR_NAME "gc0339"
#define PLATFORM_DRIVER_NAME "msm_camera_gc0339"
#define gc0339_obj gc0339_##obj

#ifdef CDBG
#undef CDBG
#endif
#ifdef CDBG_HIGH
#undef CDBG_HIGH
#endif

#define GC0339_VERBOSE_DGB

#ifdef GC0339_VERBOSE_DGB
#define CDBG(fmt, args...) printk(fmt, ##args)
#define CDBG_HIGH(fmt, args...) printk(fmt, ##args)
#else
#define CDBG(fmt, args...) do { } while (0)
#define CDBG_HIGH(fmt, args...) printk(fmt, ##args)
#endif

static struct msm_sensor_ctrl_t gc0339_s_ctrl;
DEFINE_MUTEX(gc0339_mut);

static struct msm_camera_i2c_reg_conf gc0339_prev_settings[] = {
  {0xfc,0x10},
  {0xfe,0x00},
  {0xf6,0x07},
  {0xf7,0x01},
  {0xf7,0x03},
  {0xfc,0x16},
  {0x06,0x00},
  {0x08,0x04},
  {0x09,0x01},
  {0x0a,0xe8},
  {0x0b,0x02},
  {0x0c,0x88},
  {0x0f,0x02},
  {0x14,0x23},
  {0x1a,0x21},
  {0x1b,0x80},
  {0x1c,0x49},
  {0x61,0x2a},
  {0x62,0x8c},
  {0x63,0x02},
  {0x32,0x00},
  {0x3a,0x20},
  {0x3b,0x20},
  {0x69,0x03},
  {0x60,0x84},
  {0x65,0x10},
  {0x6c,0xaa},
  {0x6d,0x00},
  {0x67,0x10},
  {0x4a,0x40},
  {0x4b,0x40},
  {0x4c,0x40},
  {0xe8,0x04},
  {0xe9,0xbb},
  {0x42,0x28},
  {0x47,0x10},
  {0x50,0x40},
  {0xd0,0x00},
  {0xd3,0x50},
  {0xf6,0x05},
  {0x01,0x6a},
  {0x02,0x0c},
  {0x0f,0x00},
  {0x6a,0x11},
  {0x71,0x01},
  {0x72,0x02},
  {0x79,0x02},
  {0x73,0x01},
  {0x7a,0x01},
  {0x2e,0x30},
  {0x2b,0x00},
  {0x2c,0x03},
  {0xd2,0x00},
  {0x20,0xb0},
  {0x60,0x94},
};

static struct msm_camera_i2c_reg_conf gc0339_snap_settings[] = {
  {0xfc,0x10},
  {0xfe,0x00},
  {0xf6,0x07},
  {0xf7,0x01},
  {0xf7,0x03},
  {0xfc,0x16},
  {0x06,0x00},
  {0x08,0x04},
  {0x09,0x01},
  {0x0a,0xe8},
  {0x0b,0x02},
  {0x0c,0x88},
  {0x0f,0x02},
  {0x14,0x23},
  {0x1a,0x21},
  {0x1b,0x80},
  {0x1c,0x49},
  {0x61,0x2a},
  {0x62,0x8c},
  {0x63,0x02},
  {0x32,0x00},
  {0x3a,0x20},
  {0x3b,0x20},
  {0x69,0x03},
  {0x60,0x84},
  {0x65,0x10},
  {0x6c,0xaa},
  {0x6d,0x00},
  {0x67,0x10},
  {0x4a,0x40},
  {0x4b,0x40},
  {0x4c,0x40},
  {0xe8,0x04},
  {0xe9,0xbb},
  {0x42,0x28},
  {0x47,0x10},
  {0x50,0x40},
  {0xd0,0x00},
  {0xd3,0x50},
  {0xf6,0x05},
  {0x01,0x6a},
  {0x02,0x0c},
  {0x0f,0x00},
  {0x6a,0x11},
  {0x71,0x01},
  {0x72,0x02},
  {0x79,0x02},
  {0x73,0x01},
  {0x7a,0x01},
  {0x2e,0x30},
  {0x2b,0x00},
  {0x2c,0x03},
  {0xd2,0x00},
  {0x20,0xb0},
  {0x60,0x94},
};

static struct msm_camera_i2c_reg_conf gc0339_recommend_settings[] = {
  {0xfc,0x10},
  {0xfe,0x00},
  {0xf6,0x07},
  {0xf7,0x01},
  {0xf7,0x03},
  {0xfc,0x16},
  {0x06,0x00},
  {0x08,0x04},
  {0x09,0x01},
  {0x0a,0xe8},
  {0x0b,0x02},
  {0x0c,0x88},
  {0x0f,0x02},
  {0x14,0x23},
  {0x1a,0x21},
  {0x1b,0x80},
  {0x1c,0x49},
  {0x61,0x2a},
  {0x62,0x8c},
  {0x63,0x02},
  {0x32,0x00},
  {0x3a,0x20},
  {0x3b,0x20},
  {0x69,0x03},
  {0x60,0x84},
  {0x65,0x10},
  {0x6c,0xaa},
  {0x6d,0x00},
  {0x67,0x10},
  {0x4a,0x40},
  {0x4b,0x40},
  {0x4c,0x40},
  {0xe8,0x04},
  {0xe9,0xbb},
  {0x42,0x28},
  {0x47,0x10},
  {0x50,0x40},
  {0xd0,0x00},
  {0xd3,0x50},
  {0xf6,0x05},
  {0x01,0x6a},
  {0x02,0x0c},
  {0x0f,0x00},
  {0x6a,0x11},
  {0x71,0x01},
  {0x72,0x02},
  {0x79,0x02},
  {0x73,0x01},
  {0x7a,0x01},
  {0x2e,0x30},
  {0x2b,0x00},
  {0x2c,0x03},
  {0xd2,0x00},
  {0x20,0xb0},
  {0x60,0x94},
};

static struct msm_camera_i2c_conf_array gc0339_init_conf[] = {
  {&gc0339_recommend_settings[0],
  ARRAY_SIZE(gc0339_recommend_settings), 0, MSM_CAMERA_I2C_BYTE_DATA}
};

static struct msm_camera_i2c_conf_array gc0339_confs[] = {
  {&gc0339_snap_settings[0],
  ARRAY_SIZE(gc0339_snap_settings), 0, MSM_CAMERA_I2C_BYTE_DATA},
  {&gc0339_prev_settings[0],
  ARRAY_SIZE(gc0339_prev_settings), 0, MSM_CAMERA_I2C_BYTE_DATA},
};

static struct msm_camera_csi_params gc0339_csi_params = {
  .data_format               = CSI_8BIT,
  .lane_cnt                  = 1,
  .lane_assign               = 0xe4,
  .dpcm_scheme               = 0,
  .settle_cnt                = 20,
};

static struct v4l2_subdev_info gc0339_subdev_info[] = {
  {
    .code                    = V4L2_MBUS_FMT_SBGGR10_1X10,
    .colorspace              = V4L2_COLORSPACE_JPEG,
    .fmt                     = 1,
    .order                   = 0,
  },
};

static struct msm_sensor_output_info_t gc0339_dimensions[] = {
  {
    .x_output                = 0x280,
    .y_output                = 0x1E0,
    .line_length_pclk        = 0x320,
    .frame_length_lines      = 0x1F4,
    .vt_pixel_clk            = 12000000,
    .op_pixel_clk            = 12000000,
    .binning_factor          = 1,
  },
};

static struct msm_sensor_output_reg_addr_t gc0339_reg_addr = {
  .x_output                  = 0x01,
  .y_output                  = 0x02,
  .line_length_pclk          = 0x0b,
  .frame_length_lines        = 0x09,
};

static struct msm_camera_csi_params *gc0339_csi_params_array[] = {
  &gc0339_csi_params,
  &gc0339_csi_params,
};

static struct msm_sensor_id_info_t gc0339_id_info = {
  .sensor_id_reg_addr        = 0x00,
  .sensor_id                 = 0xC8,
};

static struct msm_sensor_exp_gain_info_t gc0339_exp_gain_info = {
  .coarse_int_time_addr      = 0x51,
  .global_gain_addr          = 0x03,
  .vert_offset               = 4,
};

static int32_t gc0339_write_pict_exp_gain(struct msm_sensor_ctrl_t *s_ctrl,
  uint16_t gain, uint32_t line)
{
  int rc = 0;
  unsigned int  intg_time_msb, intg_time_lsb;
  CDBG("gc0339_write_pict_exp_gain,gain=%d, line=%d\n",gain,line);
  intg_time_msb = (unsigned int ) ((line & 0x0F00) >> 8);
  intg_time_lsb = (unsigned int ) (line& 0x00FF);
  if(gain>0xff)
    gain=0xff;
  msm_camera_i2c_write(s_ctrl->sensor_i2c_client,
    0x51,(gain),MSM_CAMERA_I2C_BYTE_DATA);
  msm_camera_i2c_write(s_ctrl->sensor_i2c_client,
    0x03,(intg_time_msb),MSM_CAMERA_I2C_BYTE_DATA);
  msm_camera_i2c_write(s_ctrl->sensor_i2c_client,
    0x04,(intg_time_lsb),MSM_CAMERA_I2C_BYTE_DATA);
  return rc;
}

static int32_t gc0339_write_prev_exp_gain(struct msm_sensor_ctrl_t *s_ctrl,
  uint16_t gain, uint32_t line)
{
  int rc = 0;
  unsigned int  intg_time_msb, intg_time_lsb;
  CDBG("gc0339_write_prev_exp_gain,gain=%d, line=%d\n",gain,line);
  intg_time_msb = (unsigned int ) ((line & 0x0F00) >> 8);
  intg_time_lsb = (unsigned int ) (line& 0x00FF);
  if(gain>0xff)
    gain=0xff;
  msm_camera_i2c_write(s_ctrl->sensor_i2c_client,
    0x51,(gain),MSM_CAMERA_I2C_BYTE_DATA);
  msm_camera_i2c_write(s_ctrl->sensor_i2c_client,
    0x03,(intg_time_msb),MSM_CAMERA_I2C_BYTE_DATA);
  msm_camera_i2c_write(s_ctrl->sensor_i2c_client,
    0x04,(intg_time_lsb),MSM_CAMERA_I2C_BYTE_DATA);
  return rc;
}

static const struct i2c_device_id gc0339_i2c_id[] = {
  {SENSOR_NAME, (kernel_ulong_t)&gc0339_s_ctrl},
  { }
};

int32_t gc0339_sensor_i2c_probe(struct i2c_client *client,
  const struct i2c_device_id *id)
{
  int32_t rc = 0;
  struct msm_sensor_ctrl_t *s_ctrl;
  CDBG("%s IN\r\n", __func__);
  s_ctrl = (struct msm_sensor_ctrl_t *)(id->driver_data);
  s_ctrl->sensor_i2c_addr = s_ctrl->sensor_i2c_addr;
  rc = msm_sensor_i2c_probe(client, id);
  if (client->dev.platform_data == NULL) {
    CDBG_HIGH("%s: NULL sensor data\n", __func__);
    return -EFAULT;
  }
  usleep_range(5000, 5100);
  return rc;
}

static struct i2c_driver gc0339_i2c_driver = {
  .id_table = gc0339_i2c_id,
  .probe = gc0339_sensor_i2c_probe,
  .driver = {
    .name = SENSOR_NAME,
  },
};

static struct msm_camera_i2c_client gc0339_sensor_i2c_client = {
  .addr_type = MSM_CAMERA_I2C_BYTE_ADDR,
};

static int __init msm_sensor_init_module(void)
{
  return i2c_add_driver(&gc0339_i2c_driver);
}

static struct v4l2_subdev_core_ops gc0339_subdev_core_ops = {
  .ioctl = msm_sensor_subdev_ioctl,
  .s_power = msm_sensor_power,
};

static struct v4l2_subdev_video_ops gc0339_subdev_video_ops = {
  .enum_mbus_fmt = msm_sensor_v4l2_enum_fmt,
};

static struct v4l2_subdev_ops gc0339_subdev_ops = {
  .core = &gc0339_subdev_core_ops,
  .video  = &gc0339_subdev_video_ops,
};

int32_t gc0339_sensor_power_down(struct msm_sensor_ctrl_t *s_ctrl)
{
  struct msm_camera_sensor_info *info = NULL;
  CDBG("%s IN\r\n", __func__);
  info = s_ctrl->sensordata;
  msleep(40);
  gpio_direction_output(info->sensor_pwd, 1);
  usleep_range(5000, 5100);
  msm_sensor_power_down(s_ctrl);
  msleep(40);
  /*
  if (s_ctrl->sensordata->pmic_gpio_enable){
    lcd_camera_power_onoff(0);
  }
  */
  return 0;
}

int32_t gc0339_sensor_power_up(struct msm_sensor_ctrl_t *s_ctrl)
{
  int32_t rc = 0;
  struct msm_camera_sensor_info *info = s_ctrl->sensordata;
  CDBG("%s IN\r\n", __func__);
  CDBG("%s, sensor_pwd:%d, sensor_reset:%d\r\n",__func__, info->sensor_pwd, info->sensor_reset);
  gpio_direction_output(info->sensor_pwd, 1);
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
  gpio_direction_output(info->sensor_pwd, 0);
  msleep(20);
  msm_camera_i2c_write(s_ctrl->sensor_i2c_client,
    0xfc, 0x10, MSM_CAMERA_I2C_BYTE_DATA);
  return rc;
}

int32_t gc0339_sensor_setting(struct msm_sensor_ctrl_t *s_ctrl,
  int update_type, int res)
{
  int32_t rc = 0;
  static int csi_config;
  if (update_type == MSM_SENSOR_REG_INIT) {
    CDBG("Register INIT\n");
    s_ctrl->curr_csi_params = NULL;
    msm_sensor_enable_debugfs(s_ctrl);
    csi_config = 0;
  } else if (update_type == MSM_SENSOR_UPDATE_PERIODIC) {
    CDBG("PERIODIC : %d\n", res);
    if (!csi_config) {
      s_ctrl->curr_csic_params = s_ctrl->csic_params[res];
      CDBG("CSI config in progress\n");
      v4l2_subdev_notify(&s_ctrl->sensor_v4l2_subdev,
        NOTIFY_CSIC_CFG, s_ctrl->curr_csic_params);
      CDBG("CSI config is done\n");
      mb();
      msleep(30);
      csi_config = 1;
      msm_sensor_write_conf_array(
        s_ctrl->sensor_i2c_client,
        s_ctrl->msm_sensor_reg->mode_settings, res);
    }
    msleep(50);
  }
  return rc;
}

int32_t gc0339_sensor_match_id(struct msm_sensor_ctrl_t *s_ctrl)
{
  uint16_t id = 0;
  int32_t rc = -1;
  CDBG("gc0339 sensor read id\n");
  rc = msm_camera_i2c_read(s_ctrl->sensor_i2c_client,
    s_ctrl->sensor_id_info->sensor_id_reg_addr,
    &id, MSM_CAMERA_I2C_BYTE_DATA);
  CDBG("gc0339 readid 0x%x : 0x%x\n", 0xC8, id);
  if (rc < 0) {
    CDBG("%s: read id failed\n", __func__);
    return rc;
  }
  if (id != s_ctrl->sensor_id_info->sensor_id) {
    return -ENODEV;
  }
  CDBG("gc0339 readid ok, success\n");
  return rc;
}

static struct msm_sensor_fn_t gc0339_func_tbl = {
  .sensor_start_stream = msm_sensor_start_stream,
  .sensor_stop_stream = msm_sensor_stop_stream,
  .sensor_group_hold_on = 0,
  .sensor_group_hold_off = 0,
  .sensor_set_fps = msm_sensor_set_fps,
  .sensor_write_exp_gain = gc0339_write_prev_exp_gain,
  .sensor_write_snapshot_exp_gain = gc0339_write_pict_exp_gain,
  .sensor_csi_setting = gc0339_sensor_setting,
  .sensor_set_sensor_mode = msm_sensor_set_sensor_mode,
  .sensor_mode_init = msm_sensor_mode_init,
  .sensor_get_output_info = msm_sensor_get_output_info,
  .sensor_config = msm_sensor_config,
  .sensor_power_up = gc0339_sensor_power_up,
  .sensor_power_down = gc0339_sensor_power_down,
  .sensor_match_id   = gc0339_sensor_match_id,
};

static struct msm_sensor_reg_t gc0339_regs = {
  .default_data_type = MSM_CAMERA_I2C_BYTE_DATA,
  .start_stream_conf = 0,
  .stop_stream_conf = 0,
  .group_hold_on_conf = 0,
  .group_hold_off_conf = 0,
  .init_settings = &gc0339_init_conf[0],
  .init_size = ARRAY_SIZE(gc0339_init_conf),
  .mode_settings = &gc0339_confs[0],
  .output_settings = &gc0339_dimensions[0],
  .num_conf = ARRAY_SIZE(gc0339_confs),
};

static struct msm_sensor_ctrl_t gc0339_s_ctrl = {
  .msm_sensor_reg = &gc0339_regs,
  .sensor_i2c_client = &gc0339_sensor_i2c_client,
  .sensor_i2c_addr =  0x42 ,
  .sensor_output_reg_addr = &gc0339_reg_addr,
  .sensor_id_info = &gc0339_id_info,
  .sensor_exp_gain_info = &gc0339_exp_gain_info,
  .cam_mode = MSM_SENSOR_MODE_INVALID,
  .csic_params = &gc0339_csi_params_array[0],
  .msm_sensor_mutex = &gc0339_mut,
  .sensor_i2c_driver = &gc0339_i2c_driver,
  .sensor_v4l2_subdev_info = gc0339_subdev_info,
  .sensor_v4l2_subdev_info_size = ARRAY_SIZE(gc0339_subdev_info),
  .sensor_v4l2_subdev_ops = &gc0339_subdev_ops,
  .func_tbl = &gc0339_func_tbl,
  .clk_rate = MSM_SENSOR_MCLK_24HZ,
};

module_init(msm_sensor_init_module);
MODULE_DESCRIPTION("Galaxycore VGA Bayer sensor driver");
MODULE_LICENSE("GPL v2");