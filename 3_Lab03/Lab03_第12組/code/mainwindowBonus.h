#ifndef MAINWINDOWBONUS_H
#define MAINWINDOWBONUS_H

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

    void on_pushButton_3_clicked();

    void on_pushButton_8_clicked();

    void on_pushButton_4_clicked();

    void on_pushButton_5_clicked();

    void on_pushButton_6_clicked();

    void on_pushButton_7_clicked();

private:
    Ui::MainWindow *ui;
    QTimer *blinkTimer = nullptr;
    QTimer *gameTimer = nullptr;
    int blinkStep = 0;
    int remaining = 0;
    int currentLedIndex = -1;
    int score = 0;
    bool isGameRunning = false;

    void startNewRound();
    void endGame();
    void updateScore(int points);
    void setLED(int index, bool on);
    void setAllLEDsOff();
};

#endif // MAINWINDOWBONUS_H