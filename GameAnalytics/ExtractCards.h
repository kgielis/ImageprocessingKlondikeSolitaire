#pragma once

#include "pch.h"
#include "opencv2/imgproc.hpp"
#include "opencv2/highgui.hpp"
#include "opencv2/core.hpp"
#include "opencv2/opencv.hpp"
#include "opencv2/features2d.hpp"
#include "opencv2/imgcodecs.hpp"

#include <vector>
#include <iostream>
#include <string>

#include "Card.h"

#define standardBoardWidth 1600
#define standardBoardHeight 900

using namespace std;
using namespace cv;

class ExtractCards
{
public:
	ExtractCards();
	~ExtractCards();

	// INITIALIZATION
	void determineROI(const Mat & boardImage);	// find the general location (roi) that contains all cards during the initialization
	void calculateOuterRect(std::vector<std::vector<cv::Point>> &contours);	// calculate the outer rect (roi) using the contours of all cards

	// MAIN FUNCTIONS
	//const std::vector<cv::Mat> & findCardsFromBoardImage(Mat const & boardImage);
	//const std::vector<cv::Mat> & findCardsFromBoardImage(Mat const & boardImage, const int index1, const int index2);	// main extraction function
	void resizeBoardImage(Mat const & boardImage, Mat & resizedBoardImage, const bool setblackborder);	// resize the playing board to standardBoardWidth/Height
	void cropBoardImageROI(Mat const & src, Mat & resizedImage);

	//void extractCardRegions(const cv::Mat & src);	// extract each region that contains a card
	//void extractCardRegions(const cv::Mat & src, const int index);
	//void extractCards();	// extract the top card from a card region
	//void extractCards(const int index);
	void identifyCardRegions(const cv::Mat &src, std::vector<Rect> & listrect, std::vector<bool> & iscardcomplete, const bool highthreshold = true);
	void identifyCardRegionsRightFromRectangle(const cv::Mat &src, const cv::Rect & rect, std::vector<Rect> & listrect, std::vector<bool> & iscardcomplete);
	bool identifySquare(const cv::Mat & src, cv::Rect & rect);

	// PROCESSING OF STACK CARDS WITHIN extractCards()
	void extractTopCard(Mat & src, Mat& dest, bool isVertical = true);
	//void extractTopCardUsingSobel(const cv::Mat & src, cv::Mat & dest, int i);	// extract the top card from a stack of cards using Sobel edge detection
	void extractTopCardUsingSobel(const cv::Mat & src, cv::Mat & dest, bool isVertical = true);	// extract the top card from a stack of cards using Sobel edge detection
	void extractTopCardUsingAspectRatio(const cv::Mat & src, cv::Mat & dest);	// extract the top card from a stack of cards using the standard size of a card

	//void croppedTopCardToStandardSize(const cv::Mat &croppedRef, cv::Mat &resizedCardImage);	// resize the extracted top card to standardCardWidth/Height 
	//void croppedCardToStandardSize(const cv::Mat &croppedRef, cv::Mat &resizedCardImage);

	// PROCESSING OF SELECTED CARDS BY THE PLAYER
	//int getIndexOfSelectedCard(int i);	// find the index of the deepest card that was selected by the player (topcard = 0, below top card = 1, etc.)
	int getIndexOfSelectedCard(Mat & src);

	// END OF GAME CHECK
	bool checkForOutOfMovesState(const cv::Mat &src);	// check if the middle of the screen is mostly white == out of moves screen

	// SIMPLE GET FUNCTIONS
	Rect getROI() { return ROI; };
	int getTopCardsHeight() { return topCardsHeight; };

private:
	//std::vector<cv::Mat> cards;	// extracted cards from card regions
	//std::vector<cv::Mat> cardRegions;	// extracted card regions
	Rect ROI;	// roi calculated by determineROI during the initialization that contains the general region of all cards
	int topCardsHeight;	// separation of the talon and suit stack from the build stack within the ROI

	Rect ROIBlackBorder; //roi to remove the black border
};
