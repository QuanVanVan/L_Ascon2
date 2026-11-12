/*
AsconTrailTool, a set of C++ classes for analyzing ASCON.

For specifications, please refer to https://tosc.iacr.org/index.php/ToSC/article/view/9975

Implementation by Silvia Mella, hereby denoted as "the implementer".
*/

#include "trailExtension.h"
#include "genericLayer.h"

#include <omp.h>

// DATN: the linear layer is selected at run time (genericLayer.h); for "ascon" the original
// hard-coded functions are used, so the behaviour of the original tool is unchanged.
#if DTS == 1
void linearLayer(State& s)
{
    g_layer.applyForward(s);
}

void inverseLinearLayer(State& s)
{
    g_layer.applyInverse(s);
}
#else
void linearLayer(State& s)
{
    g_layer.applyTranspose(s);
}

void inverseLinearLayer(State& s)
{
    g_layer.applyInverseTranspose(s);
}
#endif

/*
 We define the ordering of the bit coordinates as [z, x], i.e., lexicographical order first on z then on x.
 This function sets 1 to all bits whose column position is strictly less than z given in parameters.
 */
void StateMask::setMaskXZ(unsigned int x, unsigned int z)
{
    clear();
    for (unsigned int ix = 0; ix < nrRows; ix++) {
        if (ix < x)
            rows[ix] = ~(uint64_t)0;
        else if (ix == x)
            rows[ix] = ((uint64_t)1 << z) - 1;
    }
}

void StateMask::invert()
{
    for (unsigned int i = 0; i < (unsigned int)nrRows; i++)
        rows[i] = ~rows[i];
}

bool StateMask::nonEmptyIntersection(State& other)
{
    for (unsigned int ix = 0; ix < nrRows; ix++)
        if ((getRow(ix) & other.getRow(ix)) != 0)
            return true;
    return false;
}

