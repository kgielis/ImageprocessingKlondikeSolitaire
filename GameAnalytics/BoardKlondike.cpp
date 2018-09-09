#include "stdafx.h"
#include "BoardKlondike.h"

BoardKlondike::BoardKlondike()
{

	previouslySelectedCard.first = EMPTY;
	previouslySelectedCard.second = EMPTY;
}


BoardKlondike::~BoardKlondike()
{
}

bool BoardKlondike::build(Mat & src) {

	std::cout << "Building board Klondike" << std::endl;
	ec.determineROI(src);
	setBoardImage(src);

	//initialize variables
	init();
	numberOfSuitErrors = 0;
	numberOfRankErrors = 0;

	currentPlayingBoard.resize(2);
	currentPlayingBoard.at(0).resize(5);
	currentPlayingBoard.at(1).resize(7);

	//std::cout << "Extract Card regions" << std::endl;
	cv::Size srcSize = boardImage.size();
	// split the topcards (talon and suit stack) and the bottom cards (the build stack)
	Rect top = Rect(0, 0, (int)srcSize.width, ec.getTopCardsHeight() );	// take the top of the screen until topCardsHeight (calculated in calculateOuterRect)
	Rect bottom = Rect(0, ec.getTopCardsHeight(), (int)srcSize.width, (int)(srcSize.height - ec.getTopCardsHeight() - 1));	// take the rest of the image as bottomcards

	//BUILD STACK
	for (int i = 0; i < 7; i++)	// build stack: split in 7 equidistance regions of cards
	{
		currentPlayingBoard.at(1).at(i).region = Rect(bottom.x + (int)(bottom.width / 7.1 * i), bottom.y, (int)(bottom.width / 7 - 1), bottom.height);
	}

	//TALON
	currentPlayingBoard.at(0).at(0).region = Rect(top.x + top.width / 7, top.y, top.width / 5, top.height);	// talon: slightly bigger width than other cards, take a bigger margin

	//SUIT STACK
	for (int i = 3; i < 7; i++)	// suit stack: take the last 4 equidistance regions of cards
	{
		currentPlayingBoard.at(0).at(i-2).region = Rect(top.x + (int)(top.width / 7.1 * i), top.y, (int)(top.width / 7 - 1), top.height);
	}

	//PILE region
	pileRegion = Rect(0, 0, top.width / 7 , top.height);

	//initialize Klondike board
	int remaining = 0;
	bool flagknowncard = true;
	bool validate = true;
	for (int i = 0; i < currentPlayingBoard.size(); i++) {
		for (int j = 0; j < currentPlayingBoard.at(i).size(); j++) {
			std::pair<classifiers, classifiers> c = extractAndClassifyCard(i, j);
			//talon
			if (i == 0 && j == 0) {
				remaining = 24;
				flagknowncard = false;
			}
			else {
				flagknowncard = true;
				if (i == 1 && j < 7) {
					remaining = j;
					//cards are not empty at new game
					if (c.first == EMPTY) validate = false;
				}
				else {
					remaining = 0;
					//cards are empty at new game
					if (c.first != EMPTY) validate = false;
				}
			}
			//all card regions are set playable for Kondike
			currentPlayingBoard.at(i).at(j).initValues(i, j, true, remaining, c, flagknowncard);
		}
	}

	//push board to list of previous boards for undo
	storeForUndo();

	return validate;

	//drawRegions();
}

std::pair<classifiers, classifiers> BoardKlondike::extractAndClassifyCard(const int row, const int index) {
	bool istop = true;
	bool isverticaltop = true;
	if (row == 0 && index == 0) isverticaltop = false; //talon
	return (Board::extractAndClassifyCard(row, index, istop, isverticaltop));
}

void BoardKlondike::extractAndClassifyAllCards() {
	for (int i = 0; i < currentPlayingBoard.size(); i++) {
		for (int j = 0; j < currentPlayingBoard.at(i).size(); j++) {
			extractAndClassifyCard(i, j);
		}
	}
}

void BoardKlondike::extractAndClassifyChangedRegions(Mat & src) {
	//first find the changed regions based on comparing the new image with current image
	//this function will also set the board image
	vector <pair<int, int>> changedregions;
	findChangedRegions(src, changedregions);

	//extract and classify the cards for the changed regions
	for (int i = 0; i < changedregions.size(); i++) {
		int row = changedregions.at(i).first;
		int index = changedregions.at(i).second;
		cout << "Extract card for changed region : " << row << " - " << index << std::endl;
		extractAndClassifyCard(row, index);
	}
}


