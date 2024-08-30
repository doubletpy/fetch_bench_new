import sys
import os
import re
import subprocess

corenumber = '64'
setfreq = '1500000'
a = ''

strlist = ['echo userspace > /sys/devices/system/cpu/cpufreq/policy',corenumber,'/scaling_governor']
print(a.join(strlist))
process = subprocess.Popen(a.join(strlist), shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)

strlist = ['echo ',setfreq,'> /sys/devices/system/cpu/cpufreq/policy',corenumber,'/scaling_setspeed']
print(a.join(strlist))
process = subprocess.Popen(a.join(strlist), shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)

strlist = ['echo off > /sys/devices/system/cpu/smt/control']
print(a.join(strlist))
process = subprocess.Popen(a.join(strlist), shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)

strlist=['wrmsr -p ',corenumber,' 0xc0000108 0x0']
print(a.join(strlist))
process = subprocess.Popen(a.join(strlist), shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