Propagation::Propagation()
{
#if DTS == 1
    // Input           =   0  1  2  3  4  5  6  7  8  9 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26 27 28 29 30 31
    weightListPerInput = { 0, 3, 3, 3, 2, 3, 4, 3, 3, 4, 4, 4, 2, 4, 3, 3, 2, 2, 4, 2, 3, 3, 4, 3, 4, 3, 4, 4, 3, 3, 4, 3 };

    directOutputListPerInput = { {0x00},
        {0x09, 0x0B, 0x0D, 0x0F, 0x18, 0x1A, 0x1C, 0x1E},
        {0x11, 0x13, 0x15, 0x17, 0x19, 0x1B, 0x1D, 0x1F},
        {0x01, 0x05, 0x09, 0x0D, 0x10, 0x14, 0x18, 0x1C},
        {0x06, 0x0E, 0x16, 0x1E},
        {0x11, 0x13, 0x14, 0x16, 0x18, 0x1A, 0x1D, 0x1F},
        {0x01, 0x03, 0x05, 0x07, 0x09, 0x0B, 0x0D, 0x0F, 0x11, 0x13, 0x15, 0x17, 0x19, 0x1B, 0x1D, 0x1F},
        {0x02, 0x03, 0x06, 0x07, 0x0A, 0x0B, 0x0E, 0x0F},
        {0x06, 0x07, 0x0E, 0x0F, 0x16, 0x17, 0x1E, 0x1F},
        {0x01, 0x03, 0x04, 0x06, 0x08, 0x0A, 0x0D, 0x0F, 0x10, 0x12, 0x15, 0x17, 0x19, 0x1B, 0x1C, 0x1E},
        {0x01, 0x02, 0x04, 0x07, 0x09, 0x0A, 0x0C, 0x0F, 0x11, 0x12, 0x14, 0x17, 0x19, 0x1A, 0x1C, 0x1F},
        {0x02, 0x03, 0x06, 0x07, 0x0A, 0x0B, 0x0E, 0x0F, 0x12, 0x13, 0x16, 0x17, 0x1A, 0x1B, 0x1E, 0x1F},
        {0x01, 0x08, 0x10, 0x19},
        {0x01, 0x03, 0x05, 0x07, 0x08, 0x0A, 0x0C, 0x0E, 0x10, 0x12, 0x14, 0x16, 0x19, 0x1B, 0x1D, 0x1F},
        {0x01, 0x02, 0x04, 0x07, 0x11, 0x12, 0x14, 0x17},
        {0x08, 0x09, 0x0C, 0x0D, 0x18, 0x19, 0x1C, 0x1D},
        {0x09, 0x0B, 0x18, 0x1A},
        {0x11, 0x13, 0x15, 0x17},
        {0x01, 0x03, 0x05, 0x07, 0x09, 0x0B, 0x0D, 0x0F, 0x10, 0x12, 0x14, 0x16, 0x18, 0x1A, 0x1C, 0x1E},
        {0x02, 0x04, 0x0A, 0x0C},
        {0x04, 0x05, 0x06, 0x07, 0x0C, 0x0D, 0x0E, 0x0F},
        {0x05, 0x07, 0x09, 0x0B, 0x11, 0x13, 0x1D, 0x1F},
        {0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F},
        {0x02, 0x04, 0x0A, 0x0C, 0x12, 0x14, 0x1A, 0x1C},
        {0x04, 0x05, 0x06, 0x07, 0x0C, 0x0D, 0x0E, 0x0F, 0x14, 0x15, 0x16, 0x17, 0x1C, 0x1D, 0x1E, 0x1F},
        {0x03, 0x06, 0x08, 0x0D, 0x10, 0x15, 0x1B, 0x1E},
        {0x01, 0x02, 0x05, 0x06, 0x08, 0x0B, 0x0C, 0x0F, 0x11, 0x12, 0x15, 0x16, 0x18, 0x1B, 0x1C, 0x1F},
        {0x02, 0x03, 0x04, 0x05, 0x0A, 0x0B, 0x0C, 0x0D, 0x12, 0x13, 0x14, 0x15, 0x1A, 0x1B, 0x1C, 0x1D},
        {0x01, 0x03, 0x08, 0x0A, 0x10, 0x12, 0x19, 0x1B},
        {0x03, 0x05, 0x08, 0x0E, 0x10, 0x16, 0x1B, 0x1D},
        {0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F},
        {0x02, 0x03, 0x04, 0x05, 0x12, 0x13, 0x14, 0x15} };

    affinePerInput = { AffineSpaceOfColumns(0,{}),
        AffineSpaceOfColumns(0x09, {0x02, 0x04, 0x11}),
        AffineSpaceOfColumns(0x11, {0x02, 0x04, 0x08}),
        AffineSpaceOfColumns(0x01, {0x04, 0x08, 0x11}),
        AffineSpaceOfColumns(0x06, {0x08, 0x10}),
        AffineSpaceOfColumns(0x11, {0x02, 0x09, 0x0C}),
        AffineSpaceOfColumns(0x01, {0x02, 0x04, 0x08, 0x10}),
        AffineSpaceOfColumns(0x02, {0x01, 0x04, 0x08}),
        AffineSpaceOfColumns(0x06, {0x01, 0x08, 0x10}),
        AffineSpaceOfColumns(0x01, {0x02, 0x11, 0x14, 0x18}),
        AffineSpaceOfColumns(0x01, {0x05, 0x06, 0x08, 0x10}),
        AffineSpaceOfColumns(0x02, {0x01, 0x04, 0x08, 0x10}),
        AffineSpaceOfColumns(0x01, {0x11, 0x18}),
        AffineSpaceOfColumns(0x01, {0x02, 0x04, 0x11, 0x18}),
        AffineSpaceOfColumns(0x01, {0x05, 0x06, 0x10}),
        AffineSpaceOfColumns(0x08, {0x01, 0x04, 0x10}),
        AffineSpaceOfColumns(0x09, {0x02, 0x11}),
        AffineSpaceOfColumns(0x11, {0x02, 0x04}),
        AffineSpaceOfColumns(0x01, {0x02, 0x04, 0x08, 0x11}),
        AffineSpaceOfColumns(0x02, {0x06, 0x08}),
        AffineSpaceOfColumns(0x04, {0x01, 0x02, 0x08}),
        AffineSpaceOfColumns(0x05, {0x02, 0x14, 0x18}),
        AffineSpaceOfColumns(0x10, {0x01, 0x02, 0x04, 0x08}),
        AffineSpaceOfColumns(0x02, {0x06, 0x08, 0x10}),
        AffineSpaceOfColumns(0x04, {0x01, 0x02, 0x08, 0x10}),
        AffineSpaceOfColumns(0x08, {0x05, 0x16, 0x18}),
        AffineSpaceOfColumns(0x01, {0x04, 0x09, 0x0A, 0x10}),
        AffineSpaceOfColumns(0x02, {0x01, 0x06, 0x08, 0x10}),
        AffineSpaceOfColumns(0x01, {0x02, 0x11, 0x18}),
        AffineSpaceOfColumns(0x08, {0x06, 0x15, 0x18}),
        AffineSpaceOfColumns(0x08, {0x01, 0x02, 0x04, 0x10}),
        AffineSpaceOfColumns(0x02, {0x01, 0x06, 0x10}) };

    // these are ordered by their weight
    reverseOutputListPerInput = { {0x00},
        {0x0C, 0x03, 0x0E, 0x1C, 0x06, 0x09, 0x0A, 0x0D, 0x12, 0x1A},
        {0x13, 0x07, 0x0E, 0x17, 0x1F, 0x0A, 0x0B, 0x1A, 0x1B},
        {0x07, 0x19, 0x1C, 0x1D, 0x1F, 0x06, 0x09, 0x0B, 0x0D, 0x12, 0x1B},
        {0x13, 0x0E, 0x14, 0x17, 0x1F, 0x09, 0x0A, 0x18, 0x1B},
        {0x03, 0x14, 0x15, 0x1D, 0x1F, 0x06, 0x0D, 0x12, 0x18, 0x1A, 0x1B},
        {0x04, 0x07, 0x08, 0x14, 0x19, 0x09, 0x0B, 0x18, 0x1A},
        {0x07, 0x08, 0x0E, 0x14, 0x15, 0x06, 0x0A, 0x0B, 0x0D, 0x12, 0x18},
        {0x0C, 0x0F, 0x19, 0x1C, 0x1D, 0x09, 0x0D, 0x1A, 0x1E},
        {0x10, 0x01, 0x03, 0x0F, 0x15, 0x06, 0x0A, 0x12, 0x1E},
        {0x13, 0x07, 0x17, 0x1C, 0x09, 0x0A, 0x0B, 0x0D, 0x1B, 0x1E},
        {0x10, 0x01, 0x07, 0x15, 0x06, 0x0B, 0x12, 0x1A, 0x1B, 0x1E},
        {0x13, 0x0F, 0x14, 0x17, 0x0A, 0x0D, 0x18, 0x1A, 0x1B, 0x1E},
        {0x01, 0x03, 0x0F, 0x14, 0x19, 0x06, 0x09, 0x12, 0x18, 0x1B, 0x1E},
        {0x04, 0x07, 0x08, 0x14, 0x1D, 0x0B, 0x0D, 0x18, 0x1E},
        {0x01, 0x07, 0x08, 0x14, 0x06, 0x09, 0x0A, 0x0B, 0x12, 0x18, 0x1A, 0x1E},
        {0x0C, 0x03, 0x19, 0x1C, 0x1D, 0x09, 0x0D, 0x12, 0x16},
        {0x11, 0x02, 0x05, 0x0E, 0x15, 0x06, 0x0A, 0x16, 0x1A},
        {0x0E, 0x17, 0x1C, 0x1F, 0x09, 0x0A, 0x0B, 0x0D, 0x12, 0x16, 0x1A, 0x1B},
        {0x11, 0x02, 0x05, 0x15, 0x1F, 0x06, 0x0B, 0x16, 0x1B},
        {0x03, 0x05, 0x0E, 0x17, 0x1F, 0x0A, 0x0D, 0x12, 0x16, 0x18, 0x1B},
        {0x11, 0x02, 0x19, 0x1F, 0x06, 0x09, 0x16, 0x18, 0x1A, 0x1B},
        {0x04, 0x05, 0x08, 0x1D, 0x0B, 0x0D, 0x12, 0x16, 0x18, 0x1A},
        {0x11, 0x02, 0x08, 0x0E, 0x06, 0x09, 0x0A, 0x0B, 0x16, 0x18},
        {0x10, 0x01, 0x03, 0x05, 0x0F, 0x12, 0x16, 0x1A, 0x1E},
        {0x0C, 0x02, 0x0F, 0x1C, 0x06, 0x09, 0x0A, 0x0D, 0x16, 0x1E},
        {0x10, 0x01, 0x05, 0x17, 0x0A, 0x0B, 0x12, 0x16, 0x1B, 0x1E},
        {0x02, 0x19, 0x1C, 0x1D, 0x06, 0x09, 0x0B, 0x0D, 0x16, 0x1A, 0x1B, 0x1E},
        {0x01, 0x03, 0x0F, 0x17, 0x09, 0x0A, 0x12, 0x16, 0x18, 0x1A, 0x1B, 0x1E},
        {0x02, 0x05, 0x0F, 0x15, 0x1D, 0x06, 0x0D, 0x16, 0x18, 0x1B, 0x1E},
        {0x04, 0x01, 0x08, 0x19, 0x09, 0x0B, 0x12, 0x16, 0x18, 0x1E},
        {0x02, 0x05, 0x08, 0x15, 0x06, 0x0A, 0x0B, 0x0D, 0x16, 0x18, 0x1A, 0x1E} };

    // the weight of each compatible input pattern
    weightReverseOutputListPerInput = { {0},      // 0
        {2, 3, 3, 3, 4, 4, 4, 4, 4, 4},           // 1
        {2, 3, 3, 3, 3, 4, 4, 4, 4},              // 2
        {3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4},        // 3
        {2, 3, 3, 3, 3, 4, 4, 4, 4},              // 4
        {3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4},        // 5
        {2, 3, 3, 3, 3, 4, 4, 4, 4},              // 6
        {3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4},        // 7
        {2, 3, 3, 3, 3, 4, 4, 4, 4},              // 8
        {2, 3, 3, 3, 3, 4, 4, 4, 4},              // 9
        {2, 3, 3, 3, 4, 4, 4, 4, 4, 4},           //10
        {2, 3, 3, 3, 4, 4, 4, 4, 4, 4},           //11
        {2, 3, 3, 3, 4, 4, 4, 4, 4, 4},           //12
        {3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4},        //13
        {2, 3, 3, 3, 3, 4, 4, 4, 4},              //14
        {3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4},     //15
        {2, 3, 3, 3, 3, 4, 4, 4, 4},              //16
        {2, 3, 3, 3, 3, 4, 4, 4, 4},              //17
        {3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4},     //18
        {2, 3, 3, 3, 3, 4, 4, 4, 4},              //19
        {3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4},        //20
        {2, 3, 3, 3, 4, 4, 4, 4, 4, 4},           //21
        {2, 3, 3, 3, 4, 4, 4, 4, 4, 4},           //22
        {2, 3, 3, 3, 4, 4, 4, 4, 4, 4},           //23
        {2, 3, 3, 3, 3, 4, 4, 4, 4},              //24
        {2, 3, 3, 3, 4, 4, 4, 4, 4, 4},           //25
        {2, 3, 3, 3, 4, 4, 4, 4, 4, 4},           //26
        {3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4},     //27
        {3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4},     //28
        {3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4},        //29
        {2, 3, 3, 3, 4, 4, 4, 4, 4, 4},           //30
        {3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4} };   //31

    //                            0  1  2  3  4  5  6  7  8  9 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26 27 28 29 30 31
    minRevWeightListPerOutput = { 0, 2, 2, 3, 2, 3, 2, 3, 2, 2, 2, 2, 2, 3, 2, 3, 2, 2, 3, 2, 3, 2, 2, 2, 2, 2, 2, 3, 3, 3, 2, 3 };
#else
    // Input           =   0  1  2  3  4  5  6  7  8  9 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26 27 28 29 30 31
    weightListPerInput = { 0, 2, 2, 4, 2, 4, 2, 2, 2, 4, 4, 4, 4, 4, 4, 4, 2, 2, 4, 4, 4, 4, 4, 4, 4, 2, 4, 4, 4, 2, 4, 4 };

    directOutputListPerInput = { {0x00}, // 0x00
        {0x03, 0x0B, 0x12, 0x1A}, // 0x01
        {0x0C, 0x0F, 0x1C, 0x1F}, // 0x02
        {0x04, 0x05, 0x06, 0x07, 0x0C, 0x0D, 0x0E, 0x0F, 0x14, 0x15, 0x16, 0x17, 0x1C, 0x1D, 0x1E, 0x1F}, // 0x03
        {0x0C, 0x0D, 0x0E, 0x0F}, // 0x04
        {0x04, 0x05, 0x06, 0x07, 0x0C, 0x0D, 0x0E, 0x0F, 0x14, 0x15, 0x16, 0x17, 0x1C, 0x1D, 0x1E, 0x1F}, // 0x05
        {0x01, 0x02, 0x10, 0x13}, // 0x06
        {0x02, 0x09, 0x13, 0x18}, // 0x07
        {0x11, 0x17, 0x1B, 0x1D}, // 0x08
        {0x01, 0x03, 0x05, 0x07, 0x09, 0x0B, 0x0D, 0x0F, 0x10, 0x12, 0x14, 0x16, 0x18, 0x1A, 0x1C, 0x1E}, // 0x09
        {0x01, 0x02, 0x04, 0x07, 0x08, 0x0B, 0x0D, 0x0E, 0x11, 0x12, 0x14, 0x17, 0x18, 0x1B, 0x1D, 0x1E}, // 0x0A
        {0x02, 0x03, 0x06, 0x07, 0x08, 0x09, 0x0C, 0x0D, 0x10, 0x11, 0x14, 0x15, 0x1A, 0x1B, 0x1E, 0x1F}, // 0x0B
        {0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F}, // 0x0C
        {0x01, 0x03, 0x04, 0x06, 0x09, 0x0B, 0x0C, 0x0E, 0x11, 0x13, 0x14, 0x16, 0x19, 0x1B, 0x1C, 0x1E}, // 0x0D
        {0x01, 0x02, 0x04, 0x07, 0x08, 0x0B, 0x0D, 0x0E, 0x10, 0x13, 0x15, 0x16, 0x19, 0x1A, 0x1C, 0x1F}, // 0x0E
        {0x02, 0x03, 0x04, 0x05, 0x08, 0x09, 0x0E, 0x0F, 0x12, 0x13, 0x14, 0x15, 0x18, 0x19, 0x1E, 0x1F}, // 0x0F
        {0x03, 0x0B, 0x16, 0x1E}, // 0x10
        {0x11, 0x15, 0x19, 0x1D}, // 0x11
        {0x01, 0x02, 0x04, 0x07, 0x09, 0x0A, 0x0C, 0x0F, 0x11, 0x12, 0x14, 0x17, 0x19, 0x1A, 0x1C, 0x1F}, // 0x12
        {0x01, 0x02, 0x05, 0x06, 0x09, 0x0A, 0x0D, 0x0E, 0x11, 0x12, 0x15, 0x16, 0x19, 0x1A, 0x1D, 0x1E}, // 0x13
        {0x04, 0x05, 0x06, 0x07, 0x0C, 0x0D, 0x0E, 0x0F, 0x10, 0x11, 0x12, 0x13, 0x18, 0x19, 0x1A, 0x1B}, // 0x14
        {0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F}, // 0x15
        {0x01, 0x02, 0x05, 0x06, 0x09, 0x0A, 0x0D, 0x0E, 0x10, 0x13, 0x14, 0x17, 0x18, 0x1B, 0x1C, 0x1F}, // 0x16
        {0x01, 0x02, 0x05, 0x06, 0x09, 0x0A, 0x0D, 0x0E, 0x10, 0x13, 0x14, 0x17, 0x18, 0x1B, 0x1C, 0x1F}, // 0x17
        {0x01, 0x03, 0x05, 0x07, 0x09, 0x0B, 0x0D, 0x0F, 0x10, 0x12, 0x14, 0x16, 0x18, 0x1A, 0x1C, 0x1E}, // 0x18
        {0x04, 0x06, 0x08, 0x0A}, // 0x19
        {0x01, 0x03, 0x05, 0x07, 0x08, 0x0A, 0x0C, 0x0E, 0x10, 0x12, 0x14, 0x16, 0x19, 0x1B, 0x1D, 0x1F}, // 0x1A
        {0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1A, 0x1B}, // 0x1B
        {0x01, 0x03, 0x04, 0x06, 0x09, 0x0B, 0x0C, 0x0E, 0x10, 0x12, 0x15, 0x17, 0x18, 0x1A, 0x1D, 0x1F}, // 0x1C
        {0x05, 0x07, 0x08, 0x0A}, // 0x1D
        {0x01, 0x03, 0x04, 0x06, 0x08, 0x0A, 0x0D, 0x0F, 0x10, 0x12, 0x15, 0x17, 0x19, 0x1B, 0x1C, 0x1E}, // 0x1E
        {0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1A, 0x1B}, // 0x1F 
    };

    affinePerInput = { AffineSpaceOfColumns(0x00, {}), // 0x00
        AffineSpaceOfColumns(0x03, {0x08, 0x11}), // 0x01
        AffineSpaceOfColumns(0x0C, {0x03, 0x10}), // 0x02
        AffineSpaceOfColumns(0x04, {0x01, 0x02, 0x08, 0x10}), // 0x03
        AffineSpaceOfColumns(0x0C, {0x01, 0x02}), // 0x04
        AffineSpaceOfColumns(0x04, {0x01, 0x02, 0x08, 0x10}), // 0x05
        AffineSpaceOfColumns(0x01, {0x11, 0x12}), // 0x06
        AffineSpaceOfColumns(0x02, {0x11, 0x1A}), // 0x07
        AffineSpaceOfColumns(0x11, {0x0A, 0x0C}), // 0x08
        AffineSpaceOfColumns(0x01, {0x02, 0x04, 0x08, 0x11}), // 0x09
        AffineSpaceOfColumns(0x01, {0x09, 0x0A, 0x0C, 0x10}), // 0x0A
        AffineSpaceOfColumns(0x02, {0x01, 0x04, 0x12, 0x18}), // 0x0B
        AffineSpaceOfColumns(0x10, {0x01, 0x02, 0x04, 0x08}), // 0x0C
        AffineSpaceOfColumns(0x01, {0x02, 0x05, 0x08, 0x10}), // 0x0D
        AffineSpaceOfColumns(0x01, {0x11, 0x12, 0x14, 0x18}), // 0x0E
        AffineSpaceOfColumns(0x02, {0x01, 0x0A, 0x0C, 0x10}), // 0x0F
        AffineSpaceOfColumns(0x03, {0x08, 0x15}), // 0x10
        AffineSpaceOfColumns(0x11, {0x04, 0x08}), // 0x11
        AffineSpaceOfColumns(0x01, {0x05, 0x06, 0x08, 0x10}), // 0x12
        AffineSpaceOfColumns(0x01, {0x03, 0x04, 0x08, 0x10}), // 0x13
        AffineSpaceOfColumns(0x04, {0x01, 0x02, 0x08, 0x14}), // 0x14
        AffineSpaceOfColumns(0x10, {0x01, 0x02, 0x04, 0x08}), // 0x15
        AffineSpaceOfColumns(0x01, {0x04, 0x08, 0x11, 0x12}), // 0x16
        AffineSpaceOfColumns(0x01, {0x04, 0x08, 0x11, 0x12}), // 0x17
        AffineSpaceOfColumns(0x01, {0x02, 0x04, 0x08, 0x11}), // 0x18
        AffineSpaceOfColumns(0x04, {0x02, 0x0C}), // 0x19
        AffineSpaceOfColumns(0x01, {0x02, 0x04, 0x11, 0x18}), // 0x1A
        AffineSpaceOfColumns(0x04, {0x01, 0x02, 0x0C, 0x10}), // 0x1B
        AffineSpaceOfColumns(0x01, {0x02, 0x08, 0x11, 0x14}), // 0x1C
        AffineSpaceOfColumns(0x08, {0x02, 0x0D}), // 0x1D
        AffineSpaceOfColumns(0x01, {0x02, 0x11, 0x14, 0x18}), // 0x1E
        AffineSpaceOfColumns(0x04, {0x01, 0x02, 0x0C, 0x10}), // 0x1F
    };

    // these are ordered by their weight
    reverseOutputListPerInput = { {0x00}, // 0x00
        {0x06, 0x09, 0x0A, 0x0D, 0x0E, 0x12, 0x13, 0x16, 0x17, 0x18, 0x1A, 0x1C, 0x1E}, // 0x01
        {0x06, 0x07, 0x0A, 0x0B, 0x0E, 0x0F, 0x12, 0x13, 0x16, 0x17}, // 0x02
        {0x01, 0x10, 0x09, 0x0B, 0x0D, 0x0F, 0x18, 0x1A, 0x1C, 0x1E}, // 0x03
        {0x19, 0x03, 0x05, 0x0A, 0x0D, 0x0E, 0x0F, 0x12, 0x14, 0x1B, 0x1C, 0x1E, 0x1F}, // 0x04
        {0x1D, 0x03, 0x05, 0x09, 0x0F, 0x13, 0x14, 0x16, 0x17, 0x18, 0x1A, 0x1B, 0x1F}, // 0x05
        {0x19, 0x03, 0x05, 0x0B, 0x0D, 0x13, 0x14, 0x16, 0x17, 0x1B, 0x1C, 0x1E, 0x1F}, // 0x06
        {0x1D, 0x03, 0x05, 0x09, 0x0A, 0x0B, 0x0E, 0x12, 0x14, 0x18, 0x1A, 0x1B, 0x1F}, // 0x07
        {0x19, 0x1D, 0x0A, 0x0B, 0x0E, 0x0F, 0x1A, 0x1B, 0x1E, 0x1F}, // 0x08
        {0x07, 0x09, 0x0B, 0x0D, 0x0F, 0x12, 0x13, 0x16, 0x17, 0x18, 0x1B, 0x1C, 0x1F}, // 0x09
        {0x19, 0x1D, 0x12, 0x13, 0x16, 0x17, 0x1A, 0x1B, 0x1E, 0x1F}, // 0x0A
        {0x01, 0x10, 0x09, 0x0A, 0x0D, 0x0E, 0x18, 0x1B, 0x1C, 0x1F}, // 0x0B
        {0x02, 0x04, 0x03, 0x05, 0x0B, 0x0D, 0x12, 0x14, 0x1A, 0x1C}, // 0x0C
        {0x04, 0x03, 0x05, 0x09, 0x0A, 0x0B, 0x0E, 0x13, 0x14, 0x16, 0x17, 0x18, 0x1E}, // 0x0D
        {0x04, 0x03, 0x05, 0x0A, 0x0D, 0x0E, 0x0F, 0x13, 0x14, 0x16, 0x17, 0x1A, 0x1C}, // 0x0E
        {0x02, 0x04, 0x03, 0x05, 0x09, 0x0F, 0x12, 0x14, 0x18, 0x1E}, // 0x0F
        {0x06, 0x09, 0x0B, 0x0C, 0x0E, 0x14, 0x15, 0x16, 0x17, 0x18, 0x1A, 0x1C, 0x1E}, // 0x10
        {0x08, 0x11, 0x0A, 0x0B, 0x0C, 0x0D, 0x12, 0x13, 0x14, 0x15}, // 0x11
        {0x01, 0x09, 0x0A, 0x0C, 0x0F, 0x12, 0x13, 0x14, 0x15, 0x18, 0x1A, 0x1C, 0x1E}, // 0x12
        {0x06, 0x07, 0x0C, 0x0D, 0x0E, 0x0F, 0x14, 0x15, 0x16, 0x17}, // 0x13
        {0x03, 0x05, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0F, 0x12, 0x15, 0x16, 0x17, 0x18, 0x1A, 0x1B, 0x1F}, // 0x14
        {0x11, 0x03, 0x05, 0x0B, 0x0C, 0x0E, 0x0F, 0x13, 0x15, 0x1B, 0x1C, 0x1E, 0x1F}, // 0x15
        {0x10, 0x03, 0x05, 0x09, 0x0C, 0x0D, 0x0E, 0x13, 0x15, 0x18, 0x1A, 0x1B, 0x1F}, // 0x16
        {0x08, 0x03, 0x05, 0x0A, 0x0C, 0x12, 0x15, 0x16, 0x17, 0x1B, 0x1C, 0x1E, 0x1F}, // 0x17
        {0x07, 0x09, 0x0A, 0x0C, 0x0F, 0x14, 0x15, 0x16, 0x17, 0x18, 0x1B, 0x1C, 0x1F}, // 0x18
        {0x11, 0x0C, 0x0D, 0x0E, 0x0F, 0x12, 0x13, 0x14, 0x15, 0x1A, 0x1B, 0x1E, 0x1F}, // 0x19
        {0x01, 0x09, 0x0B, 0x0C, 0x0E, 0x12, 0x13, 0x14, 0x15, 0x18, 0x1B, 0x1C, 0x1F}, // 0x1A
        {0x08, 0x0A, 0x0B, 0x0C, 0x0D, 0x14, 0x15, 0x16, 0x17, 0x1A, 0x1B, 0x1E, 0x1F}, // 0x1B
        {0x02, 0x03, 0x05, 0x09, 0x0C, 0x0D, 0x0E, 0x12, 0x15, 0x16, 0x17, 0x18, 0x1E}, // 0x1C
        {0x08, 0x11, 0x03, 0x05, 0x0A, 0x0C, 0x13, 0x15, 0x1A, 0x1C}, // 0x1D
        {0x10, 0x03, 0x05, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0F, 0x13, 0x15, 0x18, 0x1E}, // 0x1E
        {0x02, 0x03, 0x05, 0x0B, 0x0C, 0x0E, 0x0F, 0x12, 0x15, 0x16, 0x17, 0x1A, 0x1C}, // 0x1F
    };

    // the weight of each compatible input pattern
    weightReverseOutputListPerInput = { {0}, // 0x00
        {2, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4}, // 0x01
        {2, 2, 4, 4, 4, 4, 4, 4, 4, 4}, // 0x02
        {2, 2, 4, 4, 4, 4, 4, 4, 4, 4}, // 0x03
        {2, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4}, // 0x04
        {2, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4}, // 0x05
        {2, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4}, // 0x06
        {2, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4}, // 0x07
        {2, 2, 4, 4, 4, 4, 4, 4, 4, 4}, // 0x08
        {2, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4}, // 0x09
        {2, 2, 4, 4, 4, 4, 4, 4, 4, 4}, // 0x0A
        {2, 2, 4, 4, 4, 4, 4, 4, 4, 4}, // 0x0B
        {2, 2, 4, 4, 4, 4, 4, 4, 4, 4}, // 0x0C
        {2, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4}, // 0x0D
        {2, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4}, // 0x0E
        {2, 2, 4, 4, 4, 4, 4, 4, 4, 4}, // 0x0F
        {2, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4}, // 0x10
        {2, 2, 4, 4, 4, 4, 4, 4, 4, 4}, // 0x11
        {2, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4}, // 0x12
        {2, 2, 4, 4, 4, 4, 4, 4, 4, 4}, // 0x13
        {4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4}, // 0x14
        {2, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4}, // 0x15
        {2, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4}, // 0x16
        {2, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4}, // 0x17
        {2, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4}, // 0x18
        {2, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4}, // 0x19
        {2, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4}, // 0x1A
        {2, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4}, // 0x1B
        {2, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4}, // 0x1C
        {2, 2, 4, 4, 4, 4, 4, 4, 4, 4}, // 0x1D
        {2, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4}, // 0x1E
        {2, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4}, // 0x1F
    };

    //                            0  1  2  3  4  5  6  7  8  9 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26 27 28 29 30 31
    minRevWeightListPerOutput = { 0, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 4, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2 };
#endif

}

