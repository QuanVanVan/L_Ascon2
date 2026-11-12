/*
DATN extension of AsconTrailTool: generic exhaustive generation of 2-round trail cores
(see generic2R.cpp for the method and its correctness argument).
*/
#ifndef GENERIC2R_H
#define GENERIC2R_H

#include <string>

/* Writes all canonical 2-round trail cores with ca*wrev(a) + cb*w(b) <= T for the layer g_layer,
   (ca,cb) = (1,1), (1,2), (2,1) for weightedW = 11, 12, 21, in the file format of the original tool. */
int generateTwoRoundTrailCoresGeneric(int weightedW, int T, const std::string& oFileName);

#endif
