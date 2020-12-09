#!/bin/bash

function key1_handler()
{
    for i in $(seq 0 ${OL_NUM}); do
        if [ -e /sys/devices/system/cpu/cpu${i}/online ]; then
            let online=`cat /sys/devices/system/cpu/cpu${i}/online` 
        else
            let online=1
        fi
        if [ $online -eq 1 ]; then
            local freq=$(cat /sys/devices/system/cpu/cpu${i}/cpufreq/cpuinfo_cur_freq)
            printf "CPU%s: %dMHz\n" ${i} $((freq/1000))
        fi
    done
    local temp=$(cat /sys/class/thermal/thermal_zone0/temp)
    printf "\nTemp: %dC\n" $((temp/1000))

    local ip=$(hostname -I)
    printf "\nIP: ${ip}\n"
}

function key2_handler()
{
    printf "This is key2.\n\nDefine in lcdhat-helper.sh."
}

function key3_handler()
{
    printf "This is key3.\n\nDefine in lcdhat-helper.sh."
}

function key4_handler()
{
    printf "This is key4.\n\nDefine in lcdhat-helper.sh."
}

function get_cpu_num()
{
    if [ -e ${OL_FILE} ]; then
        OL_NUM=`cat ${OL_FILE} | cut -f 2 -d"-"`
    else
        echo "${OL_FILE} not found"
        exit 1
    fi
}

if [[ $# -eq 2 ]]; then
    echo "$0 key(1~4)"
    exit 1
fi

OL_FILE=/sys/devices/system/cpu/online
get_cpu_num

KEY=$1
key${KEY}_handler