void BoardKlondike::extractAndClassifyImpactedRegions(int xUp, int yUp, int xDown, int yDown) {
	vector <pair<int, int>> impactedRegions;
	if (isPileClicked(xUp, yUp)) {
		//extract talon card
		pair<int, int> p(0, 0);
		impactedRegions.push_back(p);
	}
	else {
		int rowup, indexup, rowdown, indexdown;
		identifyClickedCard(xUp, yUp, rowup, indexup);
		if (rowup != -1) {
			pair<int, int> pu(rowup, indexup);
			impactedRegions.push_back(pu);
		}
		if (xDown != -1) {
			identifyClickedCard(xDown, yDown, rowdown, indexdown);
			if (rowdown != -1) {
				pair<int, int> pd(rowdown, indexdown);
				impactedRegions.push_back(pd);
			}
			if (indexdown == indexup && rowup == rowdown && previouslySelectedRow != -1) {
				//not drag/drop between regions - add the previous one
				pair<int, int> pp(previouslySelectedRow, previouslySelectedIndex);
				impactedRegions.push_back(pp);
			}
		} else {
			//double-click - add the suit regions
			pair<int, int> s1(0, 1);
			impactedRegions.push_back(s1);
			pair<int, int> s2(0, 2);
			impactedRegions.push_back(s2);
			pair<int, int> s3(0, 3);
			impactedRegions.push_back(s3);
			pair<int, int> s4(0, 4);
			impactedRegions.push_back(s4);
		}
	}

	//sort the impacted regions
	std::sort(impactedRegions.begin(), impactedRegions.end(), [](const pair<int,int>& c1, const pair<int,int>& c2) -> bool { return (7*c1.first + c2.second  < 7*c2.first + c2.second); });
	std::vector<pair<int,int>>::iterator it;
	it = std::unique(impactedRegions.begin(), impactedRegions.end());
	impactedRegions.resize(std::distance(impactedRegions.begin(), it));

	for (int i = 0; i < impactedRegions.size(); i++) {
		int row = impactedRegions.at(i).first;
		int index = impactedRegions.at(i).second;
		cout << "Extract card for impacted region : " << row << " - " << index << std::endl;
		extractAndClassifyCard(row, index);
	}
}


bool BoardKlondike::process(Mat & src, int xUp, int yUp, int xDown, int yDown) {
	
	//std::cout << "Processing board Klondike" << std::endl;
	bool boardchange = false;
	//for (int k = 0; k < 1000; k++) {
		//method 1 : changed regions based on difference - board image is set in the function
		extractAndClassifyChangedRegions(src);

		//method 2 : impacted regions based on mouse clicks
		//setBoardImage(src);
		//extractAndClassifyImpactedRegions(xUp, yUp, xDown, yDown);

		//method 3 : all regions 
		//setBoardImage(src);
		//extractAndClassifyAllCards();

		//if changes, push board to previousPlayingBoards
		boardchange = updateBoard();
		if (boardchange) {
			storeForUndo();
			//std::cout << "Change of board" << std::endl;
			printPlayingBoard();
		}
		else {
			std::cout << "No change of board" <<  std::endl;
		}
	//}

	return (boardchange);
}