unsigned int Propagation::getWeight(const State& state) const
{
    unsigned int weight = 0;
    for (unsigned int i = 0; i < nrCols; i++) {
        if(state.getColumn(i)!=0)
            weight += getWeight(state.getColumn(i));
    }
    return weight;
}

unsigned int Propagation::getMinRevWeight(const State& state) const
{
    unsigned int minRevWeight = 0;
    for (unsigned int i = 0; i < nrCols; i++) {
        if (state.getColumn(i) != 0)
            minRevWeight += getMinRevWeight(state.getColumn(i));
    }
    return minRevWeight;
}

AffineSpaceOfStates::AffineSpaceOfStates(vector<State>& aGenerators, vector<Row>& aGeneratorParities, const State& aOffset, const Row& aOffsetParity):
    offset(aOffset), offsetParity(aOffsetParity)
{
    // Copy the generators into originalGenerators
    generators = aGenerators;
    parities = aGeneratorParities;
}

EnvelopeSpaceOfStates::EnvelopeSpaceOfStates(vector<State>& aGenerators, vector<Row>& aGeneratorParities, const State& aOffset, const Row& aOffsetParity) : AffineSpaceOfStates(aGenerators, aGeneratorParities, aOffset, aOffsetParity) 
{
    // map the offset before the linear layer
    offsetAtA = aOffset;
    inverseLinearLayer(offsetAtA);
    offsetParityAtA = offsetAtA.getParity();

    // map the generators before the linear layer
    for (unsigned int i = 0; i < aGenerators.size(); i++) {
        State currentGenerator = aGenerators[i];
        inverseLinearLayer(currentGenerator);
        generatorsAtA.push_back(currentGenerator);
        Row currentParity = currentGenerator.getParity();
        paritiesAtA.push_back(currentParity);
    }
};

