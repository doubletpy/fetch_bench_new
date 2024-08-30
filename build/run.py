import sys
import os
import re
import subprocess

for i in range(1):
    corenumber = str(i)
    setfreq = '3100000'
    a = ''

    strlist = ['echo performance > /sys/devices/system/cpu/cpufreq/policy',corenumber,'/scaling_governor']
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
    
    #strlist=['./fetchbench -c ', corenumber, ' -t stream -i 1 -s 1 -f 250| tee out_stream_', corenumber, '_page.txt']
    #strlist=['./fetchbench -c ', corenumber, ' -t stride -i 1 -s 1 -f 250| tee out_stride_', corenumber, '_corr.txt']
    strlist=['sudo ./fetchbench -c ', corenumber, ' -t sms -i 1 -s 1 -f 250| tee out_region_', corenumber, '.txt']
    print(a.join(strlist))
    os.system(a.join(strlist))
