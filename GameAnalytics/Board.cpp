#include "stdafx.h"
#include "Board.h"

Board::Board() : ec(), cc()
{
}


Board::~Board()
{
}

void Board::setBoardImage(const Mat & src) {
	ec.cropBoardImageROI(src, boardImage);
}

void Board::init() {
	previousPlayingBoards.clear(); 
	previousScores.clear();

	numberOfPilePresses = 0;

	score = 0;

	locationOfLastPress.first = -1;
	locationOfLastPress.second = -1;
	locationOfPresses.clear();

	previouslySelectedCard.first = EMPTY;
	previouslySelectedCard.second = EMPTY;
	previouslySelectedIndex = -1;
	previouslySelectedRow = -1;
}


void Board::printPlayingBoard() {

	std::cout << "Printing the playing board of Solitaire: " << std::endl;

	std::cout << "Score: " << score << std::endl;	// print the score
	for (int i = 0; i < currentPlayingBoard.size(); i++) {

		std::vector<Card>& row = currentPlayingBoard.at(i);

		std::cout << "Card of row " << i << ": ";
		for (int j = 0; j < row.size(); j++) {

			std::cout << static_cast<char>(row.at(j).topCard.first) << static_cast<char>(row.at(j).topCard.second);
			if (row.at(j).playable) std::cout << "*"; else std::cout << " ";
			std::cout << " Index:" << row.at(j).index << "; ";
		}
		std::cout << std::endl;
	}
}



std::pair<classifiers, classifiers> Board::extractAndClassifyCard(const int row, const int index, bool istop, bool isverticaltop) {

	//cout << "In function extractAndClassifyCard" << endl;
	std::pair<classifiers, classifiers> cardType; cardType.first = UNKNOWN; cardType.second = UNKNOWN;
	
	//check range
	if (row < 0 || row >= currentPlayingBoard.size()) {
		cout << "Row out of range" << endl;
		return cardType;
	}
	else if (index < 0 || index >= currentPlayingBoard.at(row).size()) {
		cout << "Index out of range" << endl;
		return cardType;
	}

	Card tempCard = currentPlayingBoard.at(row).at(index);
	Rect rect = tempCard.region;

	Mat crop = boardImage(rect);
	//imshow("Card image", crop);
	//waitKey();

	if (istop) {
		Mat dest;
		ec.extractTopCard(crop, dest, isverticaltop);
		cardType = cc.classifyCard(dest, false); // TO INVESTIGATE : full card classification gives sometimes ? for Klondike
	} else {
		cardType = cc.classifyCard(crop, false);
	}

	currentPlayingBoard.at(row).at(index).classifiedCard = cardType;

	//std::cout << "Cardtype: " << static_cast<char>(cardType.first) << static_cast<char>(cardType.second) << std::endl;

	return cardType;
}

/*
int Board::getAbsXLeft(const int row, const int index) {
	return(ec.getROI().x + currentPlayingBoard.at(row).at(index).region.x);
}

int Board::getAbsXRight(const int row, const int index) {
	return(ec.getROI().x + currentPlayingBoard.at(row).at(index).region.x + currentPlayingBoard.at(row).at(index).region.width);
}

int Board::getAbsYUp(const int row, const int index) {
	return(ec.getROI().y + currentPlayingBoard.at(row).at(index).region.y);
}

int Board::getAbsYDown(const int row, const int index) {
	return(ec.getROI().y + currentPlayingBoard.at(row).at(index).region.y + currentPlayingBoard.at(row).at(index).region.height);
}
*/

int Board::getXLeft(const int row, const int index) {
	return(currentPlayingBoard.at(row).at(index).region.x);
}

int Board::getXRight(const int row, const int index) {
	return(currentPlayingBoard.at(row).at(index).region.x + currentPlayingBoard.at(row).at(index).region.width);
}

int Board::getYUp(const int row, const int index) {
	return(currentPlayingBoard.at(row).at(index).region.y);
}

int Board::getYDown(const int row, const int index) {
	return(currentPlayingBoard.at(row).at(index).region.y + currentPlayingBoard.at(row).at(index).region.height);
}


void Board::identifyClickedCard(const int x, const int y, int & row, int & index) {
	row = -1;
	index = -1;
	
	// x and y are full screen coordinates
	int xr = x - ec.getROI().x;
	int yr = y - ec.getROI().y;

	for (int i = 0; i < currentPlayingBoard.size(); i++) {

		for (int j = 0; j < currentPlayingBoard.at(i).size(); j++) {

			Card curCard = currentPlayingBoard.at(i).at(j);
			if (xr > getXLeft(i,j) && xr < getXRight(i,j) && yr > getYUp(i,j)  && yr < getYDown(i,j) && curCard.playable) {
				//std::cout << "Clicked on card with row: " << curCard.row << " and index: " << curCard.index << std::endl;
				row = i;
				index = j;

				locationOfLastPress.first = xr - currentPlayingBoard.at(row).at(index).region.x;
				locationOfLastPress.second = yr - currentPlayingBoard.at(row).at(index).region.y;

				return;
			}
		}
	}
	//std::cout << "No playable card region clicked" << std::endl;
	//not found
	return;
}


