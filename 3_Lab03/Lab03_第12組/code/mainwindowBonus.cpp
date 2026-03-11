#include "mainwindowBonus.h"
#include "ui_mainwindowBonus.h"
#include <iostream>
#include <cstdlib>
#include <fstream>
#include <string>
#include <unistd.h>
#include <QPixmap>
#include <QTimer>
#include <QMessageBox>
#include <QTime>


using namespace std;

int isClicked[4] = {0, 0, 0, 0};
int times = 0;
int freq=1;
int isShining = 0;

char * png_off = "/home/nvidia/lab03/closed.png";
char * png_on = "/home/nvidia/lab03/open.png";
// char * png_off = "/home/windows/lab03/closed.png";
// char * png_on = "/home/windows/lab03/open.png";

const int LEDS[4] = {396, 466, 397, 398};

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

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // 初始化隨機種子
    qsrand(QTime::currentTime().msec());

    // 初始化GPIO
    for (int i = 0; i < 4; i++) {
        gpio_export(LEDS[i]);
        gpio_set_direction(LEDS[i], "out");
        gpio_set_value(LEDS[i], 0);
    }

    // 初始化UI圖片
    ui->labelon->setPixmap(QPixmap(png_off));
    ui->labelon_2->setPixmap(QPixmap(png_off));
    ui->labelon_3->setPixmap(QPixmap(png_off));
    ui->labelon_4->setPixmap(QPixmap(png_off));

    // 初始化計分板
    ui->label_2->setText("0");

    // 初始化遊戲計時器
    gameTimer = new QTimer(this);
    connect(gameTimer, &QTimer::timeout, this, &MainWindow::startNewRound);
}

MainWindow::~MainWindow()
{
    setAllLEDsOff();
    if (gameTimer) {
        gameTimer->stop();
        delete gameTimer;
    }
    if (blinkTimer) {
        blinkTimer->stop();
        delete blinkTimer;
    }
    delete ui;
}

// 舊的 checkbox 函數實現(保留以避免編譯錯誤)
void MainWindow::on_checkBox_clicked(bool checked)
{
}

void MainWindow::on_checkBox_2_clicked(bool checked)
{
}

void MainWindow::on_checkBox_3_clicked(bool checked)
{
}

void MainWindow::on_checkBox_4_clicked(bool checked)
{
}

void MainWindow::on_pushButton_clicked()
{
}

void MainWindow::on_pushButton_2_clicked()
{
}

void MainWindow::on_pushButton_3_clicked()
{
}

void MainWindow::onBlinkTick()
{
}


void MainWindow::on_progressBar_valueChanged(int value)
{
    freq = 100 - value;

    // 即時更新 QTimer 閃爍間隔
    if (blinkTimer && blinkTimer->isActive()) {
        int interval = 200 + freq * 20;
        blinkTimer->setInterval(interval);
    }
}

void MainWindow::on_pushButton_8_clicked()
{
    if (isGameRunning) {
        // 如果遊戲正在進行，停止遊戲
        endGame();
        ui->pushButton_8->setText("開始");
        QMessageBox::information(this, "遊戲結束", QString("最終得分: %1").arg(score));
        return;
    }

    // 開始新遊戲
    score = 0;
    isGameRunning = true;
    ui->label_2->setText("0");
    ui->pushButton_8->setText("停止");

    startNewRound();
}

// 開始新一輪
void MainWindow::startNewRound()
{
    if (!isGameRunning) return;

    // 檢查是否達到100分
    if (score >= 100) {
        isGameRunning = false;
        ui->pushButton_8->setText("開始");
        setAllLEDsOff();
        QMessageBox::information(this, "恭喜!", "你成功達到100分!");
        return;
    }

    setAllLEDsOff();
    currentLedIndex = qrand() % 4;
    setLED(currentLedIndex, true);
    int delay = 1000 + freq * 10;
    gameTimer->start(delay);
}

// 結束遊戲
void MainWindow::endGame()
{
    isGameRunning = false;
    if (gameTimer) {
        gameTimer->stop();
    }
    setAllLEDsOff();
    score = 0;
    ui->label_2->setText("0");
}

// 更新分數
void MainWindow::updateScore(int points)
{
    score += points;
    ui->label_2->setText(QString::number(score));
}

void MainWindow::setLED(int index, bool on)
{
    if (index < 0 || index >= 4) return;

    // 設定硬體LED
    gpio_set_value(LEDS[index], on ? 1 : 0);

    // 設定UI LED
    QLabel* labels[4] = {ui->labelon, ui->labelon_2, ui->labelon_3, ui->labelon_4};
    labels[index]->setPixmap(QPixmap(on ? png_on : png_off));
}

// 關閉所有LED
void MainWindow::setAllLEDsOff()
{
    for (int i = 0; i < 4; i++) {
        setLED(i, false);
    }
}

void MainWindow::on_pushButton_4_clicked()
{
    if (!isGameRunning) return;

    if (currentLedIndex == 0) {
        updateScore(20);
        gameTimer->stop();
        startNewRound();
    } else {
        QMessageBox::warning(this, "失敗", "按錯了!遊戲結束!");
        endGame();
        ui->pushButton_8->setText("開始");
    }
}

void MainWindow::on_pushButton_5_clicked()
{
    if (!isGameRunning) return;

    if (currentLedIndex == 1) {
        updateScore(20);
        gameTimer->stop();
        startNewRound();
    } else {
        QMessageBox::warning(this, "失敗", "按錯了!遊戲結束!");
        endGame();
        ui->pushButton_8->setText("開始");
    }
}

void MainWindow::on_pushButton_6_clicked()
{
    if (!isGameRunning) return;

    if (currentLedIndex == 2) {
        updateScore(20);
        gameTimer->stop();
        startNewRound();
    } else {
        QMessageBox::warning(this, "失敗", "按錯了!遊戲結束!");
        endGame();
        ui->pushButton_8->setText("開始");
    }
}

void MainWindow::on_pushButton_7_clicked()
{
    if (!isGameRunning) return;

    if (currentLedIndex == 3) {
        updateScore(20);
        gameTimer->stop();
        startNewRound();
    } else {
        QMessageBox::warning(this, "失敗", "按錯了!遊戲結束!");
        endGame();
        ui->pushButton_8->setText("開始");
    }
}