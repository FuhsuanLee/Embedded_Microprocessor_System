#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
GPIO 控制統一程式
整合 LED 控制和光敏電阻偵測功能
"""

import Jetson.GPIO as GPIO
import time
import sys

# SPI 介面腳位定義
SPICLK = 11   # GPIO 23
SPIMISO = 9   # GPIO 21
SPIMOSI = 10  # GPIO 19
SPICS = 8     # GPIO 24

# LED 腳位定義
LED1_PIN = 17  # GPIO 11
LED2_PIN = 21  # GPIO 15

# 光敏電阻 ADC 通道
PHOTO_CHANNEL = 0

def init_gpio(pins=None):
    """
    初始化 GPIO 腳位
    Args:
        pins: 要初始化的腳位列表，若為 None 則初始化所有腳位
    """
    GPIO.setwarnings(False)
    GPIO.cleanup()
    GPIO.setmode(GPIO.BCM)
    
    # 設定 SPI 腳位
    GPIO.setup(SPIMOSI, GPIO.OUT)
    GPIO.setup(SPIMISO, GPIO.IN)
    GPIO.setup(SPICLK, GPIO.OUT)
    GPIO.setup(SPICS, GPIO.OUT)
    
    # 設定 LED 腳位
    if pins is None:
        # 初始化所有 LED 腳位
        GPIO.setup(LED1_PIN, GPIO.OUT)
        GPIO.setup(LED2_PIN, GPIO.OUT)
    else:
        # 初始化指定的腳位
        for pin in pins:
            GPIO.setup(pin, GPIO.OUT)


def read_photoresistor():
    """
    讀取光敏電阻值（透過 MCP3008 ADC）
    Returns:
        int: ADC 讀取值 (0-1023)
    """
    adcnum = PHOTO_CHANNEL
    
    if (adcnum > 7) or (adcnum < 0):
        return -1
    
    GPIO.output(SPICS, True)
    GPIO.output(SPICLK, False)
    GPIO.output(SPICS, False)
    
    # 發送命令
    commandout = adcnum
    commandout |= 0x18
    commandout <<= 3
    
    for i in range(5):
        if (commandout & 0x80):
            GPIO.output(SPIMOSI, True)
        else:
            GPIO.output(SPIMOSI, False)
        commandout <<= 1
        GPIO.output(SPICLK, True)
        GPIO.output(SPICLK, False)
    
    # 讀取資料
    adcout = 0
    for i in range(12):
        GPIO.output(SPICLK, True)
        GPIO.output(SPICLK, False)
        adcout <<= 1
        if GPIO.input(SPIMISO):
            adcout |= 0x1
    
    GPIO.output(SPICS, True)
    adcout >>= 1
    
    return adcout


def control_led(led_number, state):
    """
    控制指定的 LED 開關（項目二）
    Args:
        led_number: LED 編號 (1 或 2)
        state: 'on' 或 'off'
    """
    # 選擇對應的 GPIO 腳位
    if led_number == 1:
        pin = LED1_PIN
    elif led_number == 2:
        pin = LED2_PIN
    else:
        print(f"錯誤：無效的 LED 編號 {led_number}")
        return
    
    # 初始化該腳位
    init_gpio([pin])
    
    # 控制 LED
    if state.lower() == 'on':
        GPIO.output(pin, GPIO.HIGH)
        print(f"LED{led_number} (Pin {pin}) 已開啟")
    elif state.lower() == 'off':
        GPIO.output(pin, GPIO.LOW)
        print(f"LED{led_number} (Pin {pin}) 已關閉")
    else:
        print(f"錯誤：無效的狀態 {state}")


def detect_and_control_leds():
    """
    持續讀取光敏電阻值並根據值控制 LED 閃爍頻率（項目一）
    此函數會無限循環，直到被外部終止
    
    ADC 值範圍: 0~400
    - 值越大（越亮）-> 閃爍越快
    - 值越小（越暗）-> 閃爍越慢
    """
    # 初始化所有需要的腳位
    init_gpio()
    
    led_state = False
    
    while True:
        # 讀取光敏電阻值
        adc_value = read_photoresistor()
        
        if adc_value <= 0:
            adc_value = 1  # 避免除以零
        
        # 切換 LED 狀態
        led_state = not led_state
        if led_state:
            GPIO.output(LED1_PIN, GPIO.HIGH)
            GPIO.output(LED2_PIN, GPIO.HIGH)
        else:
            GPIO.output(LED1_PIN, GPIO.LOW)
            GPIO.output(LED2_PIN, GPIO.LOW)
        
        # 根據光敏電阻值計算延遲時間
        # 值=0 -> 1秒, 值=400 -> 0.048秒
        delay = 20.0 / (adc_value + 20)
        time.sleep(delay)


def read_adc_only():
    """
    只讀取光敏電阻值，不控制 LED（供網頁顯示用）
    
    Returns:
        int: 光敏電阻值
    """
    # 初始化 SPI 腳位（不動 LED）
    GPIO.setwarnings(False)
    GPIO.setmode(GPIO.BCM)
    GPIO.setup(SPIMOSI, GPIO.OUT)
    GPIO.setup(SPIMISO, GPIO.IN)
    GPIO.setup(SPICLK, GPIO.OUT)
    GPIO.setup(SPICS, GPIO.OUT)
    
    adc_value = read_photoresistor()
    return adc_value


def stop_leds():
    """
    關閉所有 LED
    """
    GPIO.setwarnings(False)
    GPIO.setmode(GPIO.BCM)
    GPIO.setup(LED1_PIN, GPIO.OUT)
    GPIO.setup(LED2_PIN, GPIO.OUT)
    GPIO.output(LED1_PIN, GPIO.LOW)
    GPIO.output(LED2_PIN, GPIO.LOW)
    print("LED 已關閉")

def main():
    """
    主程式，處理命令列參數
    
    使用方式：
    1. 持續閃爍 LED（項目一 - 背景程序）：
       python gpio_control.py blink
    
    2. 只讀取光敏電阻值（供網頁顯示）：
       python gpio_control.py read
    
    3. 控制單個 LED（項目二）：
       python gpio_control.py LED1 on
       python gpio_control.py LED2 off
    
    4. 停止 LED：
       python gpio_control.py stop
    """
    if len(sys.argv) < 2:
        print("使用方式:")
        print("  持續閃爍: python gpio_control.py blink")
        print("  讀取數值: python gpio_control.py read")
        print("  控制 LED: python gpio_control.py LED1 on|off")
        print("  停止 LED: python gpio_control.py stop")
        sys.exit(1)
    
    command = sys.argv[1]
    
    try:
        if command.lower() == 'blink':
            # 項目一：持續根據光敏電阻值閃爍 LED（無限循環）
            detect_and_control_leds()
        
        elif command.lower() == 'read':
            # 只讀取光敏電阻值，不控制 LED
            adc_value = read_adc_only()
            print(adc_value)
        
        elif command.lower() == 'stop':
            # 停止 LED
            stop_leds()
        
        elif command.upper().startswith('LED'):
            # 項目二：控制指定 LED
            if len(sys.argv) < 3:
                print("錯誤：請指定 LED 狀態 (on/off)")
                sys.exit(1)
            
            led_number = int(command[3])  # 從 'LED1' 或 'LED2' 取得數字
            state = sys.argv[2]
            control_led(led_number, state)
        
        else:
            print(f"錯誤：未知的命令 '{command}'")
            sys.exit(1)
    
    except Exception as e:
        print(f"執行錯誤: {e}")
        import traceback
        traceback.print_exc()
        sys.exit(1)


if __name__ == '__main__':
    main()
