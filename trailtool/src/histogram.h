/*
AsconTrailTool, a set of C++ classes for analyzing ASCON.

For specifications, please refer to https://tosc.iacr.org/index.php/ToSC/article/view/9975

Implementation by Solane El Hirch, hereby denoted as "the implementer".
FIX (DATN): the original used fixed-size arrays with a row stride of 2*limit for the
trail-core histogram but printed it with stride limitWeight (wrong when limitWeight is
odd, e.g. 39), and could wrap entries into the next row when w(b) >= 2*limit.  Rows and
columns are now sized from the actual maximal values and the stride is stored explicitly.
*/

#ifndef HISTOGRAM_H
#define HISTOGRAM_H

#include <cstdint>
#include <string>
#include <vector>

#include "config.h"

using namespace std;

class Histogram{
public:
	int dimActCol;       // number of active columns at a / at b: 1..dimActCol
	int dimCores;        // weights at a / at b: 1..dimCores
	vector<long long> histogramOfActCol;
	vector<long long> histogramOfTrailCores;
public:
	Histogram()
		: dimActCol(nrCols), dimCores(2*limitWeight+8),
		  histogramOfActCol(dimActCol*dimActCol, 0), histogramOfTrailCores(dimCores*dimCores, 0) {}

	void setValueInHistogramActCol(int in, int hw);
	void setValueInHistogramTrailCores(int in, int hw);
	void getHistogram(string fileName, string fileDesc, string in, string out, int lim, bool activeColumns);
};

#endif /* HISTOGRAM_H */