bool Board::isPileClicked(const int x, const int y) {
	//x and y are full screen coordinates
	int xr = x - ec.getROI().x;
	int yr = y - ec.getROI().y;

	bool b = false;
	if (xr > pileRegion.x && xr < pileRegion.x + pileRegion.width && yr > pileRegion.y && yr < pileRegion.y + pileRegion.height) b = true;
	
	if (b == false) return false;
	
	std::cout << "Clicked on pile" << std::endl;
	++numberOfPilePresses;
	locationOfLastPress.first = x - pileRegion.x;
	locationOfLastPress.second = y - pileRegion.y;
	locationOfPresses.push_back(locationOfLastPress);

	return true;
}

void Board::storeForUndo() {
	previousScores.push_back(score);
	previousPlayingBoards.push_back(currentPlayingBoard);
}


void Board::handleUndoState()
{
	//std::cout << "Undo board" << std::endl;
	if (previousPlayingBoards.size() > 1)	// at least one move has been played in the game
	{
		previousPlayingBoards.pop_back();	// take the previous board state as the current board state
		currentPlayingBoard = previousPlayingBoards.back();
		
		previousScores.pop_back();
		score = previousScores.back();
	}
	printPlayingBoard();
}

//note : this function will also set the board image
void Board::findChangedRegions(const Mat& src, vector<pair<int,int>> & res) {

	//imshow("image", src);
	//waitKey();

	//imshow("image", src);
	//waitKey();

	//imshow("image", origBoardImage);
	//waitKey();

	//crop and resize the new image
	Mat previousBoardImage = boardImage;
	setBoardImage(src);

	//difference
	//Mat dif = origBoardImage - src; //this does not work well - use absdiff instead
	Mat dif;
	//absdiff(src, origBoardImage, dif);
	absdiff(previousBoardImage, boardImage, dif);
	//imshow("image", dif);
	//waitKey();

	//Mat crop;
	//ec.cropBoardImageROI(dif, crop);
	//imshow("resized image", crop);
	//waitKey();

	//Mat dif = boardImage - crop;
	//imshow("resized difference", dif);
	//waitKey();

	//gray
	Mat gray;
	cvtColor(dif, gray, COLOR_BGR2GRAY);	// convert the image to gray
	//imshow("difference grey", gray);
	//waitKey();

	//threshold
	Mat thresh;
	cv::threshold(gray, thresh, 1, 255, THRESH_BINARY);
	for (int i = 0; i < currentPlayingBoard.size(); i++) {
		for (int j = 0; j < currentPlayingBoard.at(i).size(); j++) {
			Rect r = currentPlayingBoard.at(i).at(j).region;
			rectangle(thresh, r, Scalar(255, 0, 0), 1, 8, 0);
		}
	}
	//imshow("difference threshold", thresh);
	//waitKey();

	//contours
	//vector<vector<Point>> contours;
	//vector<Vec4i> hierarchy;
	//cv::findContours(thresh, contours, hierarchy, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE, Point(0, 0));	// find all the contours using the thresholded image
	//cout << "Number of difference contours : " << contours.size() << std::endl;

	//for time measurement
	//pair<int, int> a(0, 0);
	//pair<int, int> b(0, 1);
	//pair<int, int> c(0, 2);
	//pair<int, int> d(0, 3);

	//res.push_back(a);
	//res.push_back(b);
	//res.push_back(c);
	//res.push_back(d);

	//find which regions are touched by the contours
	for (int i = 0; i < currentPlayingBoard.size(); i++) {
		for (int j = 0; j < currentPlayingBoard.at(i).size(); j++) {
			Mat cropregion(gray, currentPlayingBoard.at(i).at(j).region);
			int count = countNonZero(cropregion);
			//cout << "Count non zero : " << count << std::endl;
			Size imageSize = cropregion.size();
			float perc = 100*(float)count / (float)(imageSize.height * imageSize.width);
			//cout << "Changed percentage : " << perc << "%" << std::endl;
			pair<int, int> p(i, j);
			//threshold of 2%
			if (perc > 2) {
				res.push_back(p);
				//imshow("difference region", cropregion);
				//waitKey();
			}
		}
	}
}


bool Board::updateTopCards() {
	int nr = 0;
	for (int i = 0; i < currentPlayingBoard.size(); i++) {
		for (int j = 0; j < currentPlayingBoard.at(i).size(); j++) {
			if (currentPlayingBoard.at(i).at(j).topCard != currentPlayingBoard.at(i).at(j).classifiedCard) {
				currentPlayingBoard.at(i).at(j).topCard = currentPlayingBoard.at(i).at(j).classifiedCard;
				nr++;
			}
		}
	}
	if (nr > 0) return true; else return false;
}

void Board::emptyClassifiedCard(const int row, const int index) {
	currentPlayingBoard.at(row).at(index).classifiedCard.first = EMPTY;
	currentPlayingBoard.at(row).at(index).classifiedCard.second = EMPTY;
}

void Board::drawRegions() {
	Mat src = boardImage.clone();
	for (int i = 0; i < currentPlayingBoard.size(); i++) {
		for (int j = 0; j < currentPlayingBoard.at(i).size(); j++) {
			Rect r = currentPlayingBoard.at(i).at(j).region;
				rectangle(src, r, Scalar(255, 0, 0), 1, 8, 0);
		}
	}
	imshow("Regions", src);
	waitKey();
}


bool Board::isRowCleared(const int row) {
	for (int j = 0; j < currentPlayingBoard.at(row).size(); j++) {
		if (currentPlayingBoard.at(row).at(j).topCard.first != EMPTY) return false;
	}
	return true;
}



