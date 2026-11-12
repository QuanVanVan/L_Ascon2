/*
AsconTrailTool, a set of C++ classes for analyzing ASCON.

For specifications, please refer to https://tosc.iacr.org/index.php/ToSC/article/view/9975

Implementation by Solane El Hirch, hereby denoted as "the implementer".
*/

#include <iostream>
#include <cstdint>
#include <algorithm> 

#include "treeCanonical.h"
#include "config.h"
#include "histogram.h"


using namespace std;

/* ---------------------------------------------------------------------------------- */
                              /* row functions (child-related) */
/* ---------------------------------------------------------------------------------- */

int RowAsUnits::toFirstChild(){
  int lastActUnitVal;

  if(units.size() == 0)
    lastActUnitVal = 0;
  else
    lastActUnitVal = units.back();

  if(period == 1){
    if(!toFirstCanonicalChild()) return 0;
    
    return 1;
  }else if(period == nrCols){
    if(!nextAvailablePosChild(lastActUnitVal)) return 0;
    return 1;
  }

  while(nextAvailablePosChild(lastActUnitVal)){
    if(isCanonical()) return 1;
    
    lastActUnitVal = lastActUnitVal + 1;
    toParent();
  }

  return 0;
}

int RowAsUnits::toFirstCanonicalChild(){
  if(units.size() == 0){
    units.push_back(0);
    pivot = 0;
    return 1;
  }
  
  if(units.size() == 1){
    units.push_back(1);
    pivot = 1;
    return 1;
  }
  
  int n = units.size();
  
  int newLastUnit = units[n-1] + units[pivot] - units[pivot-1];
  int firstInterval = units[1] - units[0];
  
  int newLastInterval = nrCols - newLastUnit;
  
  if(newLastInterval < firstInterval) return 0;
  if((newLastInterval == firstInterval) && (pivot != n-1)) return 0;
  
  if(pivot+1 < n){
    int chasingInterval = newLastInterval;
    int leadingInterval = units[pivot+1] - units[pivot];
    
    int it = 0;
    while (( pivot+it+2 < n ) && (leadingInterval == chasingInterval)){
         chasingInterval = units[it+1]       - units[it];
         leadingInterval = units[pivot+it+2] - units[pivot+it+1];
         it = it + 1;
    }
    if (chasingInterval < leadingInterval) {
      if ( nrCols - (newLastUnit+1) > firstInterval ) {
          newLastUnit = newLastUnit + 1;
          pivot = 0; 
      }
      else return 0;
    }
  }
  units.push_back(newLastUnit);
  pivot = pivot + 1;
  return 1;
}

int RowAsUnits::nextAvailablePosChild(int lastActUnitVal){
  if(units.size() == 0){
    units.push_back(0);
    pivot = 0;
    return 1;
  }

  int newLastUnit = lastActUnitVal + 1;
  if(newLastUnit == nrCols) return 0;
  units.push_back(newLastUnit); 
  pivot = pivot + 1;

  return 1;
}

/* ---------------------------------------------------------------------------------- */
                            /* row functions (sibling-related) */
/* ---------------------------------------------------------------------------------- */

int RowAsUnits::toSibling(){
  int lastActUnitVal;

  if(period == 1){
    if(!toCanonicalSibling()) return 0;
    return 1;
  }else if(period == nrCols){
    lastActUnitVal = units.back();
    if(!nextAvailablePosSibling(lastActUnitVal)) return 0;
    return 1;
  }
  
  lastActUnitVal = units.back();
  while(nextAvailablePosSibling(lastActUnitVal)){
    if(isCanonical()) return 1;
    lastActUnitVal = lastActUnitVal + 1;
  }
  return 0;
}

int RowAsUnits::toCanonicalSibling(){
  int n = units.size();
  
  if(n == 1) return 0;
    
  int firstInterval = units[1] - units[0];
  int newLastInterval = nrCols - (units[n - 1] + 1);

  if(newLastInterval > firstInterval){ 
    units[n-1] = units[n-1]+1;
    pivot = 1;
    
    return 1;
  }
  return 0;
}

int RowAsUnits::nextAvailablePosSibling(int lastActUnitVal){
  int n = units.size();
  int newLastUnit;

  if((n == 1) && (lastActUnitVal == nrCols-1)){
    units[0] = 0; // units.clear()?
    pivot = 0;
    return 0;
  } 

  newLastUnit = lastActUnitVal + 1;

  if(newLastUnit >= nrCols) return 0;
  units[n-1] = newLastUnit;
  pivot = 1;

  return 1;
}

