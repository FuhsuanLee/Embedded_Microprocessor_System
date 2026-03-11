# Embedded Microprocessor System Labs Overview

A little summary of each lab. </br>
Each lab focused on a specific concept that later contributed to the final system implementation.

---

**Lab1 - Embedded Linux Environment Setup**

- Set up the Embedded Linux development environment
- Learn basic Linux commands and system operations
- Configure cross-compilation and remote development workflow

**Lab2 - GPIO Basic Control**

- Control GPIO pins through /sys/class/gpio
- Configure GPIO **direction** and **value**
- Implement basic LED on/off control

**Lab3 - GUI and Hardware Integration**

- Develop a GUI using **Qt Creator**
- Connect GUI controls with GPIO hardware
- Implement LED control buttons and user interaction

**Lab4 - Web-Based GPIO Control**

- Build a **Node.js web server**
- Implement frontend-backend communication
- Control GPIO devices through a web interface

**Lab5 - Linux Kernel Module**

- Develop a basic Linux kernel module
- Learn how to use insmod, rmmod, and dmesg
- Understand the interaction between **Kernel Space and User Space**

**Lab6 - Sensor and ADC Reading**

- Use **MCP3008** to read analog signals
- Retrieve light sensor values through SPI
- Control LEDs based on environmental brightness

**Lab7 - Remote Peripheral Control**

- Implement remote device control mechanisms
- Integrate web interfaces with hardware control
- Build a basic remote control workflow

**Lab8 - Character Device Driver**

- Implement a **Linux Character Device Driver**
- Use ioctl to communicate between User Space and Kernel Space
- Control LEDs and return system status information

**Lab9 - Multithreading Programming**

- Implement multithreading using **C++ Thread**
- Prevent blocking in the main program
- Use LEDs to indicate computation status

**Lab10 - Sensor Data and System Integration**

- Read ADC sensor values using **Python**
- Call Python scripts from a **C++ program**
- Integrate multithreading with sensor data processing
