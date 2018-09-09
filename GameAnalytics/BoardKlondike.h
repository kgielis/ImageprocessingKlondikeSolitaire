#pragma once
#include "Board.h"
class BoardKlondike :
	public Board
{
public:
	BoardKlondike();
	~BoardKlondike();

	bool build(Mat & src);
	bool process(Mat & src, int xUp, int yUp, int xDown, int yDown);
	void calculateFinalScore();
	bool isGameComplete();	
	
	void printPlayingBoard();

	std::pair<classifiers, classifiers> extractAndClassifyCard(const int row, const int index);
	void extractAndClassifyAllCards();

	void extractAndClassifyChangedRegions(Mat & src);
	void extractAndClassifyImpactedRegions(int xUp, int yUp, int xDown, int yDown);

	bool updateBoard();

	void findChangedCardLocations(int & changedRow1, int & changedIndex1, int & changedRow2, int & changedIndex2);
	bool cardMoveBetweenBuildAndSuitStack(const int changedRow1, const int changedIndex1, const int changedRow2, const int changedIndex2);
	void processCardSelection(const int x, const int y);
	void detectPlayerMoveErrors(std::pair<classifiers, classifiers>& selectedCard, const int rowOfPressedCardLocation, const int indexOfPressedCardLocation);
	//int determineIndexOfPressedCard(const int x, const int y);

	//get functions for the metrics
	const int getNumberOfSuitErrors() { return numberOfSuitErrors; }
	const int getNumberOfRankErrors() { return numberOfRankErrors; }


private :
	// Klondike specific metrics
	int numberOfSuitErrors = 0;
	int numberOfRankErrors = 0;
};

