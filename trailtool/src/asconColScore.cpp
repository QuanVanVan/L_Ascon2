/*
AsconTrailTool, a set of C++ classes for analyzing ASCON.

For specifications, please refer to https://tosc.iacr.org/index.php/ToSC/article/view/9975

Implementation by Solane El Hirch, hereby denoted as "the implementer".
*/

#include <iostream>
#include <cstdint>

#include "config.h"
#include "asconColScore.h"
#include "linear_mapping.h"

/* ------------------------------------------------------------ */
/* 								ORIGINAL 					    */
/* ------------------------------------------------------------ */

uint64_t biggerThanTwo(State &stateAtB) {
    // ~x4x0+~x4x1+x4x2+x4x3
    return (~(stateAtB.getRow(0)) & stateAtB.getRow(4)) ^ 
           (~(stateAtB.getRow(0)) & stateAtB.getRow(3)) ^
           ( (stateAtB.getRow(0)) & stateAtB.getRow(2)) ^
           ( (stateAtB.getRow(0)) & stateAtB.getRow(1));
}

uint64_t biggerThanThree(State &stateAtB) {
    // x3(~x2)x1 + ~x4x3(~x1)*x0
    return (stateAtB.getRow(1) & ~(stateAtB.getRow(2)) & stateAtB.getRow(3)) ^
        (~(stateAtB.getRow(0)) & stateAtB.getRow(1) & ~(stateAtB.getRow(3)) & stateAtB.getRow(4));
}


uint64_t biggerThanTwoLTS(State &stateAtB) {
    // x1x3 + (~x2)x3x4 + (~x0)x1x4 + (~x0)x1x2 + x0(~x1)x2 + (~x0)x2(~x3)x4
    return ((stateAtB.getRow(1)) & stateAtB.getRow(3)) ^ 
           (~(stateAtB.getRow(2)) & stateAtB.getRow(3) & stateAtB.getRow(4)) ^
           (~(stateAtB.getRow(0)) & stateAtB.getRow(1) & stateAtB.getRow(4)) ^
           (~(stateAtB.getRow(0)) & stateAtB.getRow(1) & stateAtB.getRow(2)) ^ 
           ((stateAtB.getRow(0)) & ~(stateAtB.getRow(1)) & stateAtB.getRow(2)) ^ 
           (~(stateAtB.getRow(0)) & stateAtB.getRow(2) & ~(stateAtB.getRow(3)) & stateAtB.getRow(4));
}

int Shadow::checkScore(){
	int nrActColAtA;
	int nrActColAtB;

	nrActColAtA = getNrOfActColAtA();
	nrActColAtB = getNrOfActColAtB();   
	
    if (weightedWeights == 11){
        if(nrActColAtA + nrActColAtB <= limit) {
			int score = 2 * nrActColAtA;
#if DTS == 1
			uint64_t biggerThan2 = biggerThanTwo(stateAtB);
			uint64_t biggerThan3 = biggerThanThree(stateAtB);
			score += 2*nrActColAtB + countBits(biggerThan2) + countBits(biggerThan3);
#else
			uint64_t biggerThan2 = biggerThanTwoLTS(stateAtB);
			score += 2*nrActColAtB + countBits(biggerThan2);
#endif
			if(score <= limitWeight){
            	return 1; 
    		}
		}
    }
    else if (weightedWeights == 12){
        if(nrActColAtA + 2*nrActColAtB <= limit) {
			int score = 2 * nrActColAtA;
#if DTS == 1
			uint64_t biggerThan2 = biggerThanTwo(stateAtB);
			uint64_t biggerThan3 = biggerThanThree(stateAtB);
			score += 2*(2*nrActColAtB + countBits(biggerThan2) + countBits(biggerThan3));
#else
			uint64_t biggerThan2 = biggerThanTwoLTS(stateAtB);
			score += 2*(2*nrActColAtB + countBits(biggerThan2));
#endif
			if(score <= limitWeight){
            	return 1; 
    		}
		}
    }
    else if (weightedWeights == 21){
        if(2*nrActColAtA + nrActColAtB <= limit)  {
			int score = 2*(2 * nrActColAtA);
#if DTS == 1
			uint64_t biggerThan2 = biggerThanTwo(stateAtB);
			uint64_t biggerThan3 = biggerThanThree(stateAtB);
			score += 2*nrActColAtB + countBits(biggerThan2) + countBits(biggerThan3);
#else
			uint64_t biggerThan2 = biggerThanTwoLTS(stateAtB);
			score += 2*nrActColAtB + countBits(biggerThan2);
#endif
			if(score <= limitWeight){
            	return 1; 
    		}
		}
    }
    else{
        if(nrActColAtA + 2*nrActColAtB <= limit || 2*nrActColAtA + nrActColAtB <= limit) 
            return 1;      
    }

	return 0;
}


