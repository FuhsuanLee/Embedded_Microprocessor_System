import Jetson.GPIO as GPIO
import time

# GPIO 腳位設定
SPICLK = 11   # 23
SPIMISO = 9   # 21
SPIMOSI = 10  # 19
SPICS = 8     # 24
OUTPUT_PIN1 = 17  # 11
OUTPUT_PIN2 = 21  # 40
PHOTO_CH = 0       # 27


def init():
    # 初始化 GPIO 腳位
    GPIO.setwarnings(False)
    GPIO.cleanup()
    GPIO.setmode(GPIO.BCM)

    # 設定輸入與輸出腳位
    GPIO.setup(OUTPUT_PIN1, GPIO.OUT)
    GPIO.setup(OUTPUT_PIN2, GPIO.OUT)
    GPIO.setup(SPIMOSI, GPIO.OUT)
    GPIO.setup(SPIMISO, GPIO.IN)
    GPIO.setup(SPICLK, GPIO.OUT)
    GPIO.setup(SPICS, GPIO.OUT)

    # 預設輸出狀態為 LOW
    GPIO.output(OUTPUT_PIN1, GPIO.LOW)
    GPIO.output(OUTPUT_PIN2, GPIO.LOW)


def readadc(adcnum, clockpin, mosipin, misopin, cspin, output1, output2):
    # 讀取 ADC 值並依照結果控制 LED
    if adcnum < 0 or adcnum > 7:
        return -1

    GPIO.output(cspin, True)
    GPIO.output(clockpin, False)
    GPIO.output(cspin, False)

    commandout = adcnum
    commandout |= 0x18
    commandout <<= 3
    for i in range(5):
        GPIO.output(mosipin, bool(commandout & 0x80))
        commandout <<= 1
        GPIO.output(clockpin, True)
        GPIO.output(clockpin, False)

    adcout = 0
    for i in range(12):
        GPIO.output(clockpin, True)
        GPIO.output(clockpin, False)
        adcout <<= 1
        if GPIO.input(misopin):
            adcout |= 0x1

    GPIO.output(cspin, True)
    adcout >>= 1
    print(adcout)

    # 根據亮度值控制 LED
    if adcout > 200:
        GPIO.output(output1, GPIO.HIGH)
        GPIO.output(output2, GPIO.HIGH)
        print("LED1 on")
        print("LED2 on")
    elif adcout > 70:
        GPIO.output(output1, GPIO.HIGH)
        GPIO.output(output2, GPIO.LOW)
        print("LED1 on")
        print("LED2 off")
    else:
        GPIO.output(output1, GPIO.LOW)
        GPIO.output(output2, GPIO.LOW)
        print("LED1 off")
        print("LED2 off")

    return adcout


def main():
    # 主程式：重複讀取 ADC 並控制 LED
    init()
    while True:
        adc_value = readadc(PHOTO_CH, SPICLK, SPIMOSI, SPIMISO, SPICS, OUTPUT_PIN1, OUTPUT_PIN2)
        print(adc_value)
        time.sleep(1)


if __name__ == '__main__':
    main()
