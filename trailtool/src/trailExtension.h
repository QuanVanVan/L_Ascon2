/*
AsconTrailTool, a set of C++ classes for analyzing ASCON.

For specifications, please refer to https://tosc.iacr.org/index.php/ToSC/article/view/9975

Implementation by Silvia Mella, hereby denoted as "the implementer".
*/

#ifndef _TRAILEXTENSION_H_
#define _TRAILEXTENSION_H_

#pragma once

#include <sstream>
#include "asconParts.h"

void linearLayer(State& s);
void inverseLinearLayer(State& s);

class StateMask : public State {
public:
    StateMask()
        : State() {}
    void setMaskXZ(unsigned int x, unsigned int z);
    void invert();
    bool nonEmptyIntersection(State& other);
};

class AffineSpaceOfColumns {
public:
    /** The offset. */
    Column offset;
    /** The generators. */
    vector<Column> generators;
public:
    /** The constructor creates an affine space with a single element: 0. */
    AffineSpaceOfColumns() { offset = 0; }
    /** The constructor creates an affine space with a single element: 0. */
    AffineSpaceOfColumns(Column aOffset, vector<Column> aGenerators) : offset(aOffset), generators(aGenerators) {};
    /** This method adds a generator to the set of generators.
      * @param   generator  The generator to add
      */
    void addGenerator(Column generator) { generators.push_back(generator); }
    /** This method sets the offset of the affine space.
      * @param   anOffset   The new offset value.
      */
    void setOffset(Column anOffset) { offset = anOffset; }
};

/** This class expresses an affine space of states.
  * The members of the affine space are determined by the offset
  * plus any linear combination of the generators.
  */
class AffineSpaceOfStates {
public:
    /** The set of generators
      */
    vector<State> generators;
    /** The set of parities, i.e., originalParities[i] contains the parity
      * of originalGenerators[i].
      */
    vector<Row> parities;
    /** The offset of the affine space.
      */
    State offset;
    /** The parity of the offset of the affine space.
      */
    Row offsetParity;
public:
    /** This constructor initializes the different attributes from the given generators,
      * the offset and their parities.
      * @note The passed vectors aGenerators and aGeneratorParities are modified in the process.
      * @param   anInstance         The Zero instance.
      * @param   aGenerators        The set of generators of the affine space.
      * @param   aGeneratorParities The associated parities.
      * @param   aOffset            The offset of the affine space.
      * @param   aOffsetParity      The parity associated to the offset.
      */
    AffineSpaceOfStates(vector<State>& aGenerators, vector<Row>& aGeneratorParities, const State& aOffset, const Row& aOffsetParity);
    /** This method returns an iterator to all states in the affine space.
      * @return The iterator to the whole affine space.
      */
};

/** This class expresses an envelope space of states.
  * It keeps trace of the generators before the linear layer too. 
  */
class EnvelopeSpaceOfStates : public AffineSpaceOfStates {
public:
    /** The set of generators before the linear layer.
      */
    vector<State> generatorsAtA;
    /** The set of parities before the linear layer.
      */
    vector<Row> paritiesAtA;
    /** The offset of the affine space before the linear layer. This is zero.
      */
    State offsetAtA;
    /** The parity of the offset of the affine space before the linear layer. This is zero.
      */
    Row offsetParityAtA;
public:
    /** This constructor initializes the different attributes from the given generators,
      * the offset and their parities.
    */
    EnvelopeSpaceOfStates(vector<State>& aGenerators, vector<Row>& aGeneratorParities, const State& aOffset, const Row& aOffsetParity);
};

class TrailCore;