AffineSpaceOfStates Propagation::buildStateBase(const State& state, bool reverse) const
{
    (void)reverse;
    State offset;
    vector<State> basisBeforeChi;
    vector<Row> parities;
	
    for (unsigned int col = 0; col < nrCols; col++) { // for each column of the state
        Column columnIn = state.getColumn(col);
        if (columnIn != 0) {
            const AffineSpaceOfColumns& columnsOut = affinePerInput[columnIn]; // build the affine space after chi
            offset.setColumn(col, columnsOut.offset);
            for (unsigned int i = 0; i < columnsOut.generators.size(); i++) { // for each generator in the column basis
                State v;
                Column genColumnOut = columnsOut.generators[i];
                v.setColumn(col, genColumnOut);
                linearLayer(v); // map generator through the linear layer
                basisBeforeChi.push_back(v);
                Row p = v.getParity(); // take the generator parity
                parities.push_back(p);
            }
        }
    }

    // map the offset through the linear layer and take its parity
    linearLayer(offset);
    Row p = offset.getParity();
	
    return AffineSpaceOfStates(basisBeforeChi, parities, offset, p);
}

EnvelopeSpaceOfStates Propagation::buildEnvelopeSpaceBase(const State& state, bool reverse) const
{
    (void)reverse;
    State offsetAtB; // this is the offset before chi. It will remain zero
    Row offsetParityAtB = 0; // this will remain zero (FIX: was uninitialized)
    vector<State> basisAtB; // this is the basis before chi
    vector<Row> paritiesAtB; // this is the parity before chi

    vector<vector<State>> basisRowi(nrRows);

    for (unsigned int col = 0; col < nrCols; col++) { // for each column of the state
        Column columnIn = state.getColumn(col);
        if (columnIn != 0) {
            // the offset wll be all zero, so we don't add anything to it
            // build the basis vectors for the given column
            for (unsigned int i = 0; i < nrRows; i++) { // one bit per row
                State v;
                v.setBitToOne(i,col);
                basisRowi[i].push_back(v);
            }
        }
    }
   
    // put all generators together
    for (unsigned int i = 0; i < nrRows; i++) {
        for (unsigned int j = 0; j < basisRowi[i].size(); j++) {
            basisAtB.push_back(basisRowi[i][j]);
        }
    }
                
    return EnvelopeSpaceOfStates(basisAtB, paritiesAtB, offsetAtB, offsetParityAtB);
}

