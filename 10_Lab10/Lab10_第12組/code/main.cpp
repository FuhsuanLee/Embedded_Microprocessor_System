#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <fstream>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <cstdio>
#include <arpa/inet.h>
#include <sys/socket.h>

using namespace std;

#define WEB_PORT 8080

// --- 全域變數與資源保護 ---
int g_adc_value = 0;
pthread_mutex_t count_mutex;
long long g_count = 0;
pthread_mutex_t g_mutex;

pthread_mutex_t led_mutex;
bool g_led = false;

const int THRESHOLD = 500;
const int TEST_LED = 398;

int g_working = 0;

// --- GPIO 工具函式 ---
bool file_exists(const string &path) {
    ifstream f(path);
    return f.good();
}

void gpio_export(int pin) {
    if (!file_exists("/sys/class/gpio/gpio" + to_string(pin))) {
        ofstream exportFile("/sys/class/gpio/export");
        if (exportFile.is_open()) { exportFile << pin; exportFile.close(); }
    }
}

void gpio_set_direction(int pin, const string &dir) {
    string path = "/sys/class/gpio/gpio" + to_string(pin) + "/direction";
    ofstream dirFile(path);
    if (dirFile.is_open()) { dirFile << dir; dirFile.close(); }
}

void gpio_set_value(int pin, int value) {
    string path = "/sys/class/gpio/gpio" + to_string(pin) + "/value";
    ofstream valFile(path);
    if (valFile.is_open()) { valFile << value; valFile.close(); }
}

void calculation() {
    pthread_mutex_lock(&count_mutex);
    for (int i = 0; i < 2500000; i++) {
        g_count++;
    }
    pthread_mutex_unlock(&count_mutex);
}

// --- Thread A: 感測節點 ---
void* threadA_sensor(void* arg) {
    char buffer[128];
    while (true) {
        FILE* pipe = popen("python3 get_adc.py", "r");
        if (pipe) {
            if (fgets(buffer, sizeof(buffer), pipe) != NULL) {
                int val = stoi(string(buffer));
                pthread_mutex_lock(&g_mutex);
                g_adc_value = val;
                pthread_mutex_unlock(&g_mutex);
            }
            pclose(pipe);
        }
        usleep(100000); // 0.1s
    }
    return NULL;
}

// --- Thread B: 運算與控制節點 ---
void* threadB_worker(void* arg) {
    gpio_export(TEST_LED);
    gpio_set_direction(TEST_LED, "out");

    while (true) {
        int current_adc;

        pthread_mutex_lock(&count_mutex);
        long long count = g_count;
        pthread_mutex_unlock(&count_mutex);
        if (count > 50000000) {
            pthread_mutex_lock(&g_mutex);
            g_working = 2;
            pthread_mutex_unlock(&g_mutex);
            continue;
        }

        pthread_mutex_lock(&g_mutex);
        current_adc = g_adc_value;
        pthread_mutex_unlock(&g_mutex);

        if (current_adc < THRESHOLD) {
            gpio_set_value(TEST_LED, 1);
            pthread_mutex_lock(&led_mutex); g_led = true; pthread_mutex_unlock(&led_mutex);
            usleep(500000);
            pthread_mutex_lock(&g_mutex);
            if (g_working == 1) calculation();
            pthread_mutex_unlock(&g_mutex);
            gpio_set_value(TEST_LED, 0);
            pthread_mutex_lock(&led_mutex); g_led = false; pthread_mutex_unlock(&led_mutex);
            usleep(500000);
            pthread_mutex_lock(&g_mutex);
            if (g_working == 1) calculation();
            pthread_mutex_unlock(&g_mutex);
        } else {
            gpio_set_value(TEST_LED, 1);
            pthread_mutex_lock(&led_mutex); g_led = true; pthread_mutex_unlock(&led_mutex);
            usleep(2000000);
            gpio_set_value(TEST_LED, 0);
            pthread_mutex_lock(&led_mutex); g_led = false; pthread_mutex_unlock(&led_mutex);
            usleep(2000000);
        }
    }
    return NULL;
}