/* ---------------------------------------------------------------------------------- */
                              /* row function (parent-related) */
/* ---------------------------------------------------------------------------------- */

int RowAsUnits::toParent(){
  if(units.size() == 1) return 0;
  units.pop_back();
  pivot = 1;
  return 1;
}

void RowAsUnits::printRow(int y){
  int x;
  
  int shiftFactor[5] = {0, 39, 0, 10, 0};
  int piFactor[5] = {55, 25, 1, 27, 15};
  
  FILE *fptr;
  fptr = fopen(FILE_NAME_STATES, "a");
  
  for(size_t i =0; i < units.size(); i++)
    fprintf(fptr, "%d ", units[i]);
  fprintf(fptr, "\n");
  
  fprintf(fptr, "x[%d]:", y);
  for(size_t i =0; i < units.size(); i++){
    x = (units[i]*piFactor[y] + shiftFactor[y])&63;
    fprintf(fptr, "%d ", x);
  }
  fprintf(fptr, "\n\n");

  fclose(fptr);

  return;  
}

void StateAsUnits::printState(){
  FILE *fptr;
  for(size_t i =0; i < actRows.size(); i++){
    fptr = fopen(FILE_NAME_STATES, "a");
    fprintf(fptr, "z(%d):", actRowYCoordinate[i]);
    fclose(fptr);
    actRows[i].printRow(actRowYCoordinate[i]);
  }
  fprintf(fptr, "\n"); 
  fptr = fopen(FILE_NAME_STATES, "a");

  for(int j = 0; j < 64; j++){
    fprintf(fptr, "%d ", stateAtA[j]);
  }
  fprintf(fptr, "nr:%d\n", getNrOfActColAtA());
  fprintf(fptr, "\n\n"); 
  fclose(fptr);
  return;  
}


/* ---------------------------------------------------------------------------------- */
                                  /* State functions */
/* ---------------------------------------------------------------------------------- */

StateAsUnits::StateAsUnits(){
  RowAsUnits row;
  actRows.push_back(row);
  actRows[0].units.push_back(0);
  actRowYCoordinate.push_back(0);
  /* init Shadow */
  addUnitToShadow( getXOfLastUnit(), actRowYCoordinate.back()); 
  initMaskInX();
}

/* Shadow-related functions */
int StateAsUnits::getTotalActCol(int actColAtB){

  int actColAtA = getNrOfActColAtA();
  if (weightedWeights == 11){
    if(actColAtA + actColAtB <= limit) 
        return 1; 
  }
  else if (weightedWeights == 12){
    if(actColAtA + 2*actColAtB <= limit) 
        return 1; 
    }
  else if (weightedWeights == 21){
    if(2*actColAtA + actColAtB <= limit) 
        return 1; 
    }
  else{
      if(actColAtA + 2*actColAtB <= limit || 2*actColAtA + actColAtB <= limit) 
          return 1;      
  }
  return 0;
}

int StateAsUnits::getNrActColAtB(){
  return getNrOfActColAtB();
}

int StateAsUnits::getNrActColAtA(){
  return getNrOfActColAtA();
}

int StateAsUnits::getTotalWeight(int weightAtB){

  int revWeight = weightAtA();
  if (weightedWeights == 11){
    if(revWeight + weightAtB <= 2*limit) 
        return 1; 
  }
  else if (weightedWeights == 12){
    if(revWeight + 2*weightAtB <= 2*limit) 
        return 1; 
    }
  else if (weightedWeights == 21){
    if(2*revWeight + weightAtB < 2*limit) 
        return 1; 
    }
  else{
      if(revWeight + 2*weightAtB  <= 2*limit || 2*revWeight + weightAtB < 2*limit) 
          return 1;      
  } 
  return 0;
}

int StateAsUnits::getWeightAtB(){
  return weightAtB();
}

int StateAsUnits::getReverseWeight(){
  return weightAtA();
}

/* -------------------------------------------------- */

int StateAsUnits::nextState(){
  if(toFirstChild())
  {
    addUnitToShadow( getXOfLastUnit(), actRowYCoordinate.back() );
    if(checkScore())
      return 1;
  }    
  
  do
  {
    while(toSibling())
    {
      addUnitToShadow( getXOfLastUnit(), actRowYCoordinate.back() );
      if( checkScore() )
        return 1;
    }
  }while(toParent());
  return 0;
}

