/*
AsconTrailTool, a set of C++ classes for analyzing ASCON.

For specifications, please refer to https://tosc.iacr.org/index.php/ToSC/article/view/9975

Implementation by Solane El Hirch, hereby denoted as "the implementer".
*/

 #ifndef TREECANONICAL_H
#define TREECANONICAL_H

#include <cstdint>
#include <vector>

#include "asconColScore.h"

using namespace std;

class RowAsUnits{
public:
  vector<int> units;
  int pivot;
  int period;
public:
  RowAsUnits()
    :pivot(0), period(1) {}
  RowAsUnits(int p)
    :pivot(0), period(p) {} // FIX: pivot was uninitialized

  int toFirstChild(); 
  int toFirstCanonicalChild();
  int nextAvailablePosChild(int lastActUnitVal); 
  int toSibling();
  int toCanonicalSibling();
  int toParent();
  int nextAvailablePosSibling(int lastActUnitVal); 
  int getPeriodOfRow();
  int isCanonical(); 

  void printRow(int y);
};

class StateAsUnits : public Shadow {
public:
  vector<int> actRowYCoordinate;
  vector<RowAsUnits> actRows;
  // DATN: if >= 0, the traversal is restricted to the sub-tree of canonical states whose
  // first active row is restrictFirstRow (used to split the search over OpenMP threads).
  int restrictFirstRow;
  // DATN: finer split. -1: no restriction; 0: the first active row has a single active bit
  // (units {0}); j > 0: the first active row starts with units {0, j}. Only used together
  // with restrictFirstRow >= 0. For a canonical state j ranges over 1..nrCols/2.
  int restrictSecondUnit;

public:
  StateAsUnits();
  StateAsUnits(int firstRow, int secondUnit = -1);
  bool leavesSubTree() const;
  
  int nextState();
  int toFirstChild();
  int toSibling(); 
  int toParent(); 
  int appendEmptyRow();
  int getPeriodOfState();
  int incrementYCoordinate();
  int getXOfLastUnit(); 

  int getTotalWeight(int hwAtB);
  int getTotalActCol(int actColAtB);
  int getWeightAtB();
  int getNrActColAtB();
  int getNrActColAtA();
  int getReverseWeight();

  void printState();
};

int isSymmetric(vector<int> row, vector<int> shiftedRow);
int compareRow(vector<int> row, vector<int> shiftedRow);

#endif /* TREECANONICAL_H */
