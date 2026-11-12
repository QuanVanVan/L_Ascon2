/*
AsconTrailTool, a set of C++ classes for analyzing ASCON.

For specifications, please refer to https://tosc.iacr.org/index.php/ToSC/article/view/9975

Implementation by Silvia Mella, hereby denoted as "the implementer".
*/

#pragma once

#ifndef _TREEEXTENSION_H_
#define _TREEEXTENSION_H_

#include "tree.h"
#include "asconParts.h"
#include "trailExtension.h"

/////////////////// Forward Extension ///////////////////

typedef unsigned int AffineSpaceBasisIndex;

class AffineSpaceShadow {
public:
    const Propagation& DCorLC;
    TrailCore& trailCore;
    vector<State>& basis;
    State& offset;
    vector<Coordinates>& stability;
    State stateConsidered;
    StateMask partOfOffsetNeverMoving;
    bool optimized;
public:
    AffineSpaceShadow(TrailCore& aTrailCore, vector<State>& aBasis, State& anOffset, vector<Coordinates>& aStability)
        : DCorLC(aTrailCore.DCorLC), trailCore(aTrailCore), basis(aBasis), offset(anOffset), stability(aStability), stateConsidered(anOffset)
    {
        initializePartOfOffsetNeverMoving();
    }
    void push(AffineSpaceBasisIndex& newUnit);
    void pop(AffineSpaceBasisIndex& newUnit);
private:
    void initializePartOfOffsetNeverMoving();
};

class AffineSpaceUnitSet {
public:
    const AffineSpaceShadow& affineSpace;
public:
    AffineSpaceUnitSet(AffineSpaceShadow& anAffineSpace)
        : affineSpace(anAffineSpace) {}
    AffineSpaceBasisIndex getFirstChildUnit(vector<AffineSpaceBasisIndex>& unitList, AffineSpaceShadow& shadow);
    void iterateUnit(vector<AffineSpaceBasisIndex>& unitList, AffineSpaceBasisIndex& current, AffineSpaceShadow& shadow);
    bool isNodeCanonical(vector<AffineSpaceBasisIndex>& unitList, AffineSpaceShadow& shadow);
};

class AffineSpaceExtendedTrail : public TrailCore {
public:
    AffineSpaceExtendedTrail(const Propagation& aDCorLC)
        : TrailCore(aDCorLC) {}
    AffineSpaceExtendedTrail(AffineSpaceShadow& shadow)
        : TrailCore(shadow.trailCore.DCorLC) {}
    AffineSpaceExtendedTrail(TrailCore& other)
        : TrailCore(other) {}
    void set(vector<AffineSpaceBasisIndex>& unitList, AffineSpaceShadow& shadow);
};

class AffineSpaceCostFunction {
public:
    unsigned int getSubtreeLowerBound(vector<AffineSpaceBasisIndex>& parentUnitList, AffineSpaceShadow& shadow);
    unsigned int getNodeCost(vector<AffineSpaceBasisIndex>& unitList, AffineSpaceShadow& shadow);
};

void extendTrailForward(ostream& fout, TrailCore& trailCore, unsigned int maxWeight, unsigned int& minWeightFound, bool weightedWeights);

/////////////////// Backward Extension with envelope space /////////////////////

typedef unsigned int EnvelopeSpaceBasisIndex;

class EnvelopeSpaceShadow {
public:
    const Propagation& DCorLC;
    TrailCore& trailCore;
    vector<State>& basisAtA;
    vector<State>& basisAtB;
    vector<Coordinates>& stability;
    State stateAtA;
    State stateAtB;
public:
    EnvelopeSpaceShadow(TrailCore& aTrailCore, vector<State>& aBasisAtA, vector<State>& aBasisAtB, vector<Coordinates>& aStability)
        : DCorLC(aTrailCore.DCorLC), trailCore(aTrailCore), basisAtA(aBasisAtA), basisAtB(aBasisAtB), stability(aStability)
    {
        stateAtA = State();
        stateAtB = State();
    }
    void push(EnvelopeSpaceBasisIndex& newUnit);
    void pop(EnvelopeSpaceBasisIndex& newUnit);
};

class EnvelopeSpaceUnitSet {
public:
    const EnvelopeSpaceShadow& envelopeSpace;
public:
    EnvelopeSpaceUnitSet(EnvelopeSpaceShadow& anEnvelopeSpace)
        : envelopeSpace(anEnvelopeSpace) {}
    EnvelopeSpaceBasisIndex getFirstChildUnit(vector<EnvelopeSpaceBasisIndex>& unitList, EnvelopeSpaceShadow& shadow);
    void iterateUnit(vector<EnvelopeSpaceBasisIndex>& unitList, EnvelopeSpaceBasisIndex& current, EnvelopeSpaceShadow& shadow);
    bool isNodeCanonical(vector<EnvelopeSpaceBasisIndex>& unitList, EnvelopeSpaceShadow& shadow);
};

class ExtendedState {
public:
    State stateAtA;
    State stateAtB;
    unsigned int currentNrActiveColumnsAtB;
public:
    ExtendedState()
        : stateAtA(), stateAtB()
    {
        currentNrActiveColumnsAtB = 0;
    }
    ExtendedState(EnvelopeSpaceShadow& shadow)
        : stateAtA(shadow.stateAtA), stateAtB(shadow.stateAtB), currentNrActiveColumnsAtB(shadow.stateAtB.getNrActiveColumns()) {}
    void set(vector<EnvelopeSpaceBasisIndex>& unitList, EnvelopeSpaceShadow& shadow);
};

class EnvelopeSpaceCostFunction {
public:
    unsigned int getSubtreeLowerBound(vector<EnvelopeSpaceBasisIndex>& parentUnitList, EnvelopeSpaceShadow& shadow);
    unsigned int getNodeCost(vector<EnvelopeSpaceBasisIndex>& unitList, EnvelopeSpaceShadow& shadow);
};

void extendTrailBackward(ostream& fout, TrailCore& trailCore, unsigned int maxWeight, unsigned int& minWeightFound, bool weightedWeights);

#endif
