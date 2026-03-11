#include <iostream>
#include <fstream>
#include <string>
#include <pthread.h>
#include <unistd.h>

using namespace std;

/* ================
   GPIO 控制函式
================= */

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

/* ================
   全域變數
================= */

int Counter = 0;
pthread_mutex_t mutex;

int LED2 = 396;   // LED2 用來顯示 Critical Section 被哪個 Thread 佔用

/* ================
   Thread 函式
================= */

void* count_func(void* arg) {
    for (int i = 0; i < 10000; i++) {

        // 上鎖（Critical Section）
        pthread_mutex_lock(&mutex);

        gpio_set_value(LED2, 1);  // 進入 critical section → LED2 亮起
	if (i == 0) {
		cout << "In critical session" << endl;
		usleep(100000);
	}
	Counter++; 		// 安全的共享資源操作

        gpio_set_value(LED2, 0);  // 離開 critical section → LED2 熄滅
	if (i == 0) {
                cout << "leave critical session" << endl;
                usleep(100000);
        }
	pthread_mutex_unlock(&mutex);
    }

    pthread_exit(nullptr);
}

/* ================
   主程式
================= */

int main() {
    pthread_t t1, t2;

    // 初始化 Mutex
    pthread_mutex_init(&mutex, nullptr);

    // 初始化 LED2
    gpio_export(LED2);
    gpio_set_direction(LED2, "out");
    gpio_set_value(LED2, 0);

    // 建立兩個 Threads
    pthread_create(&t1, nullptr, count_func, nullptr);
    pthread_create(&t2, nullptr, count_func, nullptr);

    // 等待完成
    pthread_join(t1, nullptr);
    pthread_join(t2, nullptr);

    // 顯示結果
    cout << "Final Counter = " << Counter << endl;

    // 理論正確值： 2 * 10,000 = 20,000
    cout << "Expected Value = 20000" << endl;

    // 清理
    pthread_mutex_destroy(&mutex);

    return 0;
}

