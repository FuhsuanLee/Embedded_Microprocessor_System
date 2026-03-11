#include <iostream>
#include <fstream>
#include <string>
#include <unistd.h>
using namespace std;

bool file_exists(const string &path) {
    ifstream f(path);
    return f.good();
}

void gpio_export(int pin) {
    if (!file_exists("/sys/class/gpio/gpio" + to_string(pin))) {
        ofstream exportFile("/sys/class/gpio/export");
        if (exportFile.is_open()) {
            exportFile << pin;
            exportFile.close();
        }
    }
}

void gpio_set_direction(int pin, const string &dir) {
    string path = "/sys/class/gpio/gpio" + to_string(pin) + "/direction";
    ofstream dirFile(path);
    if (dirFile.is_open()) {
        dirFile << dir;
        dirFile.close();
    }
}

void gpio_set_value(int pin, int value) {
    string path = "/sys/class/gpio/gpio" + to_string(pin) + "/value";
    ofstream valFile(path);
    if (valFile.is_open()) {
        valFile << value;
        valFile.close();
    }
}

int main(int argc, char* argv[]) {
    int leds[4] = {396, 466, 397 ,398};

    // 初始化 GPIO
    // for (int i = 0; i < 4; i++) {
	// gpio_export(leds[i]);
        // gpio_set_direction(leds[i], "out");
    // }

    if (argc == 3 && string(argv[1]) == "Mode_Shine") {
        int times = stoi(argv[2]);
	for (int i = 0; i < 4; i++) {
	    gpio_export(leds[i]);
	    gpio_set_direction(leds[i], "out");
	}
        for (int i = 0; i < times; i++) {
            gpio_set_value(leds[0], 1);
            gpio_set_value(leds[1], 1);
            gpio_set_value(leds[2], 0);
            gpio_set_value(leds[3], 0);
            sleep(1);

            gpio_set_value(leds[0], 0);
            gpio_set_value(leds[1], 0);
            gpio_set_value(leds[2], 1);
            gpio_set_value(leds[3], 1);
            sleep(1);
        }
	for (int i = 0; i < 4; i++) {
            gpio_set_value(leds[i], 0);
	} 
    }

    else if (argc == 3) {
        string ledName = argv[1];   // e.g. "LED1"
        string action  = argv[2];   // "on" / "off"
        int ledIndex = ledName[3] - '1';  // LED1->0, LED2->1, LED3->2, LED4->3
	gpio_export(leds[ledIndex]);
        gpio_set_direction(leds[ledIndex], "out");

        if (ledIndex >= 0 && ledIndex < 4) {
            if (action == "on") {
                gpio_set_value(leds[ledIndex], 1);
            } else if (action == "off") {
                gpio_set_value(leds[ledIndex], 0);
            }
        }
    }

    // ❌ 不再 unexport → 這樣每顆 LED 狀態會被保留
    // for (int i = 0; i < 4; i++) {
    //     gpio_unexport(leds[i]);
    // }

    return 0;
}
