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


# ===== SCP 收容監控系統（加分題）=====
import subprocess
import os

def capture_image(filename="capture.jpg"):
    """
    使用攝影機截圖
    """
    try:
        cmd = f"gst-launch-1.0 nvarguscamerasrc num-buffers=1 ! nvvidconv ! jpegenc ! filesink location={filename}"
        subprocess.run(cmd, shell=True, check=True, timeout=10)
        print(f"截圖已儲存: {filename}")
        return True
    except Exception as e:
        print(f"截圖失敗: {e}")
        return False


def scp_monitor():
    """
    SCP 收容監控系統 - 三階段防禦機制（背景持續運行）
    
    安全模式 (Secure Phase) ✅: ADC > 200
        - LED: 不亮
        - 攝影機: 不截圖
    
    警戒模式 (Warning Phase) ⚠️: ADC 70~200
        - LED: 交錯巡邏閃爍 (Perimeter Patrol)，2顆LED分組交替閃爍
        - 攝影機: 每3秒擷取一次畫面
    
    失效模式 (Breach Phase) 🚨: ADC < 70
        - LED: 高頻亂數暴閃 (Chaos Strobe)
        - 攝影機: 每0.5秒擷取一次畫面 (Full Surveillance)
    """
    import random
    
    # 初始化 GPIO
    init_gpio()
    
    last_capture_time = 0
    led_toggle = False
    
    print("=== SCP 收容監控系統啟動 ===", flush=True)
    
    while True:
        # 讀取光敏電阻值
        adc_value = read_photoresistor()
        current_time = time.time()
        
        if adc_value > 200:
            # ===== 安全模式 (Secure Phase) =====
            # LED: 不亮
            GPIO.output(LED1_PIN, GPIO.LOW)
            GPIO.output(LED2_PIN, GPIO.LOW)
            # 不截圖
            time.sleep(0.3)
            
        elif adc_value >= 70:
            # ===== 警戒模式 (Warning Phase) =====
            # LED: 交錯巡邏閃爍 - 兩顆 LED 交替
            led_toggle = not led_toggle
            if led_toggle:
                GPIO.output(LED1_PIN, GPIO.HIGH)
                GPIO.output(LED2_PIN, GPIO.LOW)
            else:
                GPIO.output(LED1_PIN, GPIO.LOW)
                GPIO.output(LED2_PIN, GPIO.HIGH)
            
            # 每 3 秒截圖一次
            if current_time - last_capture_time >= 3:
                capture_image("public/scp_latest.jpg")
                last_capture_time = current_time
            
            time.sleep(0.3)
            
        else:
            # ===== 失效模式 (Breach Phase) =====
            # 高頻閃爍 LED - 非阻塞方式
            led_toggle = not led_toggle
            GPIO.output(LED1_PIN, GPIO.HIGH if led_toggle else GPIO.LOW)
            GPIO.output(LED2_PIN, GPIO.LOW if led_toggle else GPIO.HIGH)

            # 每 0.5 秒截圖一次（非阻塞）
            if current_time - last_capture_time >= 0.5:
                # 使用子執行緒避免阻塞 LED
                import threading
                threading.Thread(target=capture_image, args=("public/scp_latest.jpg",), daemon=True).start()
                last_capture_time = current_time

            # 高頻閃爍延遲
            time.sleep(0.08)


def scp_read_status():
    """
    讀取 SCP 監控狀態（供網頁顯示用）
    """
    GPIO.setwarnings(False)
    GPIO.setmode(GPIO.BCM)
    GPIO.setup(SPIMOSI, GPIO.OUT)
    GPIO.setup(SPIMISO, GPIO.IN)
    GPIO.setup(SPICLK, GPIO.OUT)
    GPIO.setup(SPICS, GPIO.OUT)
    
    adc_value = read_photoresistor()
    
    if adc_value > 200:
        phase = "SECURE"
    elif adc_value >= 70:
        phase = "WARNING"
    else:
        phase = "BREACH"
    
    print(f"{phase}|{adc_value}")
    return phase, adc_value

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
    
    5. SCP 監控模式（加分題）：
       python gpio_control.py scp
    
    6. 讀取 SCP 狀態：
       python gpio_control.py scp_status
    """
    if len(sys.argv) < 2:
        print("使用方式:")
        print("  持續閃爍: python gpio_control.py blink")
        print("  讀取數值: python gpio_control.py read")
        print("  控制 LED: python gpio_control.py LED1 on|off")
        print("  停止 LED: python gpio_control.py stop")
        print("  SCP 監控: python gpio_control.py scp")
        print("  SCP 狀態: python gpio_control.py scp_status")
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
        
        elif command.lower() == 'scp':
            # 加分題：SCP 收容監控系統
            scp_monitor()
        
        elif command.lower() == 'scp_status':
            # 讀取 SCP 監控狀態
            scp_read_status()
        
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
