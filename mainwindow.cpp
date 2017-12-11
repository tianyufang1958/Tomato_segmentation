#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <dirent.h>
#include <opencv2/opencv.hpp>
#include <string>
#include <iostream>
#include <fstream>
#include <QThread>
#include <algorithm>
#include <iterator>
#include <QFileDialog>

using namespace cv;
using namespace std;

#define NUMBER_OF_TRAINING_SAMPLES 7558
#define ATTRIBUTES_PER_SAMPLE 12
#define NUMBER_OF_CLASSES 3

int idx = 0;
Mat img_rgb, img_hsv, img_lab, img_cbcr;


MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_pushButton_exit_clicked()
{
    this->close();
}

vector<string> MainWindow::listFile(char folder_name[])
{
    DIR *pDIR;
    struct dirent *entry;
    vector<string> files;
    if( pDIR=opendir(folder_name) ){
            while(entry = readdir(pDIR)){
                    if( strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0 )
                   // cout << entry->d_name << "\n";

                    files.push_back(entry->d_name);
            }
            closedir(pDIR);
    }
    return files;
}

Mat MainWindow::remove_small_blob(Mat img, int size)
{
    Mat img_thresh = img.clone();
    vector<vector<Point> > contours;
    vector<Vec4i> hierarchy;
    findContours(img_thresh, contours, hierarchy, CV_RETR_CCOMP, CV_CHAIN_APPROX_NONE);

    for (vector<vector<Point> >::iterator it = contours.begin(); it!=contours.end(); )
    {
        if ( it->size() > size)
            it=contours.erase(it);
        else
            ++it;
    }


    drawContours(img, contours, -1, Scalar(0), CV_FILLED);

    return img;
}

Mat MainWindow::remove_big_blob(Mat img, int size)
{
    Mat img_thresh = img.clone();
    vector<vector<Point> > contours;
    vector<Vec4i> hierarchy;
    findContours(img_thresh, contours, hierarchy, CV_RETR_CCOMP, CV_CHAIN_APPROX_NONE);

    for (vector<vector<Point> >::iterator it = contours.begin(); it!=contours.end(); )
    {
        if ( it->size() < size)
            it=contours.erase(it);
        else
            ++it;
    }


    drawContours(img, contours, -1, Scalar(0), CV_FILLED);

    return img;
}

Mat MainWindow::remove_edge(Mat img)
{
    for (int i = 0; i < 5; i++)
    {
        for (int j = 0; j < img.cols; j++)
        {
            if (img.at<uchar>(i,j) == 255)
            {
                img.at<uchar>(i,j) = 0;
            }
        }
    }

    for (int i = img.rows-5; i < img.rows; i++)
    {
        for (int j = 0; j < img.cols; j++)
        {
            if (img.at<uchar>(i,j) == 255)
            {
                img.at<uchar>(i,j) = 0;
            }
        }
    }

    for (int i = 0; i < img.rows; i++)
    {
        for (int j = 0; j < 5; j++)
        {
            if (img.at<uchar>(i,j) == 255)
            {
                img.at<uchar>(i,j) = 0;
            }
        }
    }

    for (int i = 0; i < img.rows; i++)
    {
        for (int j = img.cols-5; j < img.cols; j++)
        {
            if (img.at<uchar>(i,j) == 255)
            {
                img.at<uchar>(i,j) = 0;
            }
        }
    }

    return img;
}

Mat MainWindow::overexpose_clean(Mat overexpose, Mat img_red_thresh)
{
    int left, top, right;
    bool gotoMainLoop = true;
    for (int i = 0; i < img_red_thresh.rows; i++)
    {
        for (int j = 0; j < img_red_thresh.cols; j++)
        {
            if (img_red_thresh.at<uchar>(i,j) == 255 && gotoMainLoop == true)
            {
                top = i;
                gotoMainLoop = false;
            }
        }
    }

    gotoMainLoop = true;
    for (int j = 0; j < img_red_thresh.cols; j++)
    {
        for (int i = 0; i < img_red_thresh.rows; i++)
        {
            if (img_red_thresh.at<uchar>(i,j) == 255 && gotoMainLoop == true)
            {
                left = j;
                gotoMainLoop = false;
            }
        }
    }

    gotoMainLoop = true;
    for (int j = img_red_thresh.cols-1; j > 0; j--)
    {
        for (int i = 0; i < img_red_thresh.rows; i++)
        {
            if (img_red_thresh.at<uchar>(i,j) == 255 && gotoMainLoop == true)
            {
                right = j;
                gotoMainLoop = false;
            }
        }
    }

    cout<<left<<endl;

    for (int i = 0; i < top; i++)
    {
        for (int j = 0; j < overexpose.cols; j++)
        {
            if (overexpose.at<uchar>(i,j) == 255)
            {
                overexpose.at<uchar>(i,j) = 0;
            }
        }
    }

    for (int i = 0; i < overexpose.rows; i++)
    {
        for (int j = 0; j < left; j++)
        {
            if (overexpose.at<uchar>(i,j) == 255)
            {
                overexpose.at<uchar>(i,j) = 0;
            }
        }
    }

    for (int i = 0; i < overexpose.rows; i++)
    {
        for (int j = right; j < overexpose.cols; j++)
        {
            if (overexpose.at<uchar>(i,j) == 255)
            {
                overexpose.at<uchar>(i,j) = 0;
            }
        }
    }


    return overexpose;
}

