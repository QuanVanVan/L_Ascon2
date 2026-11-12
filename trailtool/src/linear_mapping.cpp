/*
AsconTrailTool, a set of C++ classes for analyzing ASCON.

For specifications, please refer to https://tosc.iacr.org/index.php/ToSC/article/view/9975

Implementation by Solane El Hirch, hereby denoted as "the implementer".
*/

#include <iostream>
#include <cstdint>

#include "config.h"
#include "linear_mapping.h"

uint64_t shift64(uint64_t image, int x){
	return (image >> (64-x)) | (image << x);
}
