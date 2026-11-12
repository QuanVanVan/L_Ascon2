/*
AsconTrailTool, a set of C++ classes for analyzing ASCON.

For specifications, please refer to https://tosc.iacr.org/index.php/ToSC/article/view/9975

Implementation by Solane El Hirch, hereby denoted as "the implementer".
*/

#ifndef HISTOGRAM_H
#define HISTOGRAM_H

#include <cstdint>
#include <cstring>

#include "config.h"

using namespace std;

class Histogram{
public:
 	long long histogramOfActCol[nrRows*nrCols*limit];
 	long long histogramOfTrailCores[nrRows*nrCols*2*limit];
public:
	Histogram(){
		memset(histogramOfActCol, 0, (nrRows*nrCols*limit)*sizeof(long long*));
		memset(histogramOfTrailCores, 0, (nrRows*nrCols*2*limit)*sizeof(long long*));
	}

	void setValueInHistogramActCol(int in, int hw);
	void setValueInHistogramTrailCores(int in, int hw);
	void getHistogram(string fileName, string fileDesc, string in, string out, int lim);
};

#endif /* HISTOGRAM_H */