bool Propagation::checkCompatibility(const Column& columnAtB, Column& columnAtA) const
{
    for (unsigned int j = 0; j < directOutputListPerInput[columnAtB].size(); j++) {
        if (columnAtA == directOutputListPerInput[columnAtB][j])
            return true;
    }
    return false;
}

bool Propagation::checkCompatibility(const State& stateAtB, State& stateAtA) const
{
    for (unsigned int iz = 0; iz < nrCols; iz++) { // for each column of the state
        Column columnAtB = stateAtB.getColumn(iz);
        Column columnAtA = stateAtA.getColumn(iz);
        if (!checkCompatibility(columnAtB, columnAtA))
            return false;
    }
    return true;
}

/*
Given a basis in originalBasis, this function triangularizes the basis into newBasis.
The bits are ordered as [x, z].
It saves in stability the position of the first bit 1 of the corresponding generator in newBasis.
In other words, newBasis[i] has a bit 1 in position stability[i] and is zero in all positions strictly before stability[i].
 */
void upperTriangularizeBasis(const vector<State>& originalBasis, vector<State>& newBasis, vector<Coordinates>& stability)
{
    vector<State> basis(originalBasis);
    for (unsigned int ix = 0; ix < nrRows; ix++) {
        for (unsigned int iz = 0; iz < nrCols; iz++) {
            // Look for a generator with a 1 at position (x,z)
            bool found = false;
            State foundState;
            for (unsigned int i = 0; i < basis.size(); i++) {
                if (basis[i].getBit(ix, iz) == 1) {
                    found = true;
                    foundState = basis[i];
                    newBasis.push_back(foundState);
                    stability.push_back(Coordinates(ix, iz));
                    basis.erase(basis.begin() + i);
                    break;
                }
            }
            if (basis.size() == 0)
                break;
            // If found, cancel the bit at position (x,z) for the other generators
            if (found) {
                for (unsigned int i = 0; i < basis.size(); i++)
                    if (basis[i].getBit(ix, iz) == 1)
                        basis[i] ^= foundState;
            }
        }
    }
}

