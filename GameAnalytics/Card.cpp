#include "stdafx.h"
#include "Card.h"


Card::Card()
{
	topCard.first = UNKNOWN;
	topCard.second = UNKNOWN;
}


Card::~Card()
{
}

void Card::initValues(const int r, const int i, bool p, const int rem, std::pair<classifiers, classifiers> c, const bool flagknowncard) {
	row = r;
	index = i;
	playable = p;
	topCard = c;
	remainingCards = rem;

	//if card is known, push it to the list
	knownCards.clear();
	if (flagknowncard && c.first != EMPTY) knownCards.push_back(c);

	numberOfPresses = 0;
}

int Card::getRankValue() {
	return getRankValue(topCard.first);
}

int Card::getRankValue(const classifiers c) {

	switch (c) {

	case 'A':
		return 1;
		break;
	case '2':
		return 2;
		break;
	case '3':
		return 3;
		break;
	case '4':
		return 4;
		break;
	case '5':
		return 5;
		break;
	case '6':
		return 6;
		break;
	case '7':
		return 7;
		break;
	case '8':
		return 8;
		break;
	case '9':
		return 9;
		break;
	case ':':
		return 10;
		break;
	case 'J':
		return 11;
		break;
	case 'Q':
		return 12;
		break;
	case 'K':
		return 13;
		break;
	case '?':
		return 0;
		break;
	case '/':
		return 0;
		break;
	default:
		return 0;
		break;
	}

}