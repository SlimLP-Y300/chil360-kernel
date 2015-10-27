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

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/reboot.h>
#include <linux/pm.h>
#include <asm/system_misc.h>
#include <mach/proc_comm.h>

#include "devices-msm7x2xa.h"
#include "smd_rpcrouter.h"


#define RESTART_REASON_BOOT_BASE        0x77665500
#define RESTART_REASON_BOOTLOADER       (RESTART_REASON_BOOT_BASE | 0x00)
#define RESTART_REASON_REBOOT           (RESTART_REASON_BOOT_BASE | 0x01)
#define RESTART_REASON_RECOVERY         (RESTART_REASON_BOOT_BASE | 0x02)
#define RESTART_REASON_ERASE_EFS        (RESTART_REASON_BOOT_BASE | 0x03)
#define RESTART_REASON_NORMAL_BOOT      (RESTART_REASON_BOOT_BASE | 0x04)
#define RESTART_REASON_APANIC_RESET     (RESTART_REASON_BOOT_BASE | 0x05)
#define RESTART_REASON_RAMDUMP          (RESTART_REASON_BOOT_BASE | 0xAA)
#define RESTART_REASON_POWEROFF         (RESTART_REASON_BOOT_BASE | 0xBB)
#define RESTART_REASON_ERASE_FLASH      (RESTART_REASON_BOOT_BASE | 0xEF)

#define RESTART_REASON_OEM_BASE         0x6f656d00
#define RESTART_REASON_RIL_FATAL        (RESTART_REASON_OEM_BASE | 0x99)

#define MTK_DOWNLOAD_EN                 0x6d74646c


#if defined(CONFIG_ARCH_MSM7X00A)
static uint32_t restart_reason = RESTART_REASON_RAMDUMP;
#else
static uint32_t restart_reason = RESTART_REASON_REBOOT;
#endif

void msm_pm_power_off(void)
{
	pmem_log_start(9);
	msm_proc_comm(PCOM_POWER_DOWN, 0, 0);
	for (;;)
		;
}

void msm_pm_restart(char str, const char *cmd)
{
	pmem_log_start(9);
	
	if ((str >= '0') && (str <= '9'))
		restart_reason = RESTART_REASON_BOOT_BASE | (str - '0');

	if (str == 'a')
		restart_reason = RESTART_REASON_RAMDUMP;

	if (str == 'b')
		restart_reason = RESTART_REASON_POWEROFF;

	pr_info("The reset reason is 0x%08x\n", restart_reason);

	/* Disable interrupts */
	local_irq_disable();
	local_fiq_disable();

	/*
	 * Take out a flat memory mapping  and will
	 * insert a 1:1 mapping in place of
	 * the user-mode pages to ensure predictable results
	 * This function takes care of flushing the caches
	 * and flushing the TLB.
	 */
	setup_mm_for_reboot();

	msm_proc_comm(PCOM_RESET_CHIP, &restart_reason, 0);

	if (str) return;   // called by machine_restart()
	while (1);
}

static int msm_reboot_call
	(struct notifier_block *this, unsigned long code, void *_cmd)
{
	if ((code == SYS_RESTART) && _cmd) {
		char *cmd = _cmd;
		if (!strncmp(cmd, "bootloader", 10)) {
			restart_reason = RESTART_REASON_BOOTLOADER;
		} else if (!strncmp(cmd, "recovery", 8)) {
			restart_reason = RESTART_REASON_RECOVERY;
		} else if (!strncmp(cmd, "eraseflash", 10)) {
			restart_reason = RESTART_REASON_ERASE_FLASH;
		} else if (!strncmp(cmd, "oem-", 4)) {
			unsigned long code;
			int res;
			res = kstrtoul(cmd + 4, 16, &code);
			code &= 0xff;
			restart_reason = RESTART_REASON_OEM_BASE | code;
#ifdef CONFIG_HUAWEI_KERNEL
		} else if (!strcmp(cmd, "mtkupdate")) {
			restart_reason = MTK_DOWNLOAD_EN;
#endif
		} else if (!strcmp(cmd, "force-hard")) {
			restart_reason = RESTART_REASON_RAMDUMP;
		} else {
			restart_reason = RESTART_REASON_REBOOT;
		}
	}
	return NOTIFY_DONE;
}

static struct notifier_block msm_reboot_notifier = {
	.notifier_call = msm_reboot_call,
};

static int __init msm_reboot_notifier_init(void)
{
	int ret;
	ret = register_reboot_notifier(&msm_reboot_notifier);
	if (ret)
		pr_err("Failed to register reboot notifier\n");
	return ret;
}
late_initcall(msm_reboot_notifier_init);

static int __init msm_pm_restart_init(void)
{
	pm_power_off = msm_pm_power_off;
	arm_pm_restart = msm_pm_restart;
	return 0;
}
postcore_initcall(msm_pm_restart_init);
