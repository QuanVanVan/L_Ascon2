/*
AsconTrailTool, a set of C++ classes for analyzing ASCON.

For specifications, please refer to https://tosc.iacr.org/index.php/ToSC/article/view/9975

Implementation by Solane El Hirch, hereby denoted as "the implementer".
*/

#include <iostream>
#include <cstdint>

#include "histogram.h"

using namespace std;

/* ------------------------------------------------------------ */
            /* 2-round trail cores histogram functions */
/* ------------------------------------------------------------ */
void Histogram::setValueInHistogramActCol(int in, int hw){
	if (in >= 1 && hw >= 1 && in <= dimActCol && hw <= dimActCol)
		histogramOfActCol[(in-1)*dimActCol + (hw-1)]++;
	return;
}

void Histogram::setValueInHistogramTrailCores(int in, int hw){
	if (in >= 1 && hw >= 1 && in <= dimCores && hw <= dimCores)
		histogramOfTrailCores[(in-1)*dimCores + (hw-1)]++;
	return;
}

void Histogram::getHistogram(string fileName, string fileDesc, string in, string out, int lim, bool activeColumns){
	FILE *fptr = fopen(fileName.c_str(), "a");
	int i, j;
	long long val;
	const int dim = activeColumns ? dimActCol : dimCores;
	const vector<long long>& h = activeColumns ? histogramOfActCol : histogramOfTrailCores;
	if (lim > dim) lim = dim;

	fprintf(fptr, "%s,", fileDesc.c_str());
	fprintf(fptr, ",\n"); 
	fprintf(fptr, "(limit <= %d)", lim);
	fprintf(fptr, ",\n"); 

	fprintf(fptr, "%s", out.c_str());
	fprintf(fptr, ",");

	for(i = 1; i <= lim; i++){
		fprintf(fptr, "%d,", i);
	}
	fprintf(fptr, ",\n"); 
	fprintf(fptr, "%s", in.c_str());
	fprintf(fptr, ",\n");

	for(i = 0; i < lim; i++){
		fprintf(fptr, "%d,", i+1);
		for(j = 0; j < lim; j++){
			val = h[i*dim+j];
			if(val == 0) fprintf(fptr, " ,");
			else fprintf(fptr, "%lld,", val);
		}
		fprintf(fptr, ",\n");
	}
	fprintf(fptr, ",\n");
	fclose(fptr);

	return;
}