// --- Thread C: Web Server ---
void* threadC_worker(void* arg) {
    int server_fd, client_fd;
    struct sockaddr_in addr;
    char buffer[4096];

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) { perror("socket"); pthread_exit(NULL); }

    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(WEB_PORT);

    bind(server_fd, (struct sockaddr*)&addr, sizeof(addr));
    listen(server_fd, 5);

    printf("[Web] Server at http://localhost:%d\n", WEB_PORT);

    while (1) {
        client_fd = accept(server_fd, NULL, NULL);
        if (client_fd < 0) continue;

        int adc, work, led;
        long long count;

        // 讀取共用變數
        pthread_mutex_lock(&g_mutex);
        adc = g_adc_value;
        work = g_working;
        pthread_mutex_unlock(&g_mutex);

        pthread_mutex_lock(&count_mutex);
        count = g_count;
        pthread_mutex_unlock(&count_mutex);

        pthread_mutex_lock(&led_mutex);
        led = g_led;
        pthread_mutex_unlock(&led_mutex);

        // 讀取請求
        memset(buffer, 0, sizeof(buffer));
        read(client_fd, buffer, sizeof(buffer)-1);

        // 處理按鈕切換
        if (strstr(buffer, "GET /?toggle=1")) {
            pthread_mutex_lock(&g_mutex);
            g_working = !g_working;
            pthread_mutex_unlock(&g_mutex);

            const char* redirect_response =
                "HTTP/1.1 302 Found\r\n"
                "Location: /\r\n"
                "Content-Length: 0\r\n"
                "\r\n";
            write(client_fd, redirect_response, strlen(redirect_response));
            close(client_fd);
            continue;
        }

        // 處理 AJAX /status
        if (strstr(buffer, "GET /status")) {
            char json[256];
            sprintf(json,
                "{\"adc\": %d, \"led\": %d, \"work\": %d, \"count\": %lld}",
                adc, led, work, count
            );

            char http_response[512];
            sprintf(http_response,
                "HTTP/1.1 200 OK\r\n"
                "Content-Type: application/json\r\n"
                "Content-Length: %lu\r\n"
                "\r\n"
                "%s",
                strlen(json), json
            );
            write(client_fd, http_response, strlen(http_response));
            close(client_fd);
            continue;
        }

        // 產生 HTML
        char html[4096];
        sprintf(html,
            "<!DOCTYPE html>"
            "<html lang='zh-Hant'>"
            "<head>"
            "<meta charset='UTF-8'>"
            "<title>智慧環境運算監控系統</title>"
            "<style>"
                "body { font-family: Arial, sans-serif; background-color: #f0f8ff; text-align: center; }"
                "h1 { color: #333; }"
                "p { font-size: 1.2em; }"
                ".status { font-weight: bold; color: %s; }"
                "button { padding: 10px 20px; font-size: 1em; background-color: #4CAF50; color: white; border: none; border-radius: 5px; cursor: pointer; }"
                "button:hover { background-color: #45a049; }"
                ".container { margin-top: 50px; }"
            "</style>"
            "<script>"
            "function updateStatus() {"
            "  fetch('/status').then(r => r.json()).then(data => {"
            "    document.getElementById('adc').innerText = data.adc;"
            "    document.getElementById('led').innerText = data.led ? 'ON' : 'OFF';"
            "    const statusMap = {0: '待機', 1: '運算中', 2: '完成'};"
            "    document.getElementById('work').innerText = statusMap[data.work] || '未知';"
            "    document.getElementById('count').innerText = data.count;"
            "  });"
            "}"
            "setInterval(updateStatus, 80);"
            "</script>"
            "</head>"
            "<body>"
            "<div class='container'>"
                "<h1>智慧環境運算監控系統</h1>"
                "<p>亮度 ADC：<span id='adc'>%d</span></p>"
                "<p>LED 狀態：<span id='led'>%s</span></p>"
                "<p>運算狀態：<span id='work'>%s</span></p>"
                "<p>運算結果：<span id='count'>%lld</span></p>"
                "<form method='get'>"
                    "<button name='toggle' value='1' type='submit'>切換狀態</button>"
                "</form>"
            "</div>"
            "</body>"
            "</html>",
            work ? "#008000" : "#ff0000",
            adc,
            led ? "ON" : "OFF",
            work == 2 ? "完成" : work == 1 ? "運算中" : "待機",
            count
        );

        char http_response[8192];
        sprintf(http_response,
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: text/html\r\n\r\n"
            "%s", html
        );

        write(client_fd, http_response, strlen(http_response));
        close(client_fd);
    }
}

int main() {
    pthread_mutex_init(&g_mutex, NULL);
    pthread_mutex_init(&count_mutex, NULL);
    pthread_mutex_init(&led_mutex, NULL);

    pthread_t tidA, tidB, tidC;
    pthread_create(&tidA, NULL, threadA_sensor, NULL);
    pthread_create(&tidB, NULL, threadB_worker, NULL);
    pthread_create(&tidC, NULL, threadC_worker, NULL);

    printf("系統已啟動...\n");

    pthread_join(tidA, NULL);
    pthread_join(tidB, NULL);
    pthread_join(tidC, NULL);

    pthread_mutex_destroy(&g_mutex);
    pthread_mutex_destroy(&count_mutex);
    pthread_mutex_destroy(&led_mutex);

    return 0;
}
