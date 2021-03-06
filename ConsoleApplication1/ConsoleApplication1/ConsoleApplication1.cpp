// Program to illustrate SIFT keypoint and descriptor extraction, and matching using brute force
// Author: Samarth Manoj Brahmbhatt, University of Pennsylvania
#include "pch.h"
#include <opencv2/opencv.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/nonfree/features2d.hpp>
#include <opencv2/nonfree/nonfree.hpp>
#include <opencv2/features2d/features2d.hpp>
#include <iostream>
#include <string>
using namespace cv;
using namespace std;
Mat train, train_g, train_desc, test, test_g;
vector<KeyPoint> train_kp, temp_kp;
vector<Mat> train_desc_collection;
vector<vector<Point2f>> pointMoves;
vector<Point2f> newPoint;
SurfFeatureDetector featureDetector;
SurfDescriptorExtractor featureExtractor;
BFMatcher matcher;

void train_alg()
{
	matcher.clear();
	temp_kp = train_kp;
	featureDetector.detect(test_g, train_kp);
	if (train_kp.size() > 1)
	{
		featureExtractor.compute(test_g, train_kp, train_desc);
		if (train_desc_collection.size() != 0)
			train_desc_collection.pop_back();
		train_desc_collection.push_back(train_desc);
		matcher.add(train_desc_collection);
		matcher.train();
	}
	else
	{
		train_kp = temp_kp;
	}
	cout << "Frame trained." << endl;
}

void frameCompare()
{
	RNG& rng = theRNG();
	initModule_nonfree();
	int totalKP = 0;
	VideoCapture cap(0);
	unsigned int frame_count = 0;
	cap >> test;
	resize(test, test, cvSize(0, 0), 0.5, 0.5);
	train = test;
	cout << "ello?" << endl;
	cvtColor(test, test_g, CV_BGR2GRAY);
	train_alg();
	newPoint.push_back(Point2f(0, 0));
	newPoint.push_back(Point2f(0, 0));

	// VideoCapture object
	while (char(waitKey(1)) != 'q')
	{
		double t0 = getTickCount();
		Mat testbig;
		cap >> test;
		resize(test, test, cvSize(0, 0), 0.5, 0.5);
		//resize(testbig, test, cvSize(0, 0), 0.5, 0.5);
		if (test.empty())
			continue;
		cvtColor(test, test_g, CV_BGR2GRAY);
		//detect SURF keypoints and extract descriptors in the test image
		vector<KeyPoint> test_kp;
		test_kp.clear();
		Mat test_desc;
		featureDetector.detect(test_g, test_kp);
		featureExtractor.compute(test_g, test_kp, test_desc);
		// match train and test descriptors, getting 2 nearest neighbors for all test descriptors
		vector<vector<DMatch>> matches;
		cout << "test_kp.size() " << test_kp.size() << endl;
		// Currently throws an exception if not enough keypoints.
		matcher.knnMatch(test_desc, matches, 2);
		// filter for good matches according to Lowe's algorithm
		vector<DMatch> good_matches;
		for (int i = 0; i < matches.size(); i++)
		{
			if (matches[i][0].distance < 0.6 * matches[i][1].distance)
				good_matches.push_back(matches[i][0]);
		}
		Mat img_show;
		img_show = train;
		for (int i = 0; i < good_matches.size(); i++)
		{
			newPoint[0] = test_kp[good_matches[i].queryIdx].pt;
			newPoint[1] = train_kp[good_matches[i].trainIdx].pt - newPoint[0];
			//pointMoves.push_back(newPoint);
			int drawMult = 1 << 4;
			Point rounded0 = Point(cvRound(newPoint[0].x * drawMult), cvRound(newPoint[0].y * drawMult));
			Point rounded1 = Point(cvRound((newPoint[0].x + newPoint[1].x) * drawMult), cvRound((newPoint[0].y + newPoint[1].y) * drawMult));
			Scalar color = Scalar(rng(256), rng(256), rng(256));
			circle(img_show, rounded0, 10, color, 1, CV_AA, 4);
			line(img_show, rounded0, rounded1, color, 1, CV_AA, 4);
		}
		// drawMatches(test, test_kp, train, train_kp, good_matches, img_show);
		imshow("Matches", img_show);
		cout << "Frame rate = " << getTickFrequency() / (getTickCount() - t0) << endl;
		train = test;
		train_alg();
		//pointMoves.clear();
	}

}

Mat deskew(Mat& img)
{
	Mat img_g, img_b;
	double SV = 1.5;
	cvtColor(img, img_g, CV_BGR2GRAY);
	int total = 0;
	for (int i = 0; i < img_g.rows; i++)
	{
		for (int j = 0; j < img_g.cols; j++)
		{
			total += img_g.at<unsigned char>(i, j);
		}
	}
	double avg = total / img_g.total();
	avg *= .9;
	threshold(img_g, img_b, avg, 255, THRESH_BINARY);
	Moments m = moments(img_b);
	if (abs(m.mu02) < 1e-2)
	{
		// No deskewing needed. 
		cout << "No Deskew" << endl;
		return img_b;
	}
	// Calculate skew based on central momemts. 
	double skew = m.mu11 / m.mu02;
	// Calculate affine transform to correct skewness. 
	Mat warpMat = (Mat_<double>(2, 3) << 1, skew, -0.5, 0, 1, 0);

	Mat imgOut = Mat::zeros(img.rows, img.cols, img.type());
	warpAffine(img_g, imgOut, warpMat, imgOut.size());
	cout << skew << endl;
	return imgOut;
}

void deSkewTest()
{
	VideoCapture cap(0);
	Mat test;
	while (char(waitKey(1)) != 'q')
	{
		cap >> test;
		imshow("Camera", deskew(test));
	}
}

void HistTest()
{
	Mat src, hsv;
	src = imread("template.jpg");
	cvtColor(src, hsv, CV_BGR2HSV);

	// Quantize the hue to 30 levels
	// and the saturation to 32 levels
	int hbins = 30, sbins = 32;
	int histSize[] = { hbins, sbins };
	// hue varies from 0 to 179, see cvtColor
	float hranges[] = { 0, 180 };
	// saturation varies from 0 (black-gray-white) to
	// 255 (pure spectrum color)
	float sranges[] = { 0, 256 };
	const float* ranges[] = { hranges, sranges };
	MatND hist;
	// we compute the histogram from the 0-th and 1-st channels
	int channels[] = { 0, 1 };

	calcHist(&hsv, 1, channels, Mat(), // do not use mask
		hist, 2, histSize, ranges,
		true, // the histogram is uniform
		false);
	double maxVal = 0;
	minMaxLoc(hist, 0, &maxVal, 0, 0);

	int scale = 10;
	Mat histImg = Mat::zeros(sbins*scale, hbins * 10, CV_8UC3);

	for (int h = 0; h < hbins; h++)
		for (int s = 0; s < sbins; s++)
		{
			float binVal = hist.at<float>(h, s);
			int intensity = cvRound(binVal * 255 / maxVal);
			rectangle(histImg, Point(h*scale, s*scale),
				Point((h + 1)*scale - 1, (s + 1)*scale - 1),
				Scalar::all(intensity),
				CV_FILLED);
		}

	namedWindow("Source", 1);
	imshow("Source", src);

	namedWindow("H-S Histogram", 1);
	imshow("H-S Histogram", histImg);
	waitKey();
}

int main()
{
	frameCompare();
	return 0;
}