bool BoardKlondike::updateBoard() 
{
	int changedIndex1 = -1, changedIndex2 = -1;
	int changedRow1 = -1, changedRow2 = -1;
	findChangedCardLocations(changedRow1, changedIndex1, changedRow2, changedIndex2); // check which card locations have changed, this is maximum 2 (move from loc1 to loc2, or click on deck)
	if (changedRow1 == -1 && changedIndex1 == -1 && changedRow2 == -1 && changedIndex2 == -1 )
	{
		return false; // no locations changed, no update needed
	}

	// pile pressed (only talon changed)
	if (changedIndex1 == 0 && changedRow1 == 0 && changedRow2 == -1 && changedIndex2 == -1)
	{
		currentPlayingBoard.at(0).at(0).topCard = currentPlayingBoard.at(0).at(0).classifiedCard;
		//printPlayingBoard();
		return true;
	}

	// card move from talon to board (stack or suit)
	if ((changedIndex1 == 0 && changedRow1 == 0) || (changedIndex2 == 0 && changedRow2 == 0))
	{
		int tempRow, tempIndex;	// contains the other row and index
		(changedIndex1 == 0 && changedRow1 == 0) ? (tempRow = changedRow2) : (tempRow = changedRow1);
		(changedIndex1 == 0 && changedRow1 == 0) ? (tempIndex = changedIndex2) : (tempIndex = changedIndex1);

		currentPlayingBoard.at(tempRow).at(tempIndex).knownCards.push_back(currentPlayingBoard.at(0).at(0).topCard);	// add the card to the new location
		currentPlayingBoard.at(0).at(0).topCard = currentPlayingBoard.at(0).at(0).classifiedCard;	// update the card at the talon
		--currentPlayingBoard.at(0).at(0).remainingCards;
		//printPlayingBoard();
		--currentPlayingBoard.at(0).at(0).remainingCards;
		++currentPlayingBoard.at(tempRow).at(tempIndex).numberOfPresses;	// update the number of presses on that row and index
		(tempRow == 1) ? score += 5 : score += 10;	// update score
		return true;
	}

	// card move between build stack and suit stack
	if (changedRow1 != -1 && changedIndex1 != -1 && changedRow2 != -1 && changedIndex2 != -1)
	{
		if (cardMoveBetweenBuildAndSuitStack(changedRow1, changedIndex1, changedRow2, changedIndex2))	// true if changed board
		{
			//printPlayingBoard();
			return true;
		}
	}

	// error with previously detected card
	if (changedRow1 != -1 && changedIndex1 != -1 && changedIndex1 != 0 && changedRow1 != 0 && changedRow2 == -1 && changedIndex2 == -1)	// one card location changed which wasn't a talon change
	{
		if (currentPlayingBoard.at(changedRow1).at(changedIndex1).knownCards.empty())	// animation of previous state ended too fast, a card was still turning over which was missed
																		// -> no known cards in the list, but there is a card at that location: update knownCards
		{
			if (currentPlayingBoard.at(changedRow1).at(changedIndex1).remainingCards > 0)
			{
				--currentPlayingBoard.at(changedRow1).at(changedIndex1).remainingCards;
			}
			currentPlayingBoard.at(changedRow1).at(changedIndex1).knownCards.push_back(currentPlayingBoard.at(changedRow1).at(changedIndex1).classifiedCard);
			//printPlayingBoard();
			score += 5;
			return true;
		}
		return false;
	}
	
	return false;
}


void BoardKlondike::findChangedCardLocations(int & changedRow1, int & changedIndex1, int & changedRow2, int & changedIndex2)
{
	for (int i = 0; i < currentPlayingBoard.size(); i++) {
		for (int j = 0; j < currentPlayingBoard.at(i).size(); j++)
		{
			if (i == 0 && j == 0)	// card is different from the topcard of the talon AND the card isn't empty
			{
				if (currentPlayingBoard.at(0).at(0).classifiedCard != currentPlayingBoard.at(0).at(0).topCard)
				{
					changedIndex1 == -1 ? (changedIndex1 = 0) : (changedIndex2 = 0);
					changedRow1 == -1 ? (changedRow1 = 0) : (changedRow2 = 0);
				}
			}
			else if (currentPlayingBoard.at(i).at(j).knownCards.empty())	// adding card to an empty location
			{
				if (currentPlayingBoard.at(i).at(j).classifiedCard.first != EMPTY)	// new card isn't empty
				{
					changedRow1 == -1 ? (changedRow1 = i) : (changedRow2 = i);
					changedIndex1 == -1 ? (changedIndex1 = j) : (changedIndex2 = j);
				}
			}
			else
			{
				if (currentPlayingBoard.at(i).at(j).knownCards.back() != currentPlayingBoard.at(i).at(j).classifiedCard )	// classified topcard is different from the previous topcard
				{
					changedRow1 == -1 ? (changedRow1 = i) : (changedRow2 = i);
					changedIndex1 == -1 ? (changedIndex1 = j) : (changedIndex2 = j);
				}
			}
			if (changedIndex2 != -1) { return; }	// if 2 changed location were detected, return
		}
	}
}


