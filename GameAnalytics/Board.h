#pragma once

#include "ClassifyCard.h"
#include "ExtractCards.h"
#include "Card.h"

#include <vector>
#include <iostream>


class Board
{
public:
	Board();
	~Board();

	void setBoardImage(const Mat & src);

	void init();

	virtual bool build(Mat & src) = 0;
	virtual bool process(Mat & src, int xUp, int yUp, int xDown, int yDown) = 0;
	virtual void processCardSelection(const int x, const int y) = 0;
	virtual void calculateFinalScore() = 0;
	virtual bool isGameComplete() = 0;
	
	bool isPileClicked(const int x, const int y);

	void handleUndoState();
	void storeForUndo();

	void findChangedRegions(const Mat & src, vector<pair<int, int>> & res);

	bool updateTopCards();
	void emptyClassifiedCard(const int row, const int col);

	void drawRegions();

	bool isRowCleared(const int row); 

	virtual void printPlayingBoard();

	std::pair<classifiers, classifiers> extractAndClassifyCard(const int row, const int index, bool istop, bool isverticaltop);

	/*
	int getAbsXLeft(const int row, const int index);
	int getAbsXRight(const int row, const int index);
	int getAbsYUp(const int row, const int index);
	int getAbsYDown(const int row, const int index);
	*/

	//get functions for coordinates of region
	int getXLeft(const int row, const int index);
	int getXRight(const int row, const int index);
	int getYUp(const int row, const int index);
	int getYDown(const int row, const int index);

	//get functions for metrics
	const int getScore() { return score; }
	const int getNumberOfPilePresses() { return numberOfPilePresses; }
	const int getNumberOfPresses(const int row, const int index) { return currentPlayingBoard.at(row).at(index).numberOfPresses; }
	const std::vector<std::pair<int, int>> & getLocationOfPresses() { return locationOfPresses;  }

	void identifyClickedCard(const int x, const int y, int & row, int & index); // row and index are -1 if no region clicked

protected:
	std::vector<std::vector<Card>> currentPlayingBoard;
	std::vector<std::vector<std::vector<Card>>> previousPlayingBoards;
	std::vector<int> previousScores;

	Rect pileRegion; //pile or draw click region
	int numberOfPilePresses = 0;

	ExtractCards ec;
	ClassifyCard cc;

	int score = 0;

	//int cardWidth = 0;
	//int cardHeight = 0; 

	Mat boardImage; //resized and cropped
	//Mat origBoardImage; //original

	//to keep track of the location of the presses
	std::vector<std::pair<int, int>> locationOfPresses;
	std::pair<int, int> locationOfLastPress;

	//keep track of the previously selected card
	std::pair<classifiers, classifiers> previouslySelectedCard;	// keeping track of the card that was selected previously to detect player move errors
	int previouslySelectedIndex = -1;	// keeping track of the index of card that was selected previously to detect player move errors
	int previouslySelectedRow = -1;

};

