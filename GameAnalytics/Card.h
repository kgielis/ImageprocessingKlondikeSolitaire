#pragma once

#include "stdafx.h"

// opencv includes"
#include "opencv2/core.hpp"
#include "opencv2/opencv.hpp"

// other includes
#include <vector>
#include <utility>
#include <iostream>
#include <stdlib.h>
#include <string>
#include <fstream>
#include <iterator>

using namespace cv;
using namespace std;

#define standardCardWidth 150
#define standardCardHeight 200

enum classifiers : char {
	TWO = '2', THREE = '3', FOUR = '4', FIVE = '5', SIX = '6',
	SEVEN = '7', EIGHT = '8', NINE = '9', TEN, JACK = 'J', QUEEN = 'Q', KING = 'K', ACE = 'A',
	CLUBS = 'C', SPADES = 'S', HEARTS = 'H', DIAMONDS = 'D', UNKNOWN = '?', EMPTY = '/'
};

class Card
{
public:
	Card();
	~Card();

	int getRankValue(); //for top card
	static int getRankValue(const classifiers c); 
	void initValues(const int r, const int i, bool p, const int rem, std::pair<classifiers, classifiers> c, const bool flagknowncard);

	bool existsInBoard() { if (row >= 0 && index >= 0) return true; else return false; } //ternary operator

private:
	std::pair<classifiers, classifiers> topCard;
	std::pair<classifiers, classifiers> classifiedCard; //could be handy to process changes of the board

	bool playable = false;
	int row = -1;
	int index = -1;
	Rect region;

	int remainingCards = 0;
	std::vector<std::pair<classifiers, classifiers>> knownCards;	// used for the build and suit stack
	int numberOfPresses = 0;

	friend class Board;
	friend class BoardPyramid;
	friend class BoardKlondike;
};