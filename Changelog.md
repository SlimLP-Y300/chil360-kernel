 
Chil360 Kernel
==============

The Chil360 Kernel is an Android kernel for the following devices:
* Huawei Ascend Y300 (U8833)
* Huawei G510 (U8951)
* Huawei G330 (U8825)

It is based on the [huawei-kernel-3.4] by Dazzozo

I have also used patches from the following repos, so credits go the owners
of these repos and their contributors.

* [Christopher83]
* [mrg666]
* [TeamHackLG]
* [moddingg33k]

Changelog
---------
v0.44
* Patch kernel from 3.4.94 to 3.4.95
* SOUND: updating to MSM-3.4 drivers

v0.43
* Patch kernel from 3.4.93 to 3.4.94
* QDSP Updates
* Android staging driver updates to ashmem, lowmemorykiller, binder, logger, etc.
* touchscreen: adding fuzz to improve ovation sensitivity
* More... (see commit log)

v0.42
* Patch kernel from 3.4.92 to 3.4.93
* Freezer Updates
* SoftIRQ updates
* Hotplug updates
* Implement LoUIS API for cache maintenance ops
* More... (see commit log)

v0.41
* Patch kernel from 3.4.91 to 3.4.92
* BFQ I/O scheduler v7r4
* Futex updates

v0.40
* Add LZ4 API
* ARM Lib updates - memset fixes/optimizations for GCC 4.7+ toochains

v0.39
* ZRAM updates
* I2C updates
* VFP updates
* Android alarm driver updates
* Use generic strnlen_user & strncpy_from_user functions
* More... (see commit log)

v0.38
* Patch kernel from 3.4.89 to 3.4.91
* Add TCP Small Queues
* Update LZO Compression
* User-space I/O driver support for HID
* Remove some unused code/features - fmem, swap token, VCM framework, token ring
* More... (see commit log)

v0.37
* Patch kernel from 3.4.88 to 3.4.89
* BFQ I/O scheduler v7r3
* radio-tavarua: update to msm-3.4
* power: add an API to log wakeup reasons
* CHROMIUM: mm: Fix calculation of dirtyable memory
* More... (see commit log)

v0.36
* Patch kernel from 3.4.87 to 3.4.88

v0.35
* A number of patches to random & prandom
* ext4: optimize test_root()
* fs/block-dev.c:fix performance regression in O_DIRECT writes to md block devices

v0.34
* Patch kernel from 3.4.86 to 3.4.87
* Interactive govenor patches

v0.33
* Patch kernel from 3.4.85 to 3.4.86
* Add SIOPLUS I/O scheduler
* Update OTG support
* mm, oom: normalize oom scores to oom_score_adj scale only for userspace

v0.32
* Patch kernel from 3.4.83 to 3.4.85
* Writeback patches
* Optimize jbd2_journal_force_commit
* fs/sync: Make sync() satisfy many requests with one invocation
* Improve Asynchronous I/O latency

v0.31
* Patch kernel from 3.4.82 to 3.4.83

v0.30
* Overclock GPU to 320 MHz (from CeXstel kernel)
* Disable asswax, smartassh3, smartassV2 to fix audio interuption issue when locking/unlocking

v0.29
* Frequency tables and UV (from CeXstel kernel)
* Y300 Overclock to 1046.7 MHz (from CeXstel kernel)
* More CPU Govenors: asswax, badass, dancedance, smartassh3, smartmax, wheatley (from CeXstel kernel)

v0.28
* Increased free RAM 404Mb
* Set deadline as default I/O scheduler 
* Tweak config to remove unneeded things: Ext2, Ext3, Highmem, RAM console
* Optimize square root algorithm

v0.27
* Move VFP init to an earlier boot stage
* Add NEON accelerated XOR implementation
* Add support for kernel mode NEON
* Optimized ARM RWSEM algorithm

v0.26
* ext4: speed up truncate/unlink by not using bforget() unless needed
* block/partitions: optimize memory allocation in check_partition()
* writeback: fix writeback cache thrashing
* cpufreq: Optimize cpufreq_frequency_table_verify()
* Power efficient work queue
* ZSMALLOC  from 3.10
* ZRAM from 3.10

v0.25
* Enable TCP congestion control algorthms with Westwood as default
* CK3 Tweaks
* Timer slack controller
* Dynamic dirty page writebacks
* Optimized SLUB
* Other MM tweaks

v0.24
* Add CPU Govenors: Intellidemand v5, Hyper, Adaptive, Lulzactive, SmartassV2, Lionheart, OndemandX
* Updates to Ondemand
* Updates to Interactive

v0.23
* Add FRandom
* BFQ I/O scheduler 7r2
* V(R) I/O scheduler
* FIOPS I/O scheduler
* ZEN I/O scheduler

v0.22
* Simple I/O scheduler (default)
* Dynamic FSync

v0.21
* Lowered swappiness 60->45
* Add optimized AES and SHA1 routines
* Add glibc memcopy and string
* Disabled Gentle Fair Sleepers for better UI performance 

v0.20
* Patch kernel from 3.4.0 to 3.4.82

v0.10
* Set optimization level by config
* Build Fixes to compile with -O3 & -Ofast
* Build fixes to compile with Linaro 4.7
* Build fixes to compile with Linaro 4.8

[huawei-kernel-3.4]:https://github.com/Dazzozo/huawei-kernel-3.4
[Christopher83]:https://github.com/Christopher83/samsung-kernel-msm7x30
[mrg666]:https://github.com/mrg666/android_kernel_mako/
[TeamHackLG]:https://github.com/TeamHackLG/lge-kernel-lproj
[moddingg33k]:https://github.com/moddingg33k/android_kernel_huawai_Y300-J1
