#ifndef MAINWINDOW_H
#define MAINWINDOW_H


#include <QMainWindow>


namespace Ui {
class MainWindow;
}


class MainWindow : public QMainWindow
{
   Q_OBJECT


public:
   explicit MainWindow(QWidget *parent = 0);
   ~MainWindow();


private slots:
   void on_checkBox_clicked(bool checked);


   void on_checkBox_2_clicked(bool checked);


   void on_checkBox_3_clicked(bool checked);


   void on_checkBox_4_clicked(bool checked);


   void on_pushButton_clicked();


   void on_pushButton_2_clicked();


   void onBlinkTick();


   void on_progressBar_valueChanged(int value);