/*
Given a basis of the envelope space, this function triangularizes the basis at A into newBasisAtA.
The same computations are performed at B, too.
The basis vectors at B are first ordered by row, i.e. bits are ordered as [x, z].
It saves in stability the position of the first bit 1 of the corresponding generator in newBasisAtA.
In other words, newBasisAtA[i] has a bit 1 in position stability[i] and is zero in all positions strictly before stability[i].
 */
void upperTriangularizeBasis(const vector<State>& originalBasisAtA, const vector<State>& originalBasisAtB, vector<State>& newBasisAtA, vector<State>& newBasisAtB, vector<Coordinates>& stability)
{
    vector<State> basisAtA(originalBasisAtA);
    vector<State> basisAtB(originalBasisAtB);

    for (unsigned int ix = 0; ix < nrRows; ix++) {
        for (unsigned int iz = 0; iz < nrCols; iz++) {
            // Look for a generator with a 1 at position (x,z)
            bool found = false;
            State foundStateAtA, foundStateAtB;
            for (unsigned int i = 0; i < basisAtA.size(); i++) {
                if (basisAtA[i].getBit(ix, iz) == 1) {
                    found = true;
                    foundStateAtA = basisAtA[i];
                    newBasisAtA.push_back(foundStateAtA);
                    stability.push_back(Coordinates(ix, iz));
                    basisAtA.erase(basisAtA.begin() + i);
                    foundStateAtB = basisAtB[i];
                    newBasisAtB.push_back(foundStateAtB);
                    basisAtB.erase(basisAtB.begin() + i);
                    break;
                }
            }
            if (basisAtA.size() == 0)
                break;
            // If found, cancel the bit at position (x,z) for the other generators
            if (found) {
                for (unsigned int i = 0; i < basisAtA.size(); i++) {
                    if (basisAtA[i].getBit(ix, iz) == 1) {
                        basisAtA[i] ^= foundStateAtA;
                        basisAtB[i] ^= foundStateAtB;
                    }
                }
            }
        }
    }

}

ReverseStateIterator::ReverseStateIterator(const State& aStateA, const Propagation& DCorLC)
    : maxWeight(nrCols * 4)
{
    initialize(aStateA, DCorLC);
}

ReverseStateIterator::ReverseStateIterator(const State& aStateA, const Propagation& DCorLC, unsigned int aMaxWeightAtB, unsigned int aMaxWeight)
    : maxWeightAtB(aMaxWeightAtB), maxWeight(aMaxWeight)
{
    initialize(aStateA, DCorLC);
}

