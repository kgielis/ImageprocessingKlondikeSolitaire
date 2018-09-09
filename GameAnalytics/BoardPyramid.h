#pragma once
#include "Board.h"
class BoardPyramid :
	public Board
{
public:
	BoardPyramid();
	~BoardPyramid();

	bool build(Mat & src);
	bool process(Mat & src, int xUp, int yUp, int xDown, int yDown);
	void processCardSelection(const int x, const int y);
	void calculateFinalScore();
	bool isGameComplete();

	void extractAndUpdateCard(const int row, const int index);
	std::pair<classifiers, classifiers>  extractAndClassifyCard(const int row, const int index);

	void checkRowCleared(const int row);

	int getBoardsCompleted() { return boardsCompleted; };
	int getSumErrors() { return sumErrors; };


private:
	//std::vector<int> rowCleared;
	int boardsCompleted = 0;
	int sumErrors = 0;

	//Card previousClicked;
	//int previousValue = 0;
};

