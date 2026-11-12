/*
AsconTrailTool, a set of C++ classes for analyzing ASCON.

For specifications, please refer to https://tosc.iacr.org/index.php/ToSC/article/view/9975

Implementation by Solane El Hirch, hereby denoted as "the implementer".
*/

#ifndef TWOROUNDTRAILCOREGENERATION_H
#define TWOROUNDTRAILCOREGENERATION_H

#include <cstdint>

#include "treeCanonical.h"
#include "asconColScore.h"

using namespace std;

int generateTwoRoundTrailCores();

#endif /* TWOROUNDTRAILCOREGENERATION_H */