bool BoardKlondike::cardMoveBetweenBuildAndSuitStack(const int changedRow1, const int changedIndex1, const int changedRow2, const int changedIndex2)
{
	// 1. CURRENT VISIBLE CARD WAS ALREADY IN THE LIST OF KNOWN CARDS -> ONE OR MORE TOPCARDS WERE MOVED TO A OTHER LOCATION
	auto inList1 = std::find(currentPlayingBoard.at(changedRow1).at(changedIndex1).knownCards.begin(), currentPlayingBoard.at(changedRow1).at(changedIndex1).knownCards.end(), currentPlayingBoard.at(changedRow1).at(changedIndex1).classifiedCard);
	auto inList2 = std::find(currentPlayingBoard.at(changedRow2).at(changedIndex2).knownCards.begin(), currentPlayingBoard.at(changedRow2).at(changedIndex2).knownCards.end(), currentPlayingBoard.at(changedRow2).at(changedIndex2).classifiedCard);

	// 1.1 The card was in the list of the card at index "changedIndex2"
	//		-> remove all cards after this index (iterator++) from knowncards at index "changedIndex2" and add them to the knowncards at index "changedIndex1"
	if (inList1 != currentPlayingBoard.at(changedRow1).at(changedIndex1).knownCards.end() && inList2 == currentPlayingBoard.at(changedRow2).at(changedIndex2).knownCards.end())
	{
		inList1++;
		currentPlayingBoard.at(changedRow2).at(changedIndex2).knownCards.insert(
			currentPlayingBoard.at(changedRow2).at(changedIndex2).knownCards.end(),
			inList1,
			currentPlayingBoard.at(changedRow1).at(changedIndex1).knownCards.end());
		currentPlayingBoard.at(changedRow1).at(changedIndex1).knownCards.erase(
			inList1,
			currentPlayingBoard.at(changedRow1).at(changedIndex1).knownCards.end());

		// update the score
		if (changedRow2 == 0 && changedRow1 == 1)	// build to suit stack
		{
			score += 10;
		}
		else if (changedRow1 == 0 && changedRow2 == 1)	// suit to build stack
		{
			score -= 10;
		}
		++currentPlayingBoard.at(changedRow2).at(changedIndex2).numberOfPresses;
		return true;
	}

	// 1.2 The card was in the list of the card at index "changedIndex1"
	//		-> remove all cards after this index (iterator++) from knowncards at index "changedIndex1"  and add them to the knowncards at index "changedIndex2"
	if (inList1 == currentPlayingBoard.at(changedRow1).at(changedIndex1).knownCards.end() && inList2 != currentPlayingBoard.at(changedRow2).at(changedIndex2).knownCards.end())
	{
		inList2++;
		currentPlayingBoard.at(changedRow1).at(changedIndex1).knownCards.insert(
			currentPlayingBoard.at(changedRow1).at(changedIndex1).knownCards.end(),
			inList2,
			currentPlayingBoard.at(changedRow2).at(changedIndex2).knownCards.end());
		currentPlayingBoard.at(changedRow2).at(changedIndex2).knownCards.erase(
			inList2,
			currentPlayingBoard.at(changedRow2).at(changedIndex2).knownCards.end());

		// update the score
		if (changedRow1 == 0 && changedRow2 == 1)	// build to suit stack
		{
			score += 10;
		}
		else if (changedRow2 == 0 && changedRow1 == 1)	// suit to build stack
		{
			score -= 10;
		}
		++currentPlayingBoard.at(changedRow1).at(changedIndex1).numberOfPresses;
		return true;
	}

	// 2. CURRENT VISIBLE CARD WAS NOT IN THE LIST OF KNOWN CARDS -> ALL TOPCARDS WERE MOVED TO A OTHER LOCATION AND A NEW CARD TURNED OVER
	//		-> check if the current topcard was in the list of the other cardlocation, if so, all cards of the other cardlocation were moved to this location

	if (inList1 == currentPlayingBoard.at(changedRow1).at(changedIndex1).knownCards.end() && inList2 == currentPlayingBoard.at(changedRow2).at(changedIndex2).knownCards.end())
	{
		auto inList1 = std::find(currentPlayingBoard.at(changedRow1).at(changedIndex1).knownCards.begin(), currentPlayingBoard.at(changedRow1).at(changedIndex1).knownCards.end(), currentPlayingBoard.at(changedRow2).at(changedIndex2).classifiedCard);
		auto inList2 = std::find(currentPlayingBoard.at(changedRow2).at(changedIndex2).knownCards.begin(), currentPlayingBoard.at(changedRow2).at(changedIndex2).knownCards.end(), currentPlayingBoard.at(changedRow1).at(changedIndex1).classifiedCard);

		// 2.1 The topcard at index "changedIndex2" was in the list of the card at index "changedIndex1"
		//		-> all cards from "changedIndex1" were moved to "changedIndex2" AND "changedIndex1" got a new card
		//		-> remove all knowncards "changedIndex1" and add them to the knowncards at "changedIndex2" AND add the new card to "changedIndex1"
		if (inList1 != currentPlayingBoard.at(changedRow1).at(changedIndex1).knownCards.end() && inList2 == currentPlayingBoard.at(changedRow2).at(changedIndex2).knownCards.end())
		{
			currentPlayingBoard.at(changedRow2).at(changedIndex2).knownCards.insert(
				currentPlayingBoard.at(changedRow2).at(changedIndex2).knownCards.end(),
				currentPlayingBoard.at(changedRow1).at(changedIndex1).knownCards.begin(),
				currentPlayingBoard.at(changedRow1).at(changedIndex1).knownCards.end());
			currentPlayingBoard.at(changedRow1).at(changedIndex1).knownCards.clear();
			if (currentPlayingBoard.at(changedRow1).at(changedIndex1).classifiedCard.first != EMPTY)
			{
				currentPlayingBoard.at(changedRow1).at(changedIndex1).knownCards.push_back(currentPlayingBoard.at(changedRow1).at(changedIndex1).classifiedCard);
				score += 5;
				--currentPlayingBoard.at(changedRow1).at(changedIndex1).remainingCards;
			}

			// update the score
			if (changedRow2 == 0 && changedRow1 == 1)	// build to suit stack
			{
				score += 10;
			}
			else if (changedRow1 == 0 && changedRow2 == 1)	// suit to build stack
			{
				score -= 10;
			}
			++currentPlayingBoard.at(changedRow2).at(changedIndex2).numberOfPresses;
			return true;
		}

		// 2.2 The topcard at index "changedIndex1" was in the list of the card at index "changedIndex2"
		//		-> all cards from "changedIndex2" were moved to "changedIndex1" AND "changedIndex2" got a new card
		//		-> remove all knowncards "changedIndex2" and add them to the knowncards at "changedIndex1" AND add the new card to "changedIndex2"
		if (inList1 == currentPlayingBoard.at(changedRow1).at(changedIndex1).knownCards.end() && inList2 != currentPlayingBoard.at(changedRow2).at(changedIndex2).knownCards.end())
		{
			currentPlayingBoard.at(changedRow1).at(changedIndex1).knownCards.insert(
				currentPlayingBoard.at(changedRow1).at(changedIndex1).knownCards.end(),
				currentPlayingBoard.at(changedRow2).at(changedIndex2).knownCards.begin(),
				currentPlayingBoard.at(changedRow2).at(changedIndex2).knownCards.end());
			currentPlayingBoard.at(changedRow2).at(changedIndex2).knownCards.clear();
			if (currentPlayingBoard.at(changedRow2).at(changedIndex2).classifiedCard.first != EMPTY)
			{
				currentPlayingBoard.at(changedRow2).at(changedIndex2).knownCards.push_back(currentPlayingBoard.at(changedRow2).at(changedIndex2).classifiedCard);
				score += 5;
				--currentPlayingBoard.at(changedRow2).at(changedIndex2).remainingCards;
			}

			// update the score
			if (changedRow1 == 0 && changedRow2 == 1)	// build to suit stack
			{
				score += 10;
			}
			else if (changedRow2 == 0 && changedRow1 == 1)	// suit to build stack
			{
				score -= 10;
			}
			++currentPlayingBoard.at(changedRow1).at(changedIndex1).numberOfPresses;
			return true;
		}
	}
	return false;
}