int Shadow::checkScoreSibling(int z, int y){
	int nrActColAtA;
	int slidingVal;
	uint64_t sumAtB;
	uint64_t mask;
	uint64_t lastActRowValue;

	if(z<63)
		z = z+1;

	nrActColAtA = getNrOfActColAtA(); // return nr of active col
	slidingVal = (z*piInvOffset[y])&63;
	sumAtB = 0;
	// static mask
	mask = staticMask[y];
	lastActRowValue = stateAtB.getRow(y) & mask;

	// sliding mask
	mask = shift64(mask, slidingVal);

	lastActRowValue = lastActRowValue & mask;
	for(int i = 0; i < nrRows; i++){
		if(i != y)
			sumAtB |= stateAtB.getRow(i);
	}
	sumAtB |= lastActRowValue;
	
	int nrActColAtB = countBits(sumAtB);
	
    
    if (weightedWeights == 11){
        if(nrActColAtA + nrActColAtB <= limit) {
			int score = 2*nrActColAtA;
			// copy
			State stateBcopy = stateAtB;
			// mask
			stateBcopy.setRow(y, lastActRowValue);
#if DTS == 1
			uint64_t biggerThan2 = biggerThanTwo(stateBcopy);
			uint64_t biggerThan3 = biggerThanThree(stateBcopy);
			score += 2*nrActColAtB + countBits(biggerThan2) + countBits(biggerThan3);
#else
			uint64_t biggerThan2 = biggerThanTwoLTS(stateBcopy);
			score += 2*nrActColAtB + countBits(biggerThan2);
#endif
			if(score <= limitWeight){
            return 1; 
    		}
		}
    }
    else if (weightedWeights == 12){
        if(nrActColAtA + 2*countBits(sumAtB) <= limit) {
			int score = 2*nrActColAtA;
			// copy
			State stateBcopy = stateAtB;
			// mask
			stateBcopy.setRow(y, lastActRowValue);
#if DTS == 1
			uint64_t biggerThan2 = biggerThanTwo(stateBcopy);
			uint64_t biggerThan3 = biggerThanThree(stateBcopy);
			score += 2*(2*nrActColAtB + countBits(biggerThan2) + countBits(biggerThan3));
#else
		uint64_t biggerThan2 = biggerThanTwoLTS(stateBcopy);
		score += 2*(2*nrActColAtB + countBits(biggerThan2));
			
#endif
			if(score <= limitWeight){
            return 1; 
    		}
		}
    }
    else if (weightedWeights == 21){
        if(2*nrActColAtA + countBits(sumAtB) <= limit) {
			int score = 2*(2*nrActColAtA);
			// copy
			State stateBcopy = stateAtB;
			// mask
			stateBcopy.setRow(y, lastActRowValue);
#if DTS == 1
			uint64_t biggerThan2 = biggerThanTwo(stateBcopy);
			uint64_t biggerThan3 = biggerThanThree(stateBcopy);
			score += 2*nrActColAtB + countBits(biggerThan2) + countBits(biggerThan3);
#else
			uint64_t biggerThan2 = biggerThanTwoLTS(stateBcopy);
			score += 2*nrActColAtB + countBits(biggerThan2);
#endif
			if(score <= limitWeight){
            return 1; 
    		}
		}
    }
    else{
        if(nrActColAtA + 2*countBits(sumAtB) <= limit || 2*nrActColAtA + countBits(sumAtB) <= limit) 
            return 1;      
    }    


	return 0;
}

/* ------------------------------------------------------------ */
/* 							Update Shadow  					    */
/* ------------------------------------------------------------ */

