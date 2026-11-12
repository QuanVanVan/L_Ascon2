/*
AsconTrailTool, a set of C++ classes for analyzing ASCON.

For specifications, please refer to https://tosc.iacr.org/index.php/ToSC/article/view/9975

Implementation by Solane El Hirch, hereby denoted as "the implementer".
*/

#ifndef CONFIG_H
#define CONFIG_H

#include <cstdint>
#include <vector>

#define UINT64 uint64_t 

#ifndef DTS
#define DTS 0 // 1 Differential Trail Search; 0 Linear Trail Search
#endif

// debug
#define FILE_NAME_TERM "terminal.txt"
#define FILE_NAME_STATES "states.txt"

#if DTS == 1
    #define FILE_NAME_COL "DTS-columnHistogramAscon.csv"
    #define FILE_NAME_WEIGHT "DTS-trailCoresHistogramAscon.csv"
    #define FILE_DESCRIPTION_NRCOL "DTS Histogram of ASCON with the number of active columns" 
    #define FILE_DESCRIPTION_WEIGHT "DTS Histogram of ASCON: number of trail cores" 
#else
    #define FILE_NAME_COL "LTS-columnHistogramAscon.csv"
    #define FILE_NAME_WEIGHT "LTS-trailCoresHistogramAscon.csv"
    #define FILE_DESCRIPTION_NRCOL "LTS Histogram of ASCON with the number of active columns" 
    #define FILE_DESCRIPTION_WEIGHT "LTS Histogram of ASCON: number of trail cores" 
#endif

#define IN_VAL_COL "#active columns at input"
#define OUT_VAL_COL "#active columns at output"

#define IN_VAL_CORES "reverse weight"
#define OUT_VAL_CORES "weight"

#define countBits(output) \
    ({ UINT64 cp = output; \
    cp = cp - ((cp>>1)&0x5555555555555555); \
    cp = (cp&0x3333333333333333)+((cp>>2)&0x3333333333333333); \
    cp = (cp&0x0f0f0f0f0f0f0f0f)+((cp>>4)&0x0f0f0f0f0f0f0f0f); \
    cp += (cp>>32); \
    cp += (cp>>16); \
    cp += (cp>>8); \
    int count = cp&0xff;\
    count;})   

static const int nrRows = 5;
static const int nrCols = 64;

#ifndef LIMIT_WEIGHT
#define LIMIT_WEIGHT 20
#endif
#ifndef WEIGHTED_WEIGHTS
#define WEIGHTED_WEIGHTS 11
#endif
static const int limitWeight = LIMIT_WEIGHT;
static const int weightedWeights = WEIGHTED_WEIGHTS; // 11 for w0+w1, 12 for w0+2w1, 21 for 2w0+w1
static const int limit = int(limitWeight/2);

#endif /* CONFIG_H */
