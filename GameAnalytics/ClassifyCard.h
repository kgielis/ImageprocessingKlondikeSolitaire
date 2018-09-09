#pragma once

#include "stdafx.h"

#include "Card.h"

#include "opencv2/imgproc.hpp"
#include "opencv2/highgui.hpp"
#include "opencv2/core.hpp"
#include "opencv2/opencv.hpp"
#include "opencv2/features2d.hpp"
#include "opencv2/imgcodecs.hpp"

#include <vector>
#include <utility>
#include <Windows.h>
#include <string>

using namespace cv;
using namespace std;

const int RESIZED_TYPE_WIDTH = 40;
const int RESIZED_TYPE_HEIGHT = 50;
const int MIN_CONTOUR_AREA = 80;

class ClassifyCard
{
public:
	ClassifyCard();

	// INITIALIZATION
	void generateTrainingData(const cv::Mat & trainingImage, const String & outputPreName);	// generating knn data if it doesn't exist yet
	void generateImageVector();										// initialization of the image vector used for the subtraction method

	// MAIN FUNCTIONS

	std::pair<Mat, Mat> segmentRankAndSuitFromCard(const Mat & aCard);
	std::pair<classifiers, classifiers> classifyCard(std::pair<Mat, Mat> cardCharacteristics);	// main function for classification of the rank/suit images
	std::pair<classifiers, classifiers> ClassifyCard::classifyCard(const Mat & aCard, const bool full = false);

	//UTILITY FUNCTION
	void resizeCardToStandardSize(const cv::Mat & croppedRef, cv::Mat & resizedCardImage);
	
	// CLASSIFICATION METHODS
	int classifyTypeUsingSubtraction(								// return: the index of image_list of the best match
		std::vector<std::pair<classifiers, cv::Mat>> &image_list,	// input: list of known (already classified) images
		cv::Mat &resizedROI);

	classifiers classifyTypeUsingKnn(								// return: the classified type
		const Mat & image,											// input: the unknown image
		const Ptr<ml::KNearest> & kNearest);						// input: the correct trained knn algorithm


private:
	// hardcoded values, but thanks to the resizing and consistent cardextraction, that is possible
	Rect myRankROI = Rect(4, 3, 22, 27);	
	Rect mySuitROI = Rect(4, 30, 22, 21);

	// images used for subtraction method
	vector<std::pair<classifiers, cv::Mat>> rankImages;
	vector<std::pair<classifiers, cv::Mat>> suitImages;

	// knn data
	Ptr<ml::KNearest>  kNearest_suit;
	Ptr<ml::KNearest>  kNearest_rank;
};

inline bool fileExists(const std::string& name) {
	ifstream f(name.c_str());
	return f.good();
}
