#include <iostream>
#include <fstream>
#include <string>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>

using namespace std;

/* ======================
   GPIO 控制函式
====================== */
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

/* ======================
   全域變數
====================== */
long long result_T1 = 0;
long long result_T2 = 0;

sem_t sem_done; // 用來通知 Monitor Thread 停止閃爍
bool workers_finished = false;

int LED_HEARTBEAT = 396;   // LED1 (Heartbeat)

/* ======================
   Worker Thread 函式
====================== */
void* worker_func(void* arg) {
    usleep(5000000);
    long start = ((long*)arg)[0];
    long end   = ((long*)arg)[1];
    long long sum = 0;

    long count = 0;
    for (long i = start; i <= end; i++)
        sum += i;

    long thread_id = start == 1 ? 1 : 2;

    if (thread_id == 1) result_T1 = sum;
    else result_T2 = sum;

    pthread_exit(nullptr);
}

/* ======================
   Monitor Thread 函式
====================== */
void* monitor_func(void*) {
    gpio_export(LED_HEARTBEAT);
    gpio_set_direction(LED_HEARTBEAT, "out");

    while (true) {
        int ret = sem_trywait(&sem_done);

        if (ret == 0) break;  // 收到通知 → 停止閃爍

        gpio_set_value(LED_HEARTBEAT, 1);
        usleep(500000); // 0.5 秒
        gpio_set_value(LED_HEARTBEAT, 0);
        usleep(500000);
    }

    // 完成後設為常亮
    gpio_set_value(LED_HEARTBEAT, 1);

    pthread_exit(nullptr);
}

/* ======================
   主程式
====================== */
int main() {
    pthread_t t1, t2, t3;
    sem_init(&sem_done, 0, 0); // 初始值 0 → 代表未完成

    // 參數範圍
    long range1[2] = {1, 25000000};
    long range2[2] = {25000001, 50000000};

    // 啟動 Monitor Thread
    pthread_create(&t3, nullptr, monitor_func, nullptr);

    // 啟動 Worker Threads
    pthread_create(&t1, nullptr, worker_func, range1);
    pthread_create(&t2, nullptr, worker_func, range2);

    // 等待 T1、T2 完成
    pthread_join(t1, nullptr);
    pthread_join(t2, nullptr);

    // 通知 T3 停止閃爍
    sem_post(&sem_done);

    // 等待 T3 結束
    pthread_join(t3, nullptr);

    // 彙總結果
    long long total = result_T1 + result_T2;
    cout << "Sum of 1 to 50,000,000 = " << total << endl;

    sem_destroy(&sem_done);
    return 0;
}

