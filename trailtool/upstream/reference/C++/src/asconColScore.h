/*
AsconTrailTool, a set of C++ classes for analyzing ASCON.

For specifications, please refer to https://tosc.iacr.org/index.php/ToSC/article/view/9975

Implementation by Solane El Hirch, hereby denoted as "the implementer".
*/

#ifndef ASCONCOLSCORE_H
#define ASCONCOLSCORE_H

#include <cstdint>
#include <cstring>

#include "config.h"
#include "asconParts.h"

#include "trailExtension.h"


using namespace std;

typedef unsigned int columnValue;


#if DTS == 1 
static const UINT64 outputPatternAscon[5][1] = {
      {0x0000201000000001},
      {0x0000000002000009}, 
      {0x8400000000000001},  
      {0x0040800000000001},
      {0x0200000000800001} 
};
#else
static const UINT64 outputPatternAscon[5][1] = {
      {0x0000000010080001},
      {0x2000008000000001}, 
      {0x0000000000000043},  
      {0x0000000000020401},
      {0x0000020000000081} 
};
#endif


class Shadow 
{
public:
	UINT64 staticMask[5];
	int shiftOffset[5];
	int piInvOffset[5];
	int piOffset[5];
	
	columnValue stateAtA[64]; 
	int nrOfActColAtA;
	State stateAtB;

public:
#if DTS == 1 
	Shadow():staticMask{0xffffffffffffffff, 0xffffffffffffffff, 0xffffffffffffffff,0xffffffffffffffff,
	0xffffffffffffffff}, shiftOffset{0, 39, 0, 10, 0}, piInvOffset{55, 25, 1, 27, 15}, piOffset{7, 41, 1, 19, 47}, 
	nrOfActColAtA(0) 
	{
		memset(stateAtA, 0, 64*sizeof(int));
		stateAtB = State();
	}
#else
	Shadow():staticMask{0xffffffffffffffff, 0xffffffffffffffff, 0xffffffffffffffff,0xffffffffffffffff,
	0xffffffffffffffff}, shiftOffset{0, 25, 0, 54, 0}, piInvOffset{9, 39, 63, 37, 49}, piOffset{57, 23, 63, 45, 17}, 
	nrOfActColAtA(0) 
	{
		memset(stateAtA, 0, 64*sizeof(int));
		stateAtB = State();
	}
#endif
	int checkScore();
	int checkScoreSibling(int z, int y);
	int addUnitToShadow(int z, int y);
	int rmUnitToShadow(int z, int y);

	void updateStateAtACol(int x, int y, int step);

	int weightAtA();
	int weightAtB();

	int getNrOfActColAtA();
	int getNrOfActColAtB();

	int weightAtACol();
	int weightAtBCol();

	// new score
	int minRevBiggerThanTwo();

	int totalWeight(int &hwAtB);

	void initMaskInX();
};



#endif /* ASCONCOLSCORE_H */