void BoardKlondike::processCardSelection(const int x, const int y)
{
	
	int row, indexOfPressedCardLocation;
	identifyClickedCard(x, y, row, indexOfPressedCardLocation);

	if (row != -1 && indexOfPressedCardLocation != -1)
	{
		//NEEDS TO BE MODIFIED
		Mat crop = boardImage(currentPlayingBoard.at(row).at(indexOfPressedCardLocation).region);
		int indexOfPressedCard = ec.getIndexOfSelectedCard(crop);	// check which card(s) on that index have been selected using a blue filter
																						//cout << "INDEX OF PRESSED CARD : " << indexOfPressedCard << std::endl; 
		if (indexOfPressedCard != -1)													//	-> returns an index from bottom->top of how many cards have been selected
		{
			std::pair<classifiers, classifiers> selectedCard;
			if (indexOfPressedCardLocation != 0 && row != 0)
			{
				int index = (int)currentPlayingBoard.at(row).at(indexOfPressedCardLocation).knownCards.size() - indexOfPressedCard - 1;	// indexOfPressedCard is from bot->top, knownCards is from top->bot - remap index
				if (index >= currentPlayingBoard.at(row).at(indexOfPressedCardLocation).knownCards.size()) {
					cout << "UNEXPECTED ERROR WITH ROW - INDEX OF PRESSED CARD - OUT OF RANGE OF KNOWN CARDS : " << row << " - " << indexOfPressedCardLocation << " - " << indexOfPressedCard << std::endl;
					index = 0;
				}
				selectedCard = currentPlayingBoard.at(row).at(indexOfPressedCardLocation).knownCards.at(index);	// check which card has been selected
			}
			else
			{
				selectedCard = currentPlayingBoard.at(row).at(indexOfPressedCardLocation).topCard;
			}
			detectPlayerMoveErrors(selectedCard, row, indexOfPressedCardLocation);	// detection of wrong card placement using previous and current selected card

			previouslySelectedCard = selectedCard;	// update previously detected card
			previouslySelectedIndex = indexOfPressedCardLocation;
			previouslySelectedRow = row;

			// +++metrics+++
			if (indexOfPressedCard == 0)	// topcard has been pressed
			{
				locationOfPresses.push_back(locationOfLastPress);	// add the coordinate to the vector of all presses for visualisation in the end
			}
			++currentPlayingBoard.at(row).at(indexOfPressedCardLocation).numberOfPresses;	// increment the pressed card location
		}
		else	// no card has been selected (just a click on the screen below the card)
		{
			previouslySelectedCard.first = EMPTY;
			previouslySelectedCard.second = EMPTY;
			previouslySelectedIndex = -1;
			previouslySelectedRow = -1;
		}

		// +++metrics+++
		if (row == 0)	// the talon and suit stack are all 'topcards' and can immediately be added to locationOfPresses
		{
			locationOfPresses.push_back(locationOfLastPress);
		}
	}

	//std::cout << "Previously selected card row : " << previouslySelectedRow << std::endl;
	//std::cout << "Previously selected card index : " << previouslySelectedIndex << std::endl;

}

