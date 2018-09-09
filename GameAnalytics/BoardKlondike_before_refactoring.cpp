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


	currentPlayingBoard.resize(1);
	currentPlayingBoard.at(0).resize(12);

	//std::cout << "Extract Card regions" << std::endl;
	cv::Size srcSize = boardImage.size();
	// split the topcards (talon and suit stack) and the bottom cards (the build stack)
	Rect top = Rect(0, 0, (int)srcSize.width, ec.getTopCardsHeight() );	// take the top of the screen until topCardsHeight (calculated in calculateOuterRect)
	Rect bottom = Rect(0, ec.getTopCardsHeight(), (int)srcSize.width, (int)(srcSize.height - ec.getTopCardsHeight() - 1));	// take the rest of the image as bottomcards

	//BUILD STACK
	for (int i = 0; i < 7; i++)	// build stack: split in 7 equidistance regions of cards
	{
		currentPlayingBoard.at(0).at(i).region = Rect(bottom.x + (int)(bottom.width / 7.1 * i), bottom.y, (int)(bottom.width / 7 - 1), bottom.height);
	}

	//TALON
	currentPlayingBoard.at(0).at(7).region = Rect(top.x + top.width / 7, top.y, top.width / 5, top.height);	// talon: slightly bigger width than other cards, take a bigger margin

	//SUIT STACK
	for (int i = 3; i < 7; i++)	// suit stack: take the last 4 equidistance regions of cards
	{
		currentPlayingBoard.at(0).at(i+5).region = Rect(top.x + (int)(top.width / 7.1 * i), top.y, (int)(top.width / 7 - 1), top.height);
	}

	//PILE region
	pileRegion = Rect(0, 0, top.width / 7 , top.height);

	//initialize Klondike board
	int remaining = 0;
	bool flagknowncard = true;
	bool validate = true;
	for (int j = 0; j < currentPlayingBoard.at(0).size(); j++) {
		std::pair<classifiers, classifiers> c = extractAndClassifyCard(j);
		if (j == 7) {
			remaining = 24;
			flagknowncard = false;
		} else {
			flagknowncard = true;
			if (j < 7) {
				remaining = j;
				//cards are not empty at new game
				if (c.first == EMPTY) validate = false;
			} else {
				remaining = 0;
				//cards are empty at new game
				if (c.first != EMPTY) validate = false;
			}
		}
		//all card regions are set playable for Kondike
		currentPlayingBoard.at(0).at(j).initValues(0, j, true, remaining, c, flagknowncard);
	}


	//push board to list of previous boards for undo
	previousPlayingBoards.push_back(currentPlayingBoard);

	return validate;

	//drawRegions();
}

std::pair<classifiers, classifiers> BoardKlondike::extractAndClassifyCard(const int index) {
	bool istop = true;
	bool isverticaltop = true;
	if (index == 7) isverticaltop = false; //talon
	return (Board::extractAndClassifyCard(0, index, istop, isverticaltop));
}

void BoardKlondike::extractAndClassifyAllCards() {
	for (int j = 0; j < currentPlayingBoard.at(0).size(); j++) {
		extractAndClassifyCard(j);
	}
}

void BoardKlondike::extractAndClassifyChangedRegions(Mat & src) {
	//first find the changed regions based on comparing the new image with current image
	//this function will also set the board image
	vector <pair<int, int>> changedregions;
	findChangedRegions(src, changedregions);

	//extract and classify the cards for the changed regions
	for (int i = 0; i < changedregions.size(); i++) {
		int index = changedregions.at(i).second;
		cout << "Extract card for changed region : " << index << std::endl;
		extractAndClassifyCard(index);
	}
}