class Propagation {
public:
    /** This attribute contains the output patterns for the non-linear function. 
      * The index of the vector corresponds to the input pattern.
      * - for DC, contains the output difference patterns for the non-linear function.
      * - for LC, contains the output linear masks for the inverse of the non-linear function.
      */
    vector<vector<Column>> directOutputListPerInput;
    /** This attribute contains the output patterns in an affine space representation.
      * The index of the vector corresponds to the input pattern.
      * - for DC, contains the output difference patterns for the non-linear function.
      * - for LC, contains the output linear masks for the inverse of the non-linear function.
      */
    vector<AffineSpaceOfColumns> affinePerInput;
    /** This attribute contains the weight of any pattern at the input of the non-linear function.
      * The index of the vector corresponds to the input pattern.
      */
    vector<unsigned int> weightListPerInput;
    /** This attribute contains the output patterns for the non-linear function in the inverse direction,
      * ordered by their weight.
      * The index of the vector corresponds to the input pattern.
      * - for DC, contains the output difference patterns for the inverse of the non-linear function.
      * - for LC, contains the output linear masks for the non-linear function.
      */
    vector<vector<Column>> reverseOutputListPerInput;
    /** This attribute contains the weights of the output patterns for the non-linear function in the inverse direction.
      * The index of the vector corresponds to the input pattern.
      * - for DC, contains the output difference patterns for the inverse of the non-linear function.
      * - for LC, contains the output linear masks for the non-linear function.
      */
    vector<vector<unsigned int>> weightReverseOutputListPerInput;
    /** This attribute contains the minimum reverse weight of any pattern at the output of the non-linear function.
      * The index of the vector corresponds to the input pattern.
      */
    vector<unsigned int> minRevWeightListPerOutput;

public:
    Propagation();

    unsigned int getWeight(const Column& col) const { return weightListPerInput[col]; }
    unsigned int getMinRevWeight(const Column& col) const { return minRevWeightListPerOutput[col]; }
    unsigned int getWeight(const State& state) const;
    unsigned int getMinRevWeight(const State& state) const;

    AffineSpaceOfStates buildStateBase(const State& state, bool reverse) const;
    EnvelopeSpaceOfStates buildEnvelopeSpaceBase(const State& state, bool reverse) const;

    void backwardExtendTrail(ostream& fout, const TrailCore& trail, unsigned int maxTotalWeight, unsigned int maxWeightAtB, unsigned int& minWeightFound, bool weightedWeights = false);
    void forwardExtendTrail(ostream& fout, const TrailCore& trail, unsigned int maxTotalWeight, unsigned int& minWeightFound, bool weightedWeights = false);

    bool checkCompatibility(const Column& columnAtB, Column& columnAtA) const;
    bool checkCompatibility(const State& stateAtB, State& stateAtA) const;
};

class TrailCore {
public:
    const Propagation& DCorLC;
    vector<State> states;
    vector<unsigned int> weights;
    unsigned int minRevWeight;
    unsigned int totalWeight;
public:
    /** This constructor creates an empty trail.
    */
    TrailCore(const Propagation& aDCorLC);
    /** This constructor initializes a trail by copying the
      * trail given in parameter.
      * @param   other  The original trail to copy.
      */
    TrailCore(const TrailCore& other)
        : DCorLC(other.DCorLC),
        states(other.states),
        weights(other.weights),
        minRevWeight(other.minRevWeight),
        totalWeight(other.totalWeight) {}
    /** This method appends a state to the end of @a states,
      * with its corresponding propagation weight.
      * @param   state  The state to add.
      * @param   weight The propagation weight.
      */
    void append(const State& stateAtB, unsigned int weight);
    /** This method inserts a state at the beginning of @a states,
      * with its corresponding propagation weight.
      * @param   state  The state to add.
      * @param   weight The propagation weight.
      */
    void prepend(const State& stateAtB, unsigned int weight);
    /** This method sets the minimum reverse weight.
      * @param   minRevWeight The minimum reverse weight.
      */
    void setMinRevWeight(unsigned int aMinRevWeight);
    /** This methods loads the trail from a stream (e.g., file).
      * @param   fin    The input stream to read the trail from.
      */
    void load(istream& fin);
    /** This methods outputs the trail to save it in, e.g., a file.
      * @param  fout    The stream to save the trail to.
      */
    void save(ostream& fout) const;

    void display(ostream& fout) const;

    TrailCore& operator=(TrailCore& other);
};

