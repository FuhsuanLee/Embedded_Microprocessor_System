#!/usr/bin/env python3
# -*- coding: utf-8 -*-

"""
get_adc.py
功能：
- 每次執行程式時讀取一次光敏電阻 ADC
- 直接 print 出數值
- 程式結束
"""

import Jetson.GPIO as GPIO
import time

# ======================
# SPI 腳位設定（BCM）
# ======================
SPICLK  = 11   # GPIO 11
SPIMISO = 9    # GPIO 9
SPIMOSI = 10   # GPIO 10
SPICS   = 8    # GPIO 8

# ======================
# GPIO 初始化
# ======================
GPIO.setmode(GPIO.BCM)
GPIO.setup(SPIMOSI, GPIO.OUT)
GPIO.setup(SPIMISO, GPIO.IN)
GPIO.setup(SPICLK, GPIO.OUT)
GPIO.setup(SPICS, GPIO.OUT)

# ======================
# MCP3008 ADC 讀取
# ======================
def read_adc(channel=0):
    if channel < 0 or channel > 7:
        return -1

    GPIO.output(SPICS, True)
    GPIO.output(SPICLK, False)
    GPIO.output(SPICS, False)

    command = channel
    command |= 0x18
    command <<= 3

    for _ in range(5):
        GPIO.output(SPIMOSI, bool(command & 0x80))
        command <<= 1
        GPIO.output(SPICLK, True)
        GPIO.output(SPICLK, False)

    result = 0
    for _ in range(12):
        GPIO.output(SPICLK, True)
        GPIO.output(SPICLK, False)
        result <<= 1
        if GPIO.input(SPIMISO):
            result |= 0x1

    GPIO.output(SPICS, True)
    return result >> 1

# ======================
# Main：一次執行一次讀
# ======================
if __name__ == "__main__":
    try:
        adc_value = read_adc(0)
        print(adc_value)

    finally:
        GPIO.cleanup()
