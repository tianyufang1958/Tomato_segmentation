#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <opencv2/opencv.hpp>
#include <iostream>
#include <fstream>

using namespace cv;
using namespace std;

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
    void on_pushButton_exit_clicked();

    void on_pushButton_seg_clicked();

private:
    Ui::MainWindow *ui;

private:
    vector<string> listFile(char folder_name[]);
    Mat remove_small_blob(Mat img, int size);
    Mat remove_big_blob(Mat img, int size);
    Mat remove_edge(Mat img);
    bool assess_circle(Mat img, Point center, int point, float std);
    Mat overexpose_clean(Mat overexpose, Mat img_red_thresh);
};

#endif // MAINWINDOW_H