void ReverseStateIterator::initialize(const State& aStateA, const Propagation& DCorLC)
{
    stateB = State();
    stateA = State();
    minWeightAtB = 0;
    index = 0;
    nrActiveColumns = 0;
    bool firstCol = true;
    for (unsigned int ncol = 0; ncol < nrCols; ncol++) {
        Column col = aStateA.getColumn(ncol);
        if (col != 0) {
            activeColumns.push_back(ncol);
            patterns.push_back(DCorLC.reverseOutputListPerInput[col]);
            weights.push_back(DCorLC.weightReverseOutputListPerInput[col]);
            indexes.push_back(0);
            stateB.setColumn(ncol, DCorLC.reverseOutputListPerInput[col][0]);
            minWeightAtB += DCorLC.getWeight(DCorLC.reverseOutputListPerInput[col][0]);
            nrActiveColumns++;
            if (firstCol) {
                firstCol = false;
                // when the first column is added, all bits are stable, therefore we add an all zero mask
                StateMask zeroMask;
                stabilityMasks.push_back(zeroMask);
                // compute patterns before linear layer and mask
                StateMask newStabilityMask;
                vector<State> patternsBefLin;
                for (unsigned int i = 0; i < DCorLC.reverseOutputListPerInput[col].size(); i++) {
                    State patternBeforeLin;
                    patternBeforeLin.setColumn(ncol, DCorLC.reverseOutputListPerInput[col][i]);
                    inverseLinearLayer(patternBeforeLin);
                    patternsBefLin.push_back(patternBeforeLin);
                    newStabilityMask |= patternBeforeLin;
                }
                stateA ^= patternsBefLin[0];
                patternsBeforeLin.push_back(patternsBefLin);
                stabilityMasks.push_back(newStabilityMask);
            }
            else {
                StateMask newStabilityMask;
                vector<State> patternsBefLin;
                for (unsigned int i = 0; i < DCorLC.reverseOutputListPerInput[col].size(); i++) {
                    State patternBeforeLin;
                    patternBeforeLin.setColumn(ncol, DCorLC.reverseOutputListPerInput[col][i]);
                    inverseLinearLayer(patternBeforeLin);
                    newStabilityMask |= patternBeforeLin;
                    patternsBefLin.push_back(patternBeforeLin);
                }
                stateA ^= patternsBefLin[0];
                patternsBeforeLin.push_back(patternsBefLin);
                newStabilityMask |= stabilityMasks.back();
                stabilityMasks.push_back(newStabilityMask);
            }
        }
    }

    for (unsigned int j = 0; j < stabilityMasks.size() - 1; j++) {
        stabilityMasks[j].invert();
    }
    //current weight 
    currentWeightAtB = minWeightAtB;
    end = isEmpty();
	
}

bool ReverseStateIterator::isEnd() const
{
    return end;
}

bool ReverseStateIterator::isEmpty() const
{
    return (minWeightAtB+2 > maxWeight) || (nrActiveColumns == 0);
}

void ReverseStateIterator::operator++()
{
    next();
    index++;
}

const State& ReverseStateIterator::operator*() const
{
    return stateB;
}

unsigned int ReverseStateIterator::getCurrentWeight() const
{
    return currentWeightAtB;
}

void ReverseStateIterator::next()
{  
    unsigned int i = 0;
    bool columnFound = false;
    unsigned int ii = 0;
    unsigned int scoreAtB = currentWeightAtB;
    while (i < nrActiveColumns) {
        ii = indexes[i]; // get the index of the current compatible pattern for the i-th active column
        currentWeightAtB -= weights[i][ii]; // remove the contribution of the current compatible pattern
        stateA ^= patternsBeforeLin[i][ii]; // remove patterns from A, no need to remove it from B now because we can compute the weight with weights[i][ii], we will reset the column value later
        scoreAtB -= weights[i][ii];
        ii++;
        while (ii < patterns[i].size()) {
            if (scoreAtB + (int)weights[i][ii] <= maxWeightAtB) {
                State temp = stateA;
                temp ^= patternsBeforeLin[i][ii];
                temp &= stabilityMasks[i];
                if (scoreAtB + (int)weights[i][ii] + 2 * temp.getNrActiveColumns() <= maxWeight) {
                    columnFound = true;
                    break;
                }
            }
            else {
                columnFound = false;
                break;
            }
            ii++;
        }
        if (columnFound)
            break;
        scoreAtB += weights[i][0]; // this pattern will be reset to the first compatible which will contribute with minimum weight
        i++;
    }
    if (i >= nrActiveColumns) {
        end = true;
        return;
    }
    indexes[i]=ii;
    currentWeightAtB += weights[i][indexes[i]];
    stateB.setColumn(activeColumns[i], patterns[i][indexes[i]]);
    stateA ^= patternsBeforeLin[i][indexes[i]];
    for (unsigned int j = 0; j < i; j++) {
        indexes[j] = 0;
        currentWeightAtB += weights[j][0];
        stateB.setColumn(activeColumns[j], patterns[j][0]);
        stateA ^= patternsBeforeLin[j][0];
    }
}

void Propagation::backwardExtendTrail(ostream& fout, const TrailCore& trail, unsigned int maxTotalWeight, unsigned int maxWeightAtB, unsigned int& minWeightFound, bool weightedWeights)
{
    int baseWeight = trail.totalWeight - trail.minRevWeight;
    // FIX: signed comparison (the unsigned difference wrapped around when baseWeight > maxTotalWeight)
    if ((int)maxTotalWeight - baseWeight < 4)
        return;
    State stateB1 = trail.states[0];
    State stateA1 = stateB1;
    inverseLinearLayer(stateA1);


    ReverseStateIterator i(stateA1, trail.DCorLC, maxWeightAtB, maxTotalWeight - baseWeight);
    if (i.isEmpty())
        return;
    for (; !i.isEnd(); ++i) {
        State stateB0 = *i;
        unsigned int weightB0 = i.getCurrentWeight();
        State stateA0 = stateB0;
        inverseLinearLayer(stateA0);
        unsigned int minRevWeightA0 = getMinRevWeight(stateA0);
        unsigned int curWeight = weightB0 + minRevWeightA0;
        // in case of backward extension with weighted weights (i.e. w1+2w2) we keep only those with w0 >= w2
        if (weightedWeights == 0 || (weightedWeights == 1 && minRevWeightA0 >= trail.weights.back()))
        {
            if (baseWeight + curWeight <= maxTotalWeight) {
                TrailCore newTrail(trail);
                newTrail.prepend(stateB0, weightB0);
                newTrail.save(fout);
                if (newTrail.totalWeight < minWeightFound)
                    minWeightFound = newTrail.totalWeight;
            }
        }
    }
}