void BoardKlondike::detectPlayerMoveErrors(std::pair<classifiers, classifiers> &selectedCard, const int rowOfPressedCardLocation, const int indexOfPressedCardLocation)
{
	if (previouslySelectedCard.first != EMPTY && previouslySelectedCard != selectedCard)	// current selected card is different from the previous, and there was a card previously selected
	{
		char prevSuit = static_cast<char>(previouslySelectedCard.second);
		char newSuit = static_cast<char>(selectedCard.second);
		char prevRank = static_cast<char>(previouslySelectedCard.first);
		char newRank = static_cast<char>(selectedCard.first);

		if (rowOfPressedCardLocation == 0  && ( previouslySelectedRow == 1 || (previouslySelectedRow == 0 && previouslySelectedIndex == 0) ) )	// suit stack
		{
			if ((prevSuit == 'H' && newSuit != 'H') || (prevSuit == 'D' && newSuit != 'D')	// check for suit error
				|| (prevSuit == 'S' && newSuit != 'S') || (prevSuit == 'C' || newSuit != 'C'))
			{
				//std::cout << "Incompatible suit! " << prevSuit << " can't go on " << newSuit << " to build the suit stack" << std::endl;
				++numberOfSuitErrors;
			}
			if ((prevRank == '2' && newRank != 'A') || (prevRank == '3' && newRank != '2') || (prevRank == '4' && newRank != '3')	// check for number error 
				|| (prevRank == '5' && newRank != '4') || (prevRank == '6' && newRank != '5') || (prevRank == '7' && newRank != '6')
				|| (prevRank == '8' && newRank != '7') || (prevRank == '9' && newRank != '8') || (prevRank == ':' && newRank != '9')
				|| (prevRank == 'J' && newRank != ':') || (prevRank == 'Q' && newRank != 'J') || (prevRank == 'K' && newRank != 'Q'))
			{
				//std::cout << "Incompatible rank! " << prevRank << " can't go on " << newRank << std::endl;
				++numberOfRankErrors;
			}
		}
		else	// build stack
		{
			if (((prevSuit == 'H' || prevSuit == 'D') && (newSuit == 'H' || newSuit == 'D'))	// check for color error
				|| ((prevSuit == 'S' || prevSuit == 'C') && (newSuit == 'S' || newSuit == 'C')))
			{
				//std::cout << "Incompatible suit! " << prevSuit << " can't go on " << newSuit << " to build the build stack" << std::endl;
				++numberOfSuitErrors;
			}
			if ((prevRank == 'A' && newRank != '2') || (prevRank == '2' && newRank != '3') || (prevRank == '3' && newRank != '4')	// check for number error 
				|| (prevRank == '4' && newRank != '5') || (prevRank == '5' && newRank != '6') || (prevRank == '6' && newRank != '7')
				|| (prevRank == '7' && newRank != '8') || (prevRank == '8' && newRank != '9') || (prevRank == '9' && newRank != ':')
				|| (prevRank == ':' && newRank != 'J') || (prevRank == 'J' && newRank != 'Q') || (prevRank == 'Q' && newRank != 'K')
				|| (prevRank == 'K' && newRank != ' '))
			{
				//std::cout << "Incompatible rank! " << prevRank << " can't go on " << newRank << std::endl;
				++numberOfRankErrors;
			}
		}
	}
}