void BoardKlondike::extractAndClassifyImpactedRegions(int xUp, int yUp, int xDown, int yDown) {
	vector<int> listindex;
	if (isPileClicked(xUp, yUp)) {
		//extract talon card
		listindex.push_back(7);
	}
	else {
		int rowup, indexup, rowdown, indexdown;
		identifyClickedCard(xUp, yUp, rowup, indexup);
		listindex.push_back(indexup);
		if (xDown != -1) {
			identifyClickedCard(xDown, yDown, rowdown, indexdown);
			listindex.push_back(indexdown);
			if (indexdown == indexup && rowup == rowdown) {
				//not drag/drop between regions - add the previous one
				listindex.push_back(previouslySelectedIndex);
			}
		} else {
			//double-click - add the suit regions
			listindex.push_back(8);
			listindex.push_back(9);
			listindex.push_back(10);
			listindex.push_back(11);
		}
	}

	//sort the impacted regions
	std::sort(listindex.begin(), listindex.end(), [](const int& c1, const int& c2) -> bool { return (c1  < c2); });
	int index = -1, previous_index = -1;
	for (int i = 0; i < listindex.size(); i++) {
		index = listindex.at(i);
		if (index >= 0 && index != previous_index) {
			cout << "Extract card for impacted region : " << index << std::endl;
			extractAndClassifyCard(i);
		}
		previous_index = index;
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
			previousPlayingBoards.push_back(currentPlayingBoard);
			std::cout << "Change of board" << std::endl;
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
	//convert classified cards to single array for processing
	std::vector<std::pair<classifiers, classifiers>> classifiedCardsFromPlayingBoard(12);
	for (int j = 0; j < currentPlayingBoard.at(0).size(); j++) classifiedCardsFromPlayingBoard.at(j) = currentPlayingBoard.at(0).at(j).classifiedCard;
	
	int changedIndex1 = -1, changedIndex2 = -1;
	findChangedCardLocations(classifiedCardsFromPlayingBoard, changedIndex1, changedIndex2); // check which card locations have changed, this is maximum 2 (move from loc1 to loc2, or click on deck)
	if (changedIndex1 == -1 && changedIndex2 == -1)
	{
		return false; // no locations changed, no update needed
	}

	// pile pressed (only talon changed)
	if (changedIndex1 == 7 && changedIndex2 == -1)
	{
		currentPlayingBoard.at(0).at(7).topCard = classifiedCardsFromPlayingBoard.at(7);
		//printPlayingBoard();
		return true;
	}

	// card move from talon to board
	if (changedIndex1 == 7 || changedIndex2 == 7)
	{
		int tempIndex;	// contains the other index
		(changedIndex1 == 7) ? (tempIndex = changedIndex2) : (tempIndex = changedIndex1);
		currentPlayingBoard.at(0).at(tempIndex).knownCards.push_back(currentPlayingBoard.at(0).at(7).topCard);	// add the card to the new location
		currentPlayingBoard.at(0).at(7).topCard = classifiedCardsFromPlayingBoard.at(7);	// update the card at the talon
		--currentPlayingBoard.at(0).at(7).remainingCards;
		//printPlayingBoard();
		--currentPlayingBoard.at(0).at(7).remainingCards;
		++currentPlayingBoard.at(0).at(tempIndex).numberOfPresses;	// update the number of presses on that index
		(0 <= tempIndex && tempIndex < 7) ? score += 5 : score += 10;	// update the score
		return true;
	}

	// card move between build stack and suit stack
	if (changedIndex1 != -1 && changedIndex2 != -1)
	{
		if (cardMoveBetweenBuildAndSuitStack(classifiedCardsFromPlayingBoard, changedIndex1, changedIndex2))	// true if changed board
		{
			//printPlayingBoard();
			return true;
		}
	}

	// error with previously detected card
	if (changedIndex1 != -1 && changedIndex1 != 7 && changedIndex2 == -1)	// one card location changed which wasn't a talon change
	{
		if (currentPlayingBoard.at(0).at(changedIndex1).knownCards.empty())	// animation of previous state ended too fast, a card was still turning over which was missed
																		// -> no known cards in the list, but there is a card at that location: update knownCards
		{
			if (currentPlayingBoard.at(0).at(changedIndex1).remainingCards > 0)
			{
				--currentPlayingBoard.at(0).at(changedIndex1).remainingCards;
			}
			currentPlayingBoard.at(0).at(changedIndex1).knownCards.push_back(classifiedCardsFromPlayingBoard.at(changedIndex1));
			//printPlayingBoard();
			score += 5;
			return true;
		}
		return false;
	}
	
	return false;
}


void BoardKlondike::findChangedCardLocations(const std::vector<std::pair<classifiers, classifiers>> &classifiedCardsFromPlayingBoard, int & changedIndex1, int & changedIndex2)
{
	for (int i = 0; i < currentPlayingBoard.at(0).size(); i++)
	{
		if (i == 7)	// card is different from the topcard of the talon AND the card isn't empty
		{
			if (classifiedCardsFromPlayingBoard.at(7) != currentPlayingBoard.at(0).at(7).topCard)
			{
				changedIndex1 == -1 ? (changedIndex1 = 7) : (changedIndex2 = 7);
			}
		}
		else if (currentPlayingBoard.at(0).at(i).knownCards.empty())	// adding card to an empty location
		{
			if (classifiedCardsFromPlayingBoard.at(i).first != EMPTY)	// new card isn't empty
			{
				changedIndex1 == -1 ? (changedIndex1 = i) : (changedIndex2 = i);
			}
		}
		else
		{
			if (currentPlayingBoard.at(0).at(i).knownCards.back() != classifiedCardsFromPlayingBoard.at(i))	// classified topcard is different from the previous topcard
			{
				changedIndex1 == -1 ? (changedIndex1 = i) : (changedIndex2 = i);
			}
		}
		if (changedIndex2 != -1) { return; }	// if 2 changed location were detected, return
	}
}


bool BoardKlondike::cardMoveBetweenBuildAndSuitStack(const std::vector<std::pair<classifiers, classifiers>> &classifiedCardsFromPlayingBoard, int changedIndex1, int changedIndex2)
{
	// 1. CURRENT VISIBLE CARD WAS ALREADY IN THE LIST OF KNOWN CARDS -> ONE OR MORE TOPCARDS WERE MOVED TO A OTHER LOCATION
	auto inList1 = std::find(currentPlayingBoard.at(0).at(changedIndex1).knownCards.begin(), currentPlayingBoard.at(0).at(changedIndex1).knownCards.end(), classifiedCardsFromPlayingBoard.at(changedIndex1));
	auto inList2 = std::find(currentPlayingBoard.at(0).at(changedIndex2).knownCards.begin(), currentPlayingBoard.at(0).at(changedIndex2).knownCards.end(), classifiedCardsFromPlayingBoard.at(changedIndex2));

	// 1.1 The card was in the list of the card at index "changedIndex2"
	//		-> remove all cards after this index (iterator++) from knowncards at index "changedIndex2" and add them to the knowncards at index "changedIndex1"
	if (inList1 != currentPlayingBoard.at(0).at(changedIndex1).knownCards.end() && inList2 == currentPlayingBoard.at(0).at(changedIndex2).knownCards.end())
	{
		inList1++;
		currentPlayingBoard.at(0).at(changedIndex2).knownCards.insert(
			currentPlayingBoard.at(0).at(changedIndex2).knownCards.end(),
			inList1,
			currentPlayingBoard.at(0).at(changedIndex1).knownCards.end());
		currentPlayingBoard.at(0).at(changedIndex1).knownCards.erase(
			inList1,
			currentPlayingBoard.at(0).at(changedIndex1).knownCards.end());

		// update the score
		if (8 <= changedIndex2 && changedIndex2 < 12 && 0 <= changedIndex1 && changedIndex1 < 7)	// build to suit stack
		{
			score += 10;
		}
		else if (8 <= changedIndex1 && changedIndex1 < 12 && 0 <= changedIndex2 && changedIndex2 < 7)	// suit to build stack
		{
			score -= 10;
		}
		++currentPlayingBoard.at(0).at(changedIndex2).numberOfPresses;
		return true;
	}

	// 1.2 The card was in the list of the card at index "changedIndex1"
	//		-> remove all cards after this index (iterator++) from knowncards at index "changedIndex1"  and add them to the knowncards at index "changedIndex2"
	if (inList1 == currentPlayingBoard.at(0).at(changedIndex1).knownCards.end() && inList2 != currentPlayingBoard.at(0).at(changedIndex2).knownCards.end())
	{
		inList2++;
		currentPlayingBoard.at(0).at(changedIndex1).knownCards.insert(
			currentPlayingBoard.at(0).at(changedIndex1).knownCards.end(),
			inList2,
			currentPlayingBoard.at(0).at(changedIndex2).knownCards.end());
		currentPlayingBoard.at(0).at(changedIndex2).knownCards.erase(
			inList2,
			currentPlayingBoard.at(0).at(changedIndex2).knownCards.end());

		// update the score
		if (8 <= changedIndex1 && changedIndex1 < 12 && 0 <= changedIndex2 && changedIndex2 < 7)	// build to suit stack
		{
			score += 10;
		}
		else if (8 <= changedIndex2 && changedIndex2 < 12 && 0 <= changedIndex1 && changedIndex1 < 7)	// suit to build stack
		{
			score -= 10;
		}
		++currentPlayingBoard.at(0).at(changedIndex1).numberOfPresses;
		return true;
	}

	// 2. CURRENT VISIBLE CARD WAS NOT IN THE LIST OF KNOWN CARDS -> ALL TOPCARDS WERE MOVED TO A OTHER LOCATION AND A NEW CARD TURNED OVER
	//		-> check if the current topcard was in the list of the other cardlocation, if so, all cards of the other cardlocation were moved to this location

	if (inList1 == currentPlayingBoard.at(0).at(changedIndex1).knownCards.end() && inList2 == currentPlayingBoard.at(0).at(changedIndex2).knownCards.end())
	{
		auto inList1 = std::find(currentPlayingBoard.at(0).at(changedIndex1).knownCards.begin(), currentPlayingBoard.at(0).at(changedIndex1).knownCards.end(), classifiedCardsFromPlayingBoard.at(changedIndex2));
		auto inList2 = std::find(currentPlayingBoard.at(0).at(changedIndex2).knownCards.begin(), currentPlayingBoard.at(0).at(changedIndex2).knownCards.end(), classifiedCardsFromPlayingBoard.at(changedIndex1));

		// 2.1 The topcard at index "changedIndex2" was in the list of the card at index "changedIndex1"
		//		-> all cards from "changedIndex1" were moved to "changedIndex2" AND "changedIndex1" got a new card
		//		-> remove all knowncards "changedIndex1" and add them to the knowncards at "changedIndex2" AND add the new card to "changedIndex1"
		if (inList1 != currentPlayingBoard.at(0).at(changedIndex1).knownCards.end() && inList2 == currentPlayingBoard.at(0).at(changedIndex2).knownCards.end())
		{
			currentPlayingBoard.at(0).at(changedIndex2).knownCards.insert(
				currentPlayingBoard.at(0).at(changedIndex2).knownCards.end(),
				currentPlayingBoard.at(0).at(changedIndex1).knownCards.begin(),
				currentPlayingBoard.at(0).at(changedIndex1).knownCards.end());
			currentPlayingBoard.at(0).at(changedIndex1).knownCards.clear();
			if (classifiedCardsFromPlayingBoard.at(changedIndex1).first != EMPTY)
			{
				currentPlayingBoard.at(0).at(changedIndex1).knownCards.push_back(classifiedCardsFromPlayingBoard.at(changedIndex1));
				score += 5;
				--currentPlayingBoard.at(0).at(changedIndex1).remainingCards;
			}

			// update the score
			if (8 <= changedIndex2 && changedIndex2 < 12 && 0 <= changedIndex1 && changedIndex1 < 7)	// build to suit stack
			{
				score += 10;
			}
			else if (8 <= changedIndex1 && changedIndex1 < 12 && 0 <= changedIndex2 && changedIndex2 < 7)	// suit to build stack
			{
				score -= 10;
			}
			++currentPlayingBoard.at(0).at(changedIndex2).numberOfPresses;
			return true;
		}

		// 2.2 The topcard at index "changedIndex1" was in the list of the card at index "changedIndex2"
		//		-> all cards from "changedIndex2" were moved to "changedIndex1" AND "changedIndex2" got a new card
		//		-> remove all knowncards "changedIndex2" and add them to the knowncards at "changedIndex1" AND add the new card to "changedIndex2"
		if (inList1 == currentPlayingBoard.at(0).at(changedIndex1).knownCards.end() && inList2 != currentPlayingBoard.at(0).at(changedIndex2).knownCards.end())
		{
			currentPlayingBoard.at(0).at(changedIndex1).knownCards.insert(
				currentPlayingBoard.at(0).at(changedIndex1).knownCards.end(),
				currentPlayingBoard.at(0).at(changedIndex2).knownCards.begin(),
				currentPlayingBoard.at(0).at(changedIndex2).knownCards.end());
			currentPlayingBoard.at(0).at(changedIndex2).knownCards.clear();
			if (classifiedCardsFromPlayingBoard.at(changedIndex2).first != EMPTY)
			{
				currentPlayingBoard.at(0).at(changedIndex2).knownCards.push_back(classifiedCardsFromPlayingBoard.at(changedIndex2));
				score += 5;
				--currentPlayingBoard.at(0).at(changedIndex2).remainingCards;
			}

			// update the score
			if (8 <= changedIndex1 && changedIndex1 < 12 && 0 <= changedIndex2 && changedIndex2 < 7)	// build to suit stack
			{
				score += 10;
			}
			else if (8 <= changedIndex2 && changedIndex2 < 12 && 0 <= changedIndex1 && changedIndex1 < 7)	// suit to build stack
			{
				score -= 10;
			}
			++currentPlayingBoard.at(0).at(changedIndex1).numberOfPresses;
			return true;
		}
	}
	return false;
}


void BoardKlondike::processCardSelection(const int x, const int y)
{
	
	int row, indexOfPressedCardLocation;
	identifyClickedCard(x, y, row, indexOfPressedCardLocation);

	if (indexOfPressedCardLocation != -1)
	{
		//NEEDS TO BE MODIFIED
		Mat crop = boardImage(currentPlayingBoard.at(0).at(indexOfPressedCardLocation).region);
		int indexOfPressedCard = ec.getIndexOfSelectedCard(crop);	// check which card(s) on that index have been selected using a blue filter
																						//cout << "INDEX OF PRESSED CARD : " << indexOfPressedCard << std::endl; 
		if (indexOfPressedCard != -1)													//	-> returns an index from bottom->top of how many cards have been selected
		{
			std::pair<classifiers, classifiers> selectedCard;
			if (indexOfPressedCardLocation != 7)
			{
				int index = (int)currentPlayingBoard.at(0).at(indexOfPressedCardLocation).knownCards.size() - indexOfPressedCard - 1;	// indexOfPressedCard is from bot->top, knownCards is from top->bot - remap index
				if (index >= currentPlayingBoard.at(0).at(indexOfPressedCardLocation).knownCards.size()) {
					cout << "UNEXPECTED ERROR WIth INDEX OF PRESSED CARD - OUT OF RANGE OF KNOWN CARDS : " << indexOfPressedCardLocation << " - " << indexOfPressedCard << std::endl;
					index = 0;
				}
				selectedCard = currentPlayingBoard.at(0).at(indexOfPressedCardLocation).knownCards.at(index);	// check which card has been selected
			}
			else
			{
				selectedCard = currentPlayingBoard.at(0).at(indexOfPressedCardLocation).topCard;
			}
			detectPlayerMoveErrors(selectedCard, indexOfPressedCardLocation);	// detection of wrong card placement using previous and current selected card

			previouslySelectedCard = selectedCard;	// update previously detected card
			previouslySelectedIndex = indexOfPressedCardLocation;

			// +++metrics+++
			if (indexOfPressedCard == 0)	// topcard has been pressed
			{
				locationOfPresses.push_back(locationOfLastPress);	// add the coordinate to the vector of all presses for visualisation in the end
			}
			++currentPlayingBoard.at(0).at(indexOfPressedCardLocation).numberOfPresses;	// increment the pressed card location
		}
		else	// no card has been selected (just a click on the screen below the card)
		{
			previouslySelectedCard.first = EMPTY;
			previouslySelectedCard.second = EMPTY;
			previouslySelectedIndex = -1;
		}

		// +++metrics+++
		if (7 <= indexOfPressedCardLocation || indexOfPressedCardLocation < 12)	// the talon and suit stack are all 'topcards' and can immediately be added to locationOfPresses
		{
			locationOfPresses.push_back(locationOfLastPress);
		}
	}

	std::cout << "Previously selected card index : " << previouslySelectedIndex << std::endl;

}

void BoardKlondike::detectPlayerMoveErrors(std::pair<classifiers, classifiers> &selectedCard, int indexOfPressedCardLocation)
{
	if (previouslySelectedCard.first != EMPTY && previouslySelectedCard != selectedCard)	// current selected card is different from the previous, and there was a card previously selected
	{
		char prevSuit = static_cast<char>(previouslySelectedCard.second);
		char newSuit = static_cast<char>(selectedCard.second);
		char prevRank = static_cast<char>(previouslySelectedCard.first);
		char newRank = static_cast<char>(selectedCard.first);

		if (8 <= indexOfPressedCardLocation && indexOfPressedCardLocation < 12 && 0 <= previouslySelectedIndex && previouslySelectedIndex < 8)	// suit stack
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

/*
int BoardKlondike::determineIndexOfPressedCard(const int x, const int y)	// check if a cardlocation has been pressed and remap the coordinate to that cardlocation
{																				// -> uses hardcoded values (possible because the screen is always an identical 1600x900)
			
																	//std::cout << "determineIndexOfPressedcard: x : " << x << " y: " << y << std::endl;
	//SOLVED NO USE OF ABSOLUTE COORDINATES
	int row, index;
	bool found = identifyClickedCard(x, y, row, index);
	if (found == false) return -1;

	locationOfLastPress.first = x - currentPlayingBoard.at(row).at(index).region.x;
	locationOfLastPress.second = y - currentPlayingBoard.at(row).at(index).region.y;

	return index;

	
	if (84 <= y && y <= 258)
	{
		if (434 <= x && x <= 629)
		{
			locationOfLastPress.first = x - 434;
			locationOfLastPress.second = y - 84;
			return 7;
		}
		else if (734 <= x && x <= 865)
		{
			locationOfLastPress.first = x - 734;
			locationOfLastPress.second = y - 84;
			return 8;
		}
		else if (884 <= x && x <= 1015)
		{
			locationOfLastPress.first = x - 884;
			locationOfLastPress.second = y - 84;
			return 9;
		}
		else if (1034 <= x && x <= 1165)
		{
			locationOfLastPress.first = x - 1034;
			locationOfLastPress.second = y - 84;
			return 10;
		}
		else if (1184 <= x && x <= 1315)
		{
			locationOfLastPress.first = x - 1184;
			locationOfLastPress.second = y - 84;
			return 11;
		}
		else
		{
			return -1;
		}
	}
	else if (290 <= y && y <= 850)
	{
		if (284 <= x && x <= 415)
		{
			locationOfLastPress.first = x - 284;
			locationOfLastPress.second = y - 290;
			return 0;
		}
		else if (434 <= x && x <= 565)
		{
			locationOfLastPress.first = x - 434;
			locationOfLastPress.second = y - 290;
			return 1;
		}
		else if (584 <= x && x <= 715)
		{
			locationOfLastPress.first = x - 584;
			locationOfLastPress.second = y - 290;
			return 2;
		}
		else if (734 <= x && x <= 865)
		{
			locationOfLastPress.first = x - 734;
			locationOfLastPress.second = y - 290;
			return 3;
		}
		else if (884 <= x && x <= 1015)
		{
			locationOfLastPress.first = x - 884;
			locationOfLastPress.second = y - 290;
			return 4;
		}
		else if (1034 <= x && x <= 1165)
		{
			locationOfLastPress.first = x - 1034;
			locationOfLastPress.second = y - 290;
			return 5;
		}
		else if (1184 <= x && x <= 1315)
		{
			locationOfLastPress.first = x - 1184;
			locationOfLastPress.second = y - 290;
			return 6;
		}
		else
		{
			return -1;
		}
	}
	else
	{
		return -1;
	}
	return -1;
	
}
*/


void BoardKlondike::calculateFinalScore()
{
	int remainingCards = 0;
	for (int i = 0; i < 7; ++i)
	{
		remainingCards += (int)currentPlayingBoard.at(0).at(i).knownCards.size();
	}
	score += (remainingCards * 10);
}


void BoardKlondike::printPlayingBoard()
{
	std::cout << "Score: " << score << std::endl;	// print the score

	std::cout << "Talon: ";	// print the current topcard from deck
	if (currentPlayingBoard.at(0).at(7).topCard.first == EMPTY)
	{
		std::cout << "// ";
	}
	else
	{
		std::cout << static_cast<char>(currentPlayingBoard.at(0).at(7).topCard.first) << static_cast<char>(currentPlayingBoard.at(0).at(7).topCard.second);
	}
	std::cout << "	Remaining: " << currentPlayingBoard.at(0).at(7).remainingCards << std::endl;

	std::cout << "Suit stack: " << std::endl;		// print all cards from the suit stack
	for (int i = 8; i < currentPlayingBoard.at(0).size(); i++)
	{
		std::cout << "   Pos " << i - 8 << ": ";
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
	for (int i = 0; i < 7; i++)
	{
		std::cout << "   Pos " << i << ": ";
		if (currentPlayingBoard.at(0).at(i).knownCards.empty())
		{
			std::cout << "// ";
		}
		for (int j = 0; j < currentPlayingBoard.at(0).at(i).knownCards.size(); j++)
		{
			std::cout << static_cast<char>(currentPlayingBoard.at(0).at(i).knownCards.at(j).first) << static_cast<char>(currentPlayingBoard.at(0).at(i).knownCards.at(j).second) << " ";
		}
		std::cout << "	Hidden cards = " << currentPlayingBoard.at(0).at(i).remainingCards << std::endl;
	}


	//IMPLEMENTED in GAMEANALYTICS
	/*
	auto averageThinkTime2 = Clock::now();	// add the average think duration to the list
	averageThinkDurations.push_back(std::chrono::duration_cast<std::chrono::milliseconds>(averageThinkTime2 - startOfMove).count());
	startOfMove = Clock::now();
	std::cout << std::endl;
	*/

}

bool BoardKlondike::isGameComplete() {
	int cardsLeft = 0;
	for (int i = 0; i < 8; i++)
	{
		if (currentPlayingBoard.at(0).at(i).remainingCards > 0)
		{
			++cardsLeft;
		}
	}
	if (cardsLeft > 0) return false; else return true;
}