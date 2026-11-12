/*
AsconTrailTool, a set of C++ classes for analyzing ASCON.

For specifications, please refer to https://tosc.iacr.org/index.php/ToSC/article/view/9975

Implementation by Silvia Mella, hereby denoted as "the implementer".
*/

#include "treeExtension.h"
#include "trailExtension.h"

void AffineSpaceShadow::initializePartOfOffsetNeverMoving()
{
    partOfOffsetNeverMoving.clear();
    for (unsigned int i = 0; i < basis.size(); i++)
        partOfOffsetNeverMoving |= basis[i];
    partOfOffsetNeverMoving.invert();
}

void AffineSpaceShadow::push(AffineSpaceBasisIndex& newUnit)
{
    stateConsidered ^= basis[newUnit];
}

void AffineSpaceShadow::pop(AffineSpaceBasisIndex& lastUnit)
{
    stateConsidered ^= basis[lastUnit];
}

AffineSpaceBasisIndex AffineSpaceUnitSet::getFirstChildUnit(vector<AffineSpaceBasisIndex>& unitList, AffineSpaceShadow& shadow)
{
    (void)shadow;
    if (unitList.size() == 0) {
        if (affineSpace.basis.size() == 0)
            throw EndOfSet();
        return 0;
    }
    else {
        AffineSpaceBasisIndex next = unitList.back() + 1;
        if (next >= affineSpace.basis.size())
            throw EndOfSet();
        return next;
    }
}

void AffineSpaceUnitSet::iterateUnit(vector<AffineSpaceBasisIndex>& unitList, AffineSpaceBasisIndex& current, AffineSpaceShadow& shadow)
{
    (void)unitList;
    (void)shadow;
    current++;
    if (current >= affineSpace.basis.size())
        throw EndOfSet();
}

bool AffineSpaceUnitSet::isNodeCanonical(vector<AffineSpaceBasisIndex>& unitList, AffineSpaceShadow& shadow)
{
    (void)unitList;
    (void)shadow;
    return true;
}

void AffineSpaceExtendedTrail::set(vector<AffineSpaceBasisIndex>& unitList, AffineSpaceShadow& shadow)
{
    (void)unitList;
    TrailCore::operator=(shadow.trailCore);
    append(shadow.stateConsidered, DCorLC.getWeight(shadow.stateConsidered));
}

unsigned int AffineSpaceCostFunction::getSubtreeLowerBound(vector<AffineSpaceBasisIndex>& parentUnitList, AffineSpaceShadow& shadow)
{
    (void)parentUnitList;
    int lastUnit = -1;
    if (parentUnitList.size() == 0)
        lastUnit = -1;
    else
        lastUnit = parentUnitList.back();
    unsigned int trailWeight = shadow.trailCore.totalWeight;
    if (shadow.optimized) {
        if (shadow.stability.size() > (unsigned int)lastUnit + 1) {
            Coordinates currentStability = shadow.stability[lastUnit + 1];
            StateMask stabilityMask;
            stabilityMask.setMaskXZ(currentStability.x, currentStability.z);
            stabilityMask |= shadow.partOfOffsetNeverMoving;
            State maskedState(shadow.stateConsidered);
            maskedState &= stabilityMask;
            return trailWeight + 2 * maskedState.getNrActiveColumns();
        }
        else if ((unsigned int)lastUnit == (shadow.basis.size() - 1)) {
            State stateConsidered(shadow.stateConsidered);
            return trailWeight + shadow.DCorLC.getWeight(stateConsidered);
        }
        else
            return trailWeight;
    }
    else
        return trailWeight;
}

unsigned int AffineSpaceCostFunction::getNodeCost(vector<AffineSpaceBasisIndex>& unitList, AffineSpaceShadow& shadow)
{
    (void)unitList;
    unsigned int trailWeight = shadow.trailCore.totalWeight;
    return trailWeight + shadow.DCorLC.getWeight(shadow.stateConsidered);
}

void extendTrailForward(ostream& fout, TrailCore& trailCore, unsigned int maxWeight, unsigned int& minWeightFound, bool weightedWeights)
{
    State stateToExtend = trailCore.states.back();
    AffineSpaceOfStates as = trailCore.DCorLC.buildStateBase(stateToExtend, false);
    vector<State> triangularBasis;
    vector<Coordinates> stability;
    upperTriangularizeBasis(as.generators, triangularBasis, stability);
    AffineSpaceShadow shadow(trailCore, triangularBasis, as.offset, stability);

    shadow.optimized = true;
    AffineSpaceUnitSet unitSet(shadow);
    AffineSpaceCostFunction cost;
    GenericTreeIterator<AffineSpaceBasisIndex, AffineSpaceUnitSet, AffineSpaceShadow, AffineSpaceExtendedTrail, AffineSpaceCostFunction>
        tree(unitSet, shadow, cost, maxWeight);
    while (!tree.isEnd()) {
        const AffineSpaceExtendedTrail& core = (*tree);
        // in case of forward extension with weighted weights (i.e. 2w0+w1) we keep only those with w0 < w2
        if ((core.totalWeight <= maxWeight) && (weightedWeights == false || (weightedWeights == true && core.minRevWeight < core.weights.back())))
        {
            core.save(fout);
            if (core.totalWeight < minWeightFound)
                minWeightFound = core.totalWeight;
        }
        ++tree;   
    }
}

/////////////////// Backward Extension with Envelope Space ///////////////////

void EnvelopeSpaceShadow::push(EnvelopeSpaceBasisIndex& newUnit)
{
    stateAtB ^= basisAtB[newUnit];
    stateAtA ^= basisAtA[newUnit];
}

void EnvelopeSpaceShadow::pop(EnvelopeSpaceBasisIndex& lastUnit)
{
    stateAtB ^= basisAtB[lastUnit];
    stateAtA ^= basisAtA[lastUnit];
}

EnvelopeSpaceBasisIndex EnvelopeSpaceUnitSet::getFirstChildUnit(vector<EnvelopeSpaceBasisIndex>& unitList, EnvelopeSpaceShadow& shadow)
{
    (void)shadow;
    if (unitList.size() == 0) {
        if (envelopeSpace.basisAtB.size() == 0)
            throw EndOfSet();
        return 0;
    }
    else {
        AffineSpaceBasisIndex next = unitList.back() + 1;
        if (next >= envelopeSpace.basisAtB.size())
            throw EndOfSet();
        return next;
    }
}

void EnvelopeSpaceUnitSet::iterateUnit(vector<EnvelopeSpaceBasisIndex>& unitList, EnvelopeSpaceBasisIndex& current, EnvelopeSpaceShadow& shadow)
{
    (void)unitList;
    (void)shadow;
    current++;
    if (current >= envelopeSpace.basisAtB.size())
        throw EndOfSet();
}

bool EnvelopeSpaceUnitSet::isNodeCanonical(vector<EnvelopeSpaceBasisIndex>& unitList, EnvelopeSpaceShadow& shadow)
{
    (void)unitList;
    (void)shadow;
    return true;
}

void ExtendedState::set(vector<EnvelopeSpaceBasisIndex>& unitList, EnvelopeSpaceShadow& shadow)
{
    (void)unitList;
    stateAtA = shadow.stateAtA;
    stateAtB = shadow.stateAtB;
    currentNrActiveColumnsAtB = shadow.stateAtB.getNrActiveColumns();
}

unsigned int EnvelopeSpaceCostFunction::getSubtreeLowerBound(vector<EnvelopeSpaceBasisIndex>& parentUnitList, EnvelopeSpaceShadow& shadow)
{
    (void)parentUnitList;
    int lastUnit = -1;
    if (parentUnitList.size() == 0)
        lastUnit = -1;
    else
        lastUnit = parentUnitList.back();
	
    unsigned int trailWeight = shadow.trailCore.totalWeight; 
    if ((int)shadow.stability.size() > lastUnit + 1) {
        Coordinates currentStability = shadow.stability[lastUnit + 1]; // the unit has already been pushed at this point, this is why we consider lastUnit+1
        StateMask stabilityMask;
        stabilityMask.setMaskXZ(currentStability.x, currentStability.z);
        State maskedState(shadow.stateAtA);
        maskedState &= stabilityMask;
        return trailWeight + 2 * maskedState.getNrActiveColumns();
    }
    else if ((unsigned int)lastUnit == (shadow.basisAtA.size() - 1)) {
        State stateConsidered(shadow.stateAtA);
        return trailWeight + shadow.DCorLC.getMinRevWeight(stateConsidered);
    }
    else
        return trailWeight;
}

unsigned int EnvelopeSpaceCostFunction::getNodeCost(vector<EnvelopeSpaceBasisIndex>& unitList, EnvelopeSpaceShadow& shadow)
{
    (void)unitList;
    return shadow.DCorLC.getMinRevWeight(shadow.stateAtA) + shadow.DCorLC.getWeight(shadow.stateAtB);
}

void extendTrailBackward(ostream& fout, TrailCore& trailCore, unsigned int maxWeight, unsigned int& minWeightFound, bool weightedWeights)
{
    // FIX: signed comparison (the unsigned difference wrapped around when totalWeight > maxWeight)
    if ((int)maxWeight - (int)trailCore.totalWeight < 2)
        return;

    State stateToExtend = trailCore.states[0];
    inverseLinearLayer(stateToExtend);
    unsigned int nrActiveColumns = stateToExtend.getNrActiveColumns();
   
    EnvelopeSpaceOfStates envelope = trailCore.DCorLC.buildEnvelopeSpaceBase(stateToExtend, true);
    vector<State> triangularBasisAtA, triangularBasisAtB;
    vector<Coordinates> stability;
    upperTriangularizeBasis(envelope.generatorsAtA, envelope.generators, triangularBasisAtA, triangularBasisAtB, stability);
    EnvelopeSpaceShadow shadow(trailCore, triangularBasisAtA, triangularBasisAtB, stability);

    EnvelopeSpaceUnitSet unitSet(shadow);
    EnvelopeSpaceCostFunction cost;
    GenericTreeIterator<EnvelopeSpaceBasisIndex, EnvelopeSpaceUnitSet, EnvelopeSpaceShadow, ExtendedState, EnvelopeSpaceCostFunction>
        tree(unitSet, shadow, cost, maxWeight);
    while (!tree.isEnd()) {
        const ExtendedState& node = (*tree);
        if (node.currentNrActiveColumnsAtB == nrActiveColumns) {
            unsigned int baseWeight = trailCore.totalWeight - trailCore.minRevWeight;
            unsigned int w0 = trailCore.DCorLC.getMinRevWeight(node.stateAtA);
            unsigned int w1 = trailCore.DCorLC.getWeight(node.stateAtB);
            unsigned int newWeight = baseWeight + w0 + w1;
            // in case of backward extension with weighted weights (i.e. w1+2w2) we keep only those with w0 >= w2
            if ((newWeight <= maxWeight) && (weightedWeights == false || (weightedWeights == true && w0 >= trailCore.weights.back())))
            {
                // check compatibility
                if (trailCore.DCorLC.checkCompatibility(node.stateAtB, stateToExtend)) {
                    TrailCore newTrail(trailCore);
                    newTrail.prepend(node.stateAtB, w1);
                    newTrail.save(fout);
                    if (newTrail.totalWeight < minWeightFound)
                        minWeightFound = newTrail.totalWeight;
                }
            }
        }
        ++tree;
    }
}

