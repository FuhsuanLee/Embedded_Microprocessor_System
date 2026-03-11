#include <iostream>
#include <string>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

using namespace std;

void setGPIO(string led, string status)
{
    int io = open("/dev/demo", O_WRONLY);
    if (io < 0) {
        perror("open error");
        return;
    }

    char buf[64] = {0};

    // 組合字串成：LED1 on
    if (status == "") {
        // 讀取 LED 狀態
        string cmd = led;
        strcpy(buf, cmd.c_str());
    } else {
        // 控制 LED on/off
        string cmd = led + " " + status;
        strcpy(buf, cmd.c_str());
    }

    cout << "Send to driver: " << buf << endl;

    write(io, buf, strlen(buf));   // 寫入 /dev/demo
    close(io);                     // 關閉
}

int main(int argc, char *argv[])
{
    if (argc < 2) {
        cout << "Usage: " << argv[0] << " LEDx [on/off]" << endl;
        return 0;
    }

    string led = argv[1];
    string status = "";

    if (argc == 3)
        status = argv[2];   // on/off

    setGPIO(led, status);

    return 0;
}

