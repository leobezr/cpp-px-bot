#pragma once
#include <iostream>
#include <vector>
#include <memory>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>

class Observer
{
public:
	virtual void update(const cv::Mat scene) = 0;
	virtual ~Observer() = default;
};