TrailCore::TrailCore(const Propagation& aDCorLC)
    : DCorLC(aDCorLC), minRevWeight(0), totalWeight(0)
{
}

void TrailCore::append(const State& stateAtB, unsigned int weight)
{
    states.push_back(stateAtB);
    weights.push_back(weight);
    totalWeight += weight;
}

void TrailCore::prepend(const State& stateAtB, unsigned int weight)
{
    states.insert(states.begin(), stateAtB);
    weights.insert(weights.begin(), weight);
    totalWeight -= minRevWeight;
    State stateAtA = stateAtB;
    inverseLinearLayer(stateAtA);
    minRevWeight = DCorLC.getMinRevWeight(stateAtA);
    totalWeight += weight + minRevWeight;
}

void TrailCore::setMinRevWeight(unsigned int aMinRevWeight)
{
    totalWeight -= minRevWeight;
    minRevWeight = aMinRevWeight;
    totalWeight += minRevWeight;
}

void TrailCore::load(istream& fin)
{
    unsigned int nrRounds;
    fin >> dec >> nrRounds;
    fin >> dec >> totalWeight;
    fin >> dec >> minRevWeight;
    for (unsigned int i = 0; i < nrRounds - 1; i++) {
        unsigned int w;
        fin >> dec >> w;
        weights.push_back(w);
    }
    states.clear();
    for (unsigned int i = 0; i < nrRounds - 1; i++) {
        State s;
        for (unsigned int row = 0; row < nrRows; row++) {
            Row rowVal;
            fin >> hex >> rowVal;
            s.setRow(row, rowVal);
        }
        states.push_back(s);
    }
}

void TrailCore::save(ostream& fout) const
{   
    fout << hex;
    fout << states.size() + 1 << " "; // number of rounds
    fout << dec << totalWeight << " "; // total weight
    fout << minRevWeight << " "; // minimum reverse weight
    for (unsigned int i = 0; i < weights.size(); i++)
        fout << weights[i] << " "; // weight over each round
    for (unsigned int i = 0; i < states.size(); i++) { // each state
        for (unsigned int r = 0; r < nrRows; r++) { // each row
            fout << hex << states[i].getRow(r) << " ";
        }
        fout << " ";
    }
    fout << endl;
}

void TrailCore::display(ostream& fout) const
{
    fout << dec << states.size() + 1 << "-round trail core of weight "; // number of rounds
    fout << dec << totalWeight << endl << endl; // total weight
    fout << "weight profile: ";
    fout << minRevWeight << " "; // minimum reverse weight
    for (unsigned int i = 0; i < weights.size(); i++)
        fout << weights[i] << " "; // weight over each round
    fout << endl << endl;
    for (unsigned int i = 0; i < states.size(); i++) { // each state
        State B = states[i];
        State A = B;
        inverseLinearLayer(A);
        A.display(fout, false);
        fout << "---------------------------------------------------------------- pL" << endl;
        B.display(fout, false);
        fout << "---------------------------------------------------------------- pS" << endl;
    }
}

TrailCore& TrailCore::operator=(TrailCore& other)
{
    states = other.states;
    weights = other.weights;
    minRevWeight = other.minRevWeight;
    totalWeight = other.totalWeight;
    return *this;
}

Trail::Trail(const Propagation& aDCorLC)
    : DCorLC(aDCorLC), totalWeight(0)
{
}

void Trail::append(const State& stateAtB, unsigned int weight)
{
    states.push_back(stateAtB);
    weights.push_back(weight);
    totalWeight += weight;
}

void Trail::prepend(const State& stateAtB, unsigned int weight)
{
    states.insert(states.begin(), stateAtB);
    weights.insert(weights.begin(), weight);
    totalWeight += weight;
}

void Trail::load(istream& fin)
{
    unsigned int nrRounds;
    fin >> dec >> nrRounds;
    fin >> dec >> totalWeight;
    for (unsigned int i = 0; i < nrRounds; i++) {
        unsigned int w;
        fin >> dec >> w;
        weights.push_back(w);
    }
    states.clear();
    for (unsigned int i = 0; i < nrRounds; i++) {
        State s;
        for (unsigned int row = 0; row < nrRows; row++) {
            Row rowVal;
            fin >> hex >> rowVal;
            s.setRow(row, rowVal);
        }
        states.push_back(s);
    }
}

void Trail::save(ostream& fout) const
{
    fout << hex;
    fout << states.size() << " "; // number of rounds
    fout << dec << totalWeight << " "; // total weight
    for (unsigned int i = 0; i < weights.size(); i++)
        fout << weights[i] << " "; // weight over each round
    for (unsigned int i = 0; i < states.size(); i++) { // each state
        for (unsigned int r = 0; r < nrRows; r++) { // each row
            fout << hex << states[i].getRow(r) << " ";
        }
        fout << " ";
    }
    fout << endl;
}

void Trail::display(ostream& fout) const
{
    fout << dec << states.size() << "-round trail of weight "; // number of rounds
    fout << dec << totalWeight << endl; // total weight
    fout << "weight profile: ";
    for (unsigned int i = 0; i < weights.size(); i++)
        fout << weights[i] << " "; // weight over each round
    fout << endl << endl;
    for (unsigned int i = 0; i < states.size(); i++) { // each state
        State B = states[i];
        State A = B;
        inverseLinearLayer(A);
        A.display(fout, false);
        fout << "---------------------------------------------------------------- pL" << endl;
        B.display(fout, false);
        fout << "---------------------------------------------------------------- pS" << endl;
    }
    fout << endl << endl;
}

void generateTrailsInCore(ostream& fout, const TrailCore& trailCore, unsigned int maxTotalWeight)
{
    int baseWeight = trailCore.totalWeight - trailCore.minRevWeight;
    State stateB1 = trailCore.states[0];
    State stateA1 = stateB1;
    inverseLinearLayer(stateA1);
	ReverseStateIterator i(stateA1, trailCore.DCorLC, maxTotalWeight - baseWeight, 9999);
    if (i.isEmpty())
        return;
    for (; !i.isEnd(); ++i) {
        State stateB0 = *i;
        unsigned int weightB0 = i.getCurrentWeight();
        if (baseWeight + weightB0 <= maxTotalWeight) {
            Trail newTrail(trailCore);
            newTrail.prepend(stateB0, weightB0);
            newTrail.save(fout);
        }
    }
}

vector<unsigned int> countActiveSboxesInCore(const TrailCore& trailCore)
{
    vector<unsigned int> numSboxes;
    State stateB1 = trailCore.states[0];
    State stateA1 = stateB1;
    inverseLinearLayer(stateA1);
    numSboxes.push_back(stateA1.getNrActiveColumns());
    for(unsigned int i = 0; i < trailCore.states.size(); i++){
        State stateB = trailCore.states[i];
        numSboxes.push_back(stateB.getNrActiveColumns());
    }
    return numSboxes;
}
