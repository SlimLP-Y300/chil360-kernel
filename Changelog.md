 
Chil360 Kernel
==============

The Chil360 Kernel is an Android kernel for the following devices:
* Huawei Ascend Y300 (U8833)
* Huawei G510 (U8951)
* Huawei G330 (U8825)

It is based on the [huawei-kernel-3.4] by Dazzozo

Changelog
---------
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