class Trail {
public:
    const Propagation& DCorLC;
    vector<State> states;
    vector<unsigned int> weights;
    unsigned int totalWeight;
public:
    /** This constructor creates an empty trail.
    */
    Trail(const Propagation& aDCorLC);
    /** This constructor initializes a trail by copying the
      * trail given in parameter.
      * @param   other  The original trail to copy.
      */
    Trail(const TrailCore& other)
        : DCorLC(other.DCorLC),
        states(other.states),
        weights(other.weights),
        totalWeight(other.totalWeight-other.minRevWeight) {}
    /** This method appends a state to the end of @a states,
      * with its corresponding propagation weight.
      * @param   state  The state to add.
      * @param   weight The propagation weight.
      */
    void append(const State& stateAtB, unsigned int weight);
    /** This method inserts a state at the beginning of @a states,
      * with its corresponding propagation weight.
      * @param   state  The state to add.
      * @param   weight The propagation weight.
      */
    void prepend(const State& stateAtB, unsigned int weight);
    /** This methods outputs the trail to save it in, e.g., a file.
      * @param  fout    The stream to save the trail to.
      */
    void save(ostream& fout) const;
    /** This methods loads the trail from a stream (e.g., file).
      * @param   fin    The input stream to read the trail from.
      */
    void load(istream& fin);

    void display(ostream& fout) const;
};


/** This class implements an iterator over the possible state values
  * before the non-linear layer given a state after the non-linear layer.
  */
class ReverseStateIterator
{
private:
    State stateB; // stateB
    State stateA; // Lin^-1(B)
    vector<vector<Column>> patterns;
    vector<vector<State>> patternsBeforeLin;
    vector<StateMask> stabilityMasks;
    vector<vector<unsigned int>> weights;
    vector<unsigned int> activeColumns; 
    vector<unsigned int> indexes;
    unsigned int nrActiveColumns;
    unsigned int minWeightAtB, maxWeightAtB, maxWeight;
    unsigned int currentWeightAtB;
    //unsigned int currentBoundMinRevWeight;
    uint64_t index;
    bool end;
public:
    /** This constructor initializes the iterator based on a state value after the non-linear layer.
      * With this constructor, the iterator runs through all the possible states regardless of their weight.
      * @param   stateA  The state value after the non-linear layer.
      * @param   DCorLC  A reference to the Propagation instance that
      *                  determines the type of propagation.
      */
    ReverseStateIterator(const State& aStateA, const Propagation& DCorLC);
    /** This constructor initializes the iterator based on a state value after the non-linear layer,
      * and a maximum weight.
      * @param   stateA         The state value after the non-linear layer.
      * @param   DCorLC         A reference to the Propagation instance that
      *                         determines the type of propagation.
      * @param   aMaxWeight     The iterator will run through the states whose propagation
      *                         weight is not higher than this parameter.
      */
    ReverseStateIterator(const State& aStateA, const Propagation& DCorLC, unsigned int aMaxWeightAtB, unsigned int aMaxWeight);
    /** This method tells whether the iterator has reached the end of the possible states.
      * @return It returns true iff there are no more states to run through.
      */
    bool isEnd() const;
    /** This method tells wether the set of states to run through is empty.
      * @return It returns true iff there are no states to run through.
      */
    bool isEmpty() const;
    /** This method moves the iterator to the next state. */
    void operator++();
    /** This method returns a constant reference to the current state.
      * @return A constant reference to the current state as a vector of slices.
      */
    const State& operator*() const;
    /** This method returns the propagation weight of the current state.
      * @return The weight of the current state.
      */
    unsigned int getCurrentWeight() const;
private:
    void initialize(const State& stateAfterChi, const Propagation& DCorLC);
    void next();
};

void upperTriangularizeBasis(const vector<State>& originalBasis, vector<State>& newBasis, vector<Coordinates>& stability);
void upperTriangularizeBasis(const vector<State>& originalBasisAtA, const vector<State>& originalBasisAtB, vector<State>& newBasisAtA, vector<State>& newBasisAtB, vector<Coordinates>& stability);

void generateTrailsInCore(ostream& fout, const TrailCore& trailCore, unsigned int maxTotalWeight);
vector<unsigned int> countActiveSboxesInCore(const TrailCore& trailCore);

#endif