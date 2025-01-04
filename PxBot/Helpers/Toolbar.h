#pragma once
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/imgcodecs.hpp>
#include <iostream>
#include <windows.h>

using namespace cv;
using namespace std;

class Toolbar
{
public:
	int hue_min = 0;
	int hue_max = 179;
	int sat_min = 0;
	int sat_max = 255;
	int val_min = 0;
	int val_max = 255;

	Toolbar()
	{
		namedWindow("Trackbars", (500, 200));
		createTrackbar("Hue Min", "Trackbars", &hue_min, 179);
		createTrackbar("Hue Max", "Trackbars", &hue_max, 179);
		createTrackbar("Sat Min", "Trackbars", &sat_min, 255);
		createTrackbar("Sat Max", "Trackbars", &sat_max, 255);
		createTrackbar("Val Min", "Trackbars", &val_min, 255);
		createTrackbar("Val Max", "Trackbars", &val_max, 255);
	}

	void color_trackbar(Mat scene)
	{
		Scalar lower(hue_min, sat_min, val_min);
		Scalar upper(hue_max, sat_max, val_max);
		Mat mask;

		cvtColor(scene, scene, COLOR_BGR2HSV);
		inRange(scene, lower, upper, mask);

		cout << "Lower: (" << lower[0] << "," << lower[1] << "," << lower[2] << ")" << endl;
		cout << "Upper: (" << upper[0] << "," << upper[1] << "," << upper[2] << ")" << endl;
			
		imshow("Mask", mask);
		waitKey(1);
	}
};

