#!/bin/bash
PWD := $(shell pwd)
cd ${PWD}/ko
sudo insmod cpu_load_monitor_kmod.ko
sudo insmod cpu_stat_monitor_kmod.ko
sudo insmod mem_monitor_kmod.ko
sudo insmod net_monitor_kmod.ko

chmod +x load_modules.sh