#include "stdafx.h"
#include "BoardPyramid.h"


BoardPyramid::BoardPyramid()
{
}


BoardPyramid::~BoardPyramid()
{
}


bool BoardPyramid::build(Mat & src) {

	std::cout << "building board Pyramid" << std::endl;

	ec.determineROI(src);
	setBoardImage(src);

	//declare variables
	init();
	boardsCompleted = 0;
	sumErrors = 0;

	std::vector<Rect> listrect;
	std::vector<bool> iscardcomplete;
	ec.identifyCardRegions(boardImage, listrect, iscardcomplete);

	//extract the standard card size of playable cards
	int h = 0; //standard height
	int w = 0; //standard width
	for (int r = 0; r < listrect.size(); r++) {
		if (iscardcomplete.at(r)) {
			if (listrect.at(r).height > h) h = listrect.at(r).height;
			if (listrect.at(r).width > w) w = listrect.at(r).width;
		}
	}

	std::pair<classifiers, classifiers> cardType;

	Card card;
	std::vector<Card> cardList; //local variable for unorganized playing board
	std::vector<int> rowPositions;

	for (int i = 0; i < listrect.size(); i++) {
		Rect rect = listrect.at(i);
		card.region = rect;

		Mat crop = boardImage(rect);
		Mat resizedCardImage;

		//imshow("extracted card", crop);
		//waitKey();

		//note that classifyCard will first resize the card
		cardType = cc.classifyCard(crop, false);
		//std::cout << "Cardtype: " << static_cast<char>(cardType.first) << static_cast<char>(cardType.second) << std::endl;

		card.playable = iscardcomplete.at(i);

		//Point leftCorner = Point(ROI.x + rect.x, ROI.y + rect.y);

		//std::cout << "Coordinates: " << leftCorner << std::endl;

		card.topCard = cardType;
		//card.leftCorner = leftCorner;
		//card.rightTop = Point(rect.x + rect.width, rect.y);
		//card.rightBot = Point(ROI.x + rect.x + rect.width, ROI.y + rect.y + rect.height);
		//card.leftBot = Point(rect.x, rect.y + rect.height);

		rowPositions.push_back(card.region.y);
		cardList.push_back(card);
	}

	std::sort(rowPositions.begin(), rowPositions.end(), [](int const &a, int const &b) -> bool { return b > a; });

	std::vector<int>::iterator it;
	it = std::unique(rowPositions.begin(), rowPositions.end());
	rowPositions.resize(std::distance(rowPositions.begin(), it));

	std::cout << "ordered row coordinates: " << std::endl;
	for (int i = 0; i < rowPositions.size(); i++) std::cout << rowPositions.at(i) << " ";
	std::cout << std::endl;

	currentPlayingBoard.resize(rowPositions.size());

	for (int i = 0; i < cardList.size(); i++) {
		Card card = cardList.at(i);
		int rowCoo = card.region.y;
		//std::cout << "rowCoo : " << rowCoo << std::endl;

		for (int j = 0; j < rowPositions.size(); j++) {

			//std::cout << rowPositions.at(j) << std::endl;
			if (card.region.y == rowPositions.at(j)) { //TO DO add toleration
				card.row = j;
				currentPlayingBoard.at(j).push_back(card);

				//std::cout << "rowPosition : " << j << std::endl;
				//rowXIndexes.at(j).push_back(card.leftCorner.x);
			}
		}
	}

	for (int i = 0; i < currentPlayingBoard.size(); i++) {

		std::vector<Card>& row = currentPlayingBoard.at(i);

		//std::cout << "Card of row " << i << ": ";

		std::sort(row.begin(), row.end(), [](const Card& c1, const Card& c2) -> bool { return (c1.region.x < c2.region.x); });
		for (int j = 0; j < row.size(); j++) {
			row.at(j).index = j;
			// update the region
			if (row.at(j).playable == false) { row.at(j).region.width = w; row.at(j).region.height = h; }
			//std::cout << static_cast<char>(row.at(j).cardType.first) << static_cast<char>(row.at(j).cardType.second) << " ; ";
		}
		//std::cout << std::endl;
	}


	//add missing empty card regions right by using low treshold
	int lastrow = currentPlayingBoard.size() - 1;
	int lastindex = currentPlayingBoard.at(lastrow).size() - 1;
	Rect r = currentPlayingBoard.at(lastrow).at(lastindex).region;

	ec.identifyCardRegionsRightFromRectangle(boardImage, r, listrect, iscardcomplete);
	cout << "detect missing regions " << listrect.size() << endl;
	for (int k = 0; k < listrect.size(); k++) {
		Card emptycard;
		emptycard.index = lastindex + 1;
		emptycard.row = lastrow;
		emptycard.playable = true;
		Rect tmp = listrect.at(k);
		tmp.x = tmp.x + r.x + r.width;
		tmp.y = tmp.y + r.y;
		emptycard.region = tmp;
		emptycard.topCard.first = EMPTY;
		emptycard.topCard.second = EMPTY;

		currentPlayingBoard.at(lastrow).push_back(emptycard);
	}

	//Draw region - extract square between the cards - if not succeeded, use entire areq between the cards
	r = Rect(getXRight(7, 0), getYUp(7, 0), getXLeft(7, 1) - getXRight(7, 0), getYDown(7, 1) - getYUp(7, 0)); 
	Rect s; //square if found
	if (ec.identifySquare(boardImage(r), s)) pileRegion = Rect(r.x + s.x, r.y + s.y, s.width, s.height); else pileRegion = r;

	//initialize
	//rowCleared.resize(currentPlayingBoard.size());
	//for (int i = 0; i < rowCleared.size(); i++) {
		//rowCleared.at(i) = 0;
	//}

	//push board to list of previous boards for undo
	storeForUndo();

	//drawRegions();
	
	//validate board has pyramid shape
	bool validate = true;
	if (currentPlayingBoard.size() < 2) validate = false;
	for (int k = 1; k < currentPlayingBoard.size() - 1; k++) {
		if (currentPlayingBoard.at(k).size() != currentPlayingBoard.at(k-1).size() + 1) { validate = false; break; }
	}
	//last row should have size of 2
	if (validate) if (currentPlayingBoard.at(currentPlayingBoard.size()-1).size() != 2) validate = false;

	return validate;
}


void BoardPyramid::extractAndUpdateCard(const int row, const int index) {
	std::pair<classifiers, classifiers> cardType = extractAndClassifyCard(row, index);
	//update the top card in the board
	currentPlayingBoard.at(row).at(index).topCard = cardType;
}

std::pair<classifiers, classifiers> BoardPyramid::extractAndClassifyCard(const int row, const int index) {
	return(Board::extractAndClassifyCard(row, index, false, false));
}

void BoardPyramid::checkRowCleared(const int row) {

	//if (rowCleared.at(row) == currentPlayingBoard.at(row).size()) {
	if (isRowCleared(row)) {
		std::cout << "Row" << row << " cleared! " << std::endl;
		switch (row) {
		case 0:
			score += 500;
			boardsCompleted++;
			cout << "Board Completed!" << endl;
			break;
		case 1:
			score += 250;
			break;
		case 2:
			score += 150;
			break;
		case 3:
			score += 100;
			break;
		case 4:
			score += 75;
			break;
		case 5:
			score += 50;
			break;
		case 6:
			score += 25;
			break;
		default:
			break;
		}

		//rowCleared.at(row) = -1;
		//std::cout << "Score is now: " << score << std::endl;
	}
}

bool BoardPyramid::process(Mat & src, int xUp, int yUp, int xDown, int yDown) {

	setBoardImage(src);

	bool boardchange = false;
	
	Card clickedCard;
	int clickedrow, clickedindex;
    identifyClickedCard(xUp, yUp, clickedrow, clickedindex);
	if (clickedrow != -1) {
		clickedCard = currentPlayingBoard.at(clickedrow).at(clickedindex);
		locationOfPresses.push_back(locationOfLastPress);	// add the coordinate to the vector of all presses for visualisation in the end
		//cout << "Clicked card " << clickedrow << " - " << clickedindex << " : " << static_cast<char>(clickedCard.topCard.first) << static_cast<char>(clickedCard.topCard.second) << std::endl;
	}

	int score_before = score;
	
	if (isPileClicked(xUp, yUp)) {
		//cout << "Draw button is clicked" << endl;
		extractAndUpdateCard(7, 0);
		extractAndUpdateCard(7, 1);
		//cout << "Talon is extracted and classified" << endl;
		//printPlayingBoard();
		boardchange = true;
	}

	/*
	if (clickedCard.row == 7) {
		if (clickedCard.topCard.first == 'K') score += 5;
		extractAndUpdateCard(7, clickedCard.index);
		printPlayingBoard();
		boardchange = true;
	}
	*/

	int sum = 0;

	if (clickedCard.topCard.first == 'K') sum = 13;
	else sum = clickedCard.getRankValue() + Card::getRankValue(previouslySelectedCard.first);

	std::cout << "Sum is " << sum << std::endl;

	if (sum == 13) {

		score += 5;
		//cout << "Score is " << score << endl;

		if (clickedCard.row != 7) {
			clickedCard.topCard.first = EMPTY;
			clickedCard.topCard.second = EMPTY;
			clickedCard.playable = false;
			currentPlayingBoard.at(clickedCard.row).at(clickedCard.index) = clickedCard;
			//cout << "before rowCleared" << endl;
			//rowCleared.at(clickedCard.row)++;
			//cout << "cleared card in row" << endl;
			checkRowCleared(clickedCard.row);
			//cout << "check for cleared row done" << endl;
		}
		else {
			//cout << "In else statement of clickedCard" << clickedCard.row << clickedCard.index << endl;
			extractAndUpdateCard(clickedCard.row, clickedCard.index);
		}

		if (previouslySelectedRow != 7) {
			if (clickedCard.topCard.first != 'K' && previouslySelectedRow != -1) {
				previouslySelectedCard.first = EMPTY;
				previouslySelectedCard.second = EMPTY;
				currentPlayingBoard.at(previouslySelectedRow).at(previouslySelectedIndex).topCard = previouslySelectedCard;
				currentPlayingBoard.at(previouslySelectedRow).at(previouslySelectedIndex).playable = false;
				//rowCleared.at(previouslySelectedRow)++;
				checkRowCleared(previouslySelectedRow);
			}
		}
		else {
			//cout << "In else statement of previousClicked : " << previouslySelectedRow << " - " << previouslySelectedIndex << endl;
			extractAndUpdateCard(previouslySelectedRow, previouslySelectedIndex);
		}

		for (int i = 0; i < 2; i++) {

			Card tempCard;
			if (i == 0) tempCard = clickedCard;
			if (i == 1) {
				if (previouslySelectedRow == -1) break;
				else {
					tempCard.topCard = previouslySelectedCard;
					tempCard.row = previouslySelectedRow;
					tempCard.index = previouslySelectedIndex;
				}
			}
			//cout << "INDEX " << tempCard.index << " - ROW: " << tempCard.row << endl;

			if (tempCard.row > 0 && tempCard.row < 7) {
				if (tempCard.index > 0) {

					//cout << "Check left for " << i << endl;
					if (currentPlayingBoard.at(tempCard.row).at(tempCard.index - 1).topCard.first == EMPTY) {
						//cout << "Check left for " << i << endl;
						currentPlayingBoard.at(tempCard.row - 1).at(tempCard.index - 1).playable = true;
						//std::cout << "Card  at row: " << currentPlayingBoard.at(tempCard.row - 1).at(tempCard.index - 1).row << "and index " << currentPlayingBoard.at(tempCard.row - 1).at(tempCard.index - 1).index << " is now PLAYABLE" << std::endl;
						extractAndUpdateCard(tempCard.row - 1, tempCard.index - 1);
						//printPlayingBoard();
					}
				}
				if (tempCard.index < currentPlayingBoard.at(tempCard.row).size() - 1) {
					//cout << "Check right for " << i << endl;
					if (currentPlayingBoard.at(tempCard.row).at(tempCard.index + 1).topCard.first == EMPTY) {
						//cout << "Check right for " << i << endl;
						currentPlayingBoard.at(tempCard.row - 1).at(tempCard.index).playable = true;
						//std::cout << "Card  at row: " << currentPlayingBoard.at(tempCard.row - 1).at(tempCard.index).row << "and index " << currentPlayingBoard.at(tempCard.row - 1).at(tempCard.index).index << " is now PLAYABLE" << std::endl;
						extractAndUpdateCard(tempCard.row - 1, tempCard.index);
						//printPlayingBoard();
					}
				}
			}
		}

		clickedCard.topCard.first = EMPTY;
		if (score > score_before) boardchange = true;
	}
	//sum is not 13
	else {
		if (previouslySelectedRow != -1) sumErrors++;
	}
	
	previouslySelectedCard.first = EMPTY;
	previouslySelectedCard.second = EMPTY;
	previouslySelectedRow = -1;
	previouslySelectedIndex = -1;

	//check if there is previous card to be marked as selected
	//if sum was 13, there are no selected cards anymore
	if (clickedrow != -1 && sum != 13) {
		int margin = 5;
		Rect region = clickedCard.region;
		Rect rect(region.x - margin, region.y - margin, region.width + 2 * margin, region.height + 2 * margin);
		Mat crop = boardImage(rect);
		//imshow("selection", crop);
		//waitKey();
		int nr_sel = ec.getIndexOfSelectedCard(crop);
		if (nr_sel == 0) {
			previouslySelectedCard = clickedCard.topCard;
			previouslySelectedRow = clickedrow;
			previouslySelectedIndex = clickedindex;
		}
	}
	//cout << "Rank value of selected card (previous next time) : " << Card::getRankValue(previouslySelectedCard.first) << std::endl;

	if (boardchange) {
		printPlayingBoard();
		storeForUndo();
	} else {
		std::cout << "No change of board" << std::endl;
	}
	return boardchange;
 
}

void BoardPyramid::processCardSelection(const int x, const int y) {
	//register the click

}

void BoardPyramid::calculateFinalScore()
{

}

bool BoardPyramid::isGameComplete() {
	return false;
}