bool MainWindow::assess_circle(Mat img, Point centre, int radius, float std)
{
    Size axes(radius, radius);
    vector<Point> circle_points;
    ellipse2Poly( centre, axes, 0, 0, 360, 1, circle_points );

    float temp;
    vector<float> val;

    for (int i = 0; i < circle_points.size(); i++)
    {
        temp = 0;
        for (int m = -1; m <= 1; m++)
        {
            for (int n = -1; n <= 1; n++)
            {
                temp += img.at<uchar>(circle_points[i].y+m, circle_points[i].x+n);
            }
        }
        val.push_back(temp/9);

    }


    float max = *max_element(begin(val), end(val));
    float min = *min_element(begin(val), end(val));

    double sum = accumulate(val.begin(), val.end(), 0.0);
    double mean = sum / val.size();
    double sq_sum = inner_product(val.begin(), val.end(), val.begin(), 0.0);
    double stdev = sqrt(sq_sum / val.size() - mean * mean);

    //cout<<stdev;

    if (min > 2 &&stdev < std)
    {
        return true;
    }
    else
    {
        return false;
    }
}

void MainWindow::on_pushButton_seg_clicked()
{
    QString str = QFileDialog::getExistingDirectory();
        QByteArray ba = str.toLocal8Bit();
        char *c_str = ba.data();
        string slash = "/";

        ofstream output_file;
        output_file.open("Ratio.txt");


        vector<string> img_name;

        img_name = listFile(c_str);

        for(int i = 0; i < img_name.size(); i++)                         //size of the img_name
        {

         string file_name = c_str + slash + img_name[i];

         Mat img_ori = imread(file_name);

         Mat img, img_meanshift;
         cv::resize(img_ori, img, Size(), 0.1, 0.1, INTER_LINEAR);
        // pyrMeanShiftFiltering(img, img_meanshift, 20, 30, 3);


         Mat img_no_background = img.clone();
         Mat img_no_background_thresh;

         vector<Mat> rgb;
         split(img, rgb);

         threshold(rgb[0], img_no_background_thresh, 70, 255, CV_THRESH_BINARY_INV);
         Mat img_no_background_mask = remove_small_blob(img_no_background_thresh, 150);

         img_no_background_mask = remove_edge(img_no_background_mask);


         Mat hole = img_no_background_mask.clone();
         floodFill(hole, Point(0,0), Scalar(255));
         bitwise_not(hole, hole);
         img_no_background_mask = (img_no_background_mask | hole);

         for (int i = 0; i < img.rows; i++)
         {
             for (int j = 0; j < img.cols; j++)
             {
                 if (img_no_background_mask.at<uchar>(i,j) ==0)
                 {
                     img_no_background.at<Vec3b>(i,j)[0] = 0;
                     img_no_background.at<Vec3b>(i,j)[1] = 0;
                     img_no_background.at<Vec3b>(i,j)[2] = 0;
                 }
             }
         }

         Mat img_hsv, img_lab, img_crcb;
         vector<Mat> hsv;
         vector<Mat> lab;
         vector<Mat> cbcr;


         cvtColor(img_no_background, img_hsv, CV_RGB2HSV);
         cvtColor(img_no_background, img_lab, CV_RGB2Lab);
         cvtColor(img_no_background, img_cbcr, CV_RGB2YCrCb);
         split(img_hsv, hsv);
         split(img_lab, lab);
         split(img_cbcr, cbcr);


         Mat ratio = hsv[0] / lab[2] * 100;

         Mat ratio_thresh, ratio_thresh2, overexpose;
         threshold(ratio, ratio_thresh, 150, 255, CV_THRESH_BINARY);
         threshold(ratio, ratio_thresh2, 70, 255, CV_THRESH_BINARY_INV);
         threshold(hsv[1], overexpose, 70, 255, CV_THRESH_BINARY_INV);

         Mat img_red_thresh, img_red;

         threshold(ratio, img_red_thresh, 130, 255, CV_THRESH_BINARY);

         img_red_thresh = remove_small_blob(img_red_thresh, 100);

         Mat sum = ratio_thresh + ratio_thresh2;

         inRange(hsv[1], Scalar(20), Scalar(80), overexpose);
         overexpose = remove_big_blob(overexpose, 20);


         overexpose = overexpose_clean(overexpose, img_red_thresh);


         bool fruit;
         Point centre;

         for (int i = 0; i < overexpose.rows; i++)
         {
             for (int j = 0; j < overexpose.cols; j++)
             {
                 if (overexpose.at<uchar>(i,j) == 255)
                 {
                     centre = Point(j,i);

                     fruit = assess_circle(hsv[1], centre, 10, 20);
                   //  cout<<endl;
                 }

                 if (fruit == true)
                 {
                     circle(img_no_background, centre, 10, Scalar(255, 0, 0), 1);
                 }

                 if (fruit == false)
                 {
                     overexpose.at<uchar>(i,j) = 0;

                 }
             }
         }


        /* Mat marker = watershed(hsv[1]);

         watershed(img, marker);
         Mat mark = Mat::zeros(marker.size(), CV_8UC1);
         marker.convertTo(mark, CV_8UC1);
         bitwise_not(mark, mark);
         imshow("Markers_v2", mark);*/


         //namedWindow("ImageDisplay", WINDOW_NORMAL);
         //namedWindow("hsv", WINDOW_NORMAL);

         imshow("ori", img_no_background);
         imshow("sum", overexpose);
         imshow("ImageDisplay", img_red_thresh);
         imshow("hsv", ratio);
         waitKey(0);

        }
}