int StateAsUnits::toFirstChild(){
  if(!appendEmptyRow()) return 0;
  return actRows.back().toFirstChild();
}

int StateAsUnits::toSibling(){
  if( checkScoreSibling(getXOfLastUnit(), actRowYCoordinate.back()) ) 
  {
    if(actRows[actRows.size()-1].toFirstChild()) return 1;  
  }

  rmUnitToShadow( getXOfLastUnit(), actRowYCoordinate.back() );  

  while(!actRows.back().toSibling()){
    if(!actRows.back().toParent()){
      if(!incrementYCoordinate()) return 0;

      return actRows.back().toFirstChild();
    }
    rmUnitToShadow( getXOfLastUnit(), actRowYCoordinate.back() );    
  }
  return 1;
}

int StateAsUnits::toParent(){
  actRows.pop_back();
  actRowYCoordinate.pop_back();
  if(actRows.size() == 0) return 0;
  return 1;    
}


int StateAsUnits::appendEmptyRow(){
  int periodOfState, periodMax;

  if(actRowYCoordinate.back() == (nrRows-1)) return 0;
  
  periodOfState = getPeriodOfState();

  if(periodOfState == nrCols){
    periodMax = periodOfState;
  }else{
    periodMax = actRows.back().getPeriodOfRow();
    if(periodOfState>periodMax)
      periodMax = periodOfState;
  }

  RowAsUnits row(periodMax);
  actRows.push_back(row);
  actRowYCoordinate.push_back(actRowYCoordinate.back() + 1);

  return 1;
}

int StateAsUnits::incrementYCoordinate(){
  if(actRowYCoordinate.back() == nrRows-1) return 0;

  actRowYCoordinate.back() = actRowYCoordinate.back() + 1;
  actRows.back().units.clear();
  return 1;
}

int StateAsUnits::getXOfLastUnit(){
  return actRows.back().units[actRows.back().units.size() - 1];
}

/* ---------------------------------------------------------------------------------- */
                              /* Compute period of a row */
/* ---------------------------------------------------------------------------------- */

int RowAsUnits::getPeriodOfRow(){
  vector<int> shiftedRow;
  int unit;

  for( int i=1; i<nrCols; ){
    for(size_t j = 0; j < units.size(); j++){
      unit = (units[j] - i) & (nrCols-1);
      shiftedRow.push_back(unit);
    }
    sort(shiftedRow.begin(), shiftedRow.end());
    if(isSymmetric(units, shiftedRow))
      return i;
    i = i+1;
    shiftedRow.clear();
  }
  return nrCols;
}

/* ---------------------------------------------------------------------------------- */
                              /* Canonicity check */
/* ---------------------------------------------------------------------------------- */
int RowAsUnits::isCanonical(){
  vector<int> shiftedRow;
  int i=2;
  int unit;

  for(int p = period; p<nrCols; ){
    for (size_t j = 0; j<units.size();j++ )
    {
      unit = (units[j] - p) & (nrCols-1);
      shiftedRow.push_back(unit);
    }
    sort(shiftedRow.begin(), shiftedRow.end());
    if(compareRow(units, shiftedRow) == 0)
      return 0;

    p = i*period;
    i = i + 1;
    shiftedRow.clear();
  }
  
  return 1;
}

/* ---------------------------------------------------------------------------------- */
              /* Functions used for periodicity and canonicity of a row */
/* ---------------------------------------------------------------------------------- */
int isSymmetric(vector<int> row, vector<int> shiftedRow){
  for(size_t i = 0; i<row.size(); i++){
    if(row[i] != shiftedRow[i]) return 0;
  }
  return 1;
}

int compareRow(vector<int> row, vector<int> shiftedRow){
  for(size_t i = 0; i < row.size(); i++){
    if(row[i] < shiftedRow[i]) return 1;
    if(shiftedRow[i] < row[i]) return 0;
  }
  return 1;
} 

/* ---------------------------------------------------------------------------------- */
                              /* Compute period of the state */
/* ---------------------------------------------------------------------------------- */

int StateAsUnits::getPeriodOfState(){
  int periodMax = actRows[0].period;

  for(size_t i = 1; i < actRows.size(); i++){
    if(actRows[i].period > periodMax)
      periodMax = actRows[i].period;
  }

  return periodMax;
}