int Shadow::addUnitToShadow(int z, int y){
	int x = (z*piInvOffset[y] + shiftOffset[y])&63;
	uint64_t image = outputPatternAscon[y][0];
	
	// state at A update function here
	updateStateAtACol(x, y, 1); 

	image = shift64(image, x); 
	image ^= stateAtB.getRow(y);
	stateAtB.setRow(y, image);
	return 1;
}

int Shadow::rmUnitToShadow(int z, int y){
	int x = (z*piInvOffset[y] + shiftOffset[y])&63;
	uint64_t image = outputPatternAscon[y][0];
	
	// state at A update function
	updateStateAtACol(x, y, 0);

	image = shift64(image, x); 
	image ^= stateAtB.getRow(y);
	stateAtB.setRow(y, image);

	return 1;
}


/* ------------------------------------------------------------ */
/* 						Nr of Active Columns 				    */
/* ------------------------------------------------------------ */

void Shadow::updateStateAtACol(int x, int y, int step){
	unsigned int indexBit = (0x1U << (nrRows-1-y));
	stateAtA[x] = stateAtA[x] ^ indexBit; 
	if(step == 1){
		// check if column is a power of 2
		if( (stateAtA[x] & (stateAtA[x] -1 )) == 0 ){
			nrOfActColAtA = nrOfActColAtA + 1;
		}	
		return;
	}
	if(stateAtA[x] == 0){
		nrOfActColAtA = nrOfActColAtA - 1;
	}
	return;
}

// reverse weight
int Shadow::weightAtACol(){
#if DTS == 1 
    unsigned int minRevWeightListPerOutput[32] = { 0, 2, 2, 3, 2, 3, 2, 3, 2, 2, 2, 2, 2, 3, 2, 3, 2, 2, 3, 2, 3, 2, 2, 2, 2, 2, 2, 3, 3, 3, 2, 3 };
#else
    unsigned int minRevWeightListPerOutput[32] = { 0, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 4, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2 };
#endif
    unsigned int minRevWeight = 0;
	int column;
    for (unsigned int i = 0; i < nrCols; i++) {
    	column = stateAtA[i];
        minRevWeight += minRevWeightListPerOutput[column];
    }
    return minRevWeight;
}

// weight
int Shadow::weightAtBCol(){
#if DTS == 1 
    unsigned int weightListPerInput[32] = { 0, 3, 3, 3, 2, 3, 4, 3, 3, 4, 4, 4, 2, 4, 3, 3, 2, 2, 4, 2, 3, 3, 4, 3, 4, 3, 4, 4, 3, 3, 4, 3 }; 
#else
    unsigned int weightListPerInput[32] = { 0, 2, 2, 4, 2, 4, 2, 2, 2, 4, 4, 4, 4, 4, 4, 4, 2, 2, 4, 4, 4, 4, 4, 4, 4, 2, 4, 4, 4, 2, 4, 4 };
#endif
    
    unsigned int weight = 0;
	int column;
    for (unsigned int i = 0; i < nrCols; i++) {
    	column = stateAtB.getColumn(i);
        weight += weightListPerInput[column];
    }
    return weight;
}

int Shadow::getNrOfActColAtA(){
	return nrOfActColAtA;
}

int Shadow::getNrOfActColAtB(){
	int nrOfActCol = 0;
	uint64_t result = 0;

	for(int i = 0; i < 5; i++){
		result |= stateAtB.getRow(i);
	}
	nrOfActCol = countBits(result);

	return nrOfActCol;
}

int Shadow::weightAtA(){
	return weightAtACol();
}

int Shadow::weightAtB(){
	return weightAtBCol();
}


/* ------------------------------------------------------------ */
/* 						Static masks init 					    */
/* ------------------------------------------------------------ */

void Shadow::initMaskInX(){
	int j;
	int z, x;
	uint64_t indexBit;

	int startOfIntervalInZ[5] = {5, 6, 6, 5, 9};
	int shiftFactor[5] = {0, 1, 0, 2, 0};

	for(int y = 0; y < nrRows; y++){
		j = startOfIntervalInZ[y];
		for(z = 64-j; z < 64; z++){
			x = (z*piInvOffset[y] + shiftOffset[y]) & 63;
      		indexBit = (0x1ULL << x);
      		staticMask[y] = staticMask[y] ^ indexBit;
		}
		staticMask[y] = shift64(staticMask[y], shiftFactor[y]*piInvOffset[y]); 
	}
	return;
}