void BoardKlondike::calculateFinalScore()
{
	int remainingCards = 0;
	for (int i = 0; i < 7; ++i)
	{
		remainingCards += (int)currentPlayingBoard.at(1).at(i).knownCards.size();
	}
	score += (remainingCards * 10);
}


void BoardKlondike::printPlayingBoard()
{
	std::cout << "Printing the playing board of Klondike Solitaire: " << std::endl;
	std::cout << "Score: " << score << std::endl;	// print the score

	std::cout << "Talon: ";	// print the current topcard from deck
	if (currentPlayingBoard.at(0).at(0).topCard.first == EMPTY)
	{
		std::cout << "// ";
	}
	else
	{
		std::cout << static_cast<char>(currentPlayingBoard.at(0).at(0).topCard.first) << static_cast<char>(currentPlayingBoard.at(0).at(0).topCard.second);
	}
	std::cout << "	Remaining: " << currentPlayingBoard.at(0).at(0).remainingCards << std::endl;

	std::cout << "Suit stack: " << std::endl;		// print all cards from the suit stack
	for (int i = 1; i < currentPlayingBoard.at(0).size(); i++)
	{
		std::cout << "   Pos " << i - 1 << ": ";
		if (currentPlayingBoard.at(0).at(i).knownCards.empty())
		{
			std::cout << "// ";
		}
		for (int j = 0; j < currentPlayingBoard.at(0).at(i).knownCards.size(); j++)
		{
			std::cout << static_cast<char>(currentPlayingBoard.at(0).at(i).knownCards.at(j).first) << static_cast<char>(currentPlayingBoard.at(0).at(i).knownCards.at(j).second) << " ";
		}
		std::cout << std::endl;
	}

	std::cout << "Build stack: " << std::endl;		// print all cards from the build stack
	for (int i = 0; i < currentPlayingBoard.at(1).size(); i++)
	{
		std::cout << "   Pos " << i << ": ";
		if (currentPlayingBoard.at(1).at(i).knownCards.empty())
		{
			std::cout << "// ";
		}
		for (int j = 0; j < currentPlayingBoard.at(1).at(i).knownCards.size(); j++)
		{
			std::cout << static_cast<char>(currentPlayingBoard.at(1).at(i).knownCards.at(j).first) << static_cast<char>(currentPlayingBoard.at(1).at(i).knownCards.at(j).second) << " ";
		}
		std::cout << "	Hidden cards = " << currentPlayingBoard.at(1).at(i).remainingCards << std::endl;
	}
}

bool BoardKlondike::isGameComplete() {
	int cardsLeft = currentPlayingBoard.at(0).at(0).remainingCards;
	for (int i = 0; i < 7; i++)
	{
		if (currentPlayingBoard.at(1).at(i).remainingCards > 0)
		{
			++cardsLeft;
		}
	}
	if (cardsLeft > 0) return false; else return true;
}