#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <iostream>
#include <fstream>
#include <string>
#include <unistd.h>
#include <QPixmap>
#include <QTimer>


using namespace std;


int isClicked[4] = {0, 0, 0, 0};
int times = 0;
int freq=1;
int isShining = 0;


char * png_off = "/home/nvidia/lab03/closed.png";
char * png_on = "/home/nvidia/lab03/open.png";
// char * png_off = "/home/windows/Documents/lab03/closed.png";
// char * png_on = "/home/windows/Documents/lab03/open.png";


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
   int leds[4] = {396, 466, 397 ,398};
   for (int i = 0; i < 4; i++) {
       gpio_export(leds[i]);
       gpio_set_direction(leds[i], "out");
   }
   ui->setupUi(this);
   ui->labelon->setPixmap(QPixmap(png_off));
   ui->labelon_2->setPixmap(QPixmap(png_off));
   ui->labelon_3->setPixmap(QPixmap(png_off));
   ui->labelon_4->setPixmap(QPixmap(png_off));
}


MainWindow::~MainWindow()
{
   delete ui;
}


void MainWindow::on_checkBox_clicked(bool checked)
{
   isClicked[0] = checked;
}


void MainWindow::on_checkBox_2_clicked(bool checked)
{
   isClicked[1] = checked;
}


void MainWindow::on_checkBox_3_clicked(bool checked)
{
   isClicked[2] = checked;
}


void MainWindow::on_checkBox_4_clicked(bool checked)
{
   isClicked[3] = checked;
}


void MainWindow::on_pushButton_clicked()
{
   int leds[4] = {396, 466, 397 ,398};
   for (int i = 0; i < 4; i++){
       gpio_set_value(leds[i], isClicked[i]);
   }
   if (isClicked[0]) ui->labelon->setPixmap(QPixmap(png_on));
   else ui->labelon->setPixmap(QPixmap(png_off));


   if (isClicked[1]) ui->labelon_2->setPixmap(QPixmap(png_on));
   else ui->labelon_2->setPixmap(QPixmap(png_off));


   if (isClicked[2]) ui->labelon_3->setPixmap(QPixmap(png_on));
   else ui->labelon_3->setPixmap(QPixmap(png_off));


   if (isClicked[3]) ui->labelon_4->setPixmap(QPixmap(png_on));
   else ui->labelon_4->setPixmap(QPixmap(png_off));
}


void MainWindow::on_pushButton_2_clicked()
{


   int leds[4] = {396, 466, 397 ,398};
   for (int i = 0; i < 4; i++) {
       gpio_export(leds[i]);
       gpio_set_direction(leds[i], "out");
       gpio_set_value(leds[i], 0);
   }


   isShining = 1;
   blinkStep = 0;


   if (!blinkTimer) {
       blinkTimer = new QTimer(this);
       connect(blinkTimer, &QTimer::timeout, this, &MainWindow::onBlinkTick);
   }
   blinkTimer->start(100+freq*50);
}


void MainWindow::onBlinkTick()
{
   int leds[4] = {396, 466, 397 ,398};
   if (isShining == 0) {
       // 收尾：全部關掉
       for (int i = 0; i < 4; i++) gpio_set_value(leds[i], 0);
       ui->labelon->setPixmap(QPixmap(png_off));
       ui->labelon_2->setPixmap(QPixmap(png_off));
       ui->labelon_3->setPixmap(QPixmap(png_off));
       ui->labelon_4->setPixmap(QPixmap(png_off));
       blinkTimer->stop();
       return;
   }


   if (blinkStep == 0) {
       gpio_set_value(leds[0], 1); ui->labelon->setPixmap(QPixmap(png_on));
       gpio_set_value(leds[1], 1); ui->labelon_2->setPixmap(QPixmap(png_on));
       gpio_set_value(leds[2], 0); ui->labelon_3->setPixmap(QPixmap(png_off));
       gpio_set_value(leds[3], 0); ui->labelon_4->setPixmap(QPixmap(png_off));
       blinkStep = 1;
   } else {
       gpio_set_value(leds[0], 0); ui->labelon->setPixmap(QPixmap(png_off));
       gpio_set_value(leds[1], 0); ui->labelon_2->setPixmap(QPixmap(png_off));
       gpio_set_value(leds[2], 1); ui->labelon_3->setPixmap(QPixmap(png_on));
       gpio_set_value(leds[3], 1); ui->labelon_4->setPixmap(QPixmap(png_on));
       blinkStep = 0;
   }
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


void MainWindow::on_pushButton_3_clicked()
{
   isShining = 0;
}