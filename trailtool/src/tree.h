/*
AsconTrailTool, a set of C++ classes for analyzing ASCON.

For specifications, please refer to https://tosc.iacr.org/index.php/ToSC/article/view/9975

Implementation by Silvia Mella, hereby denoted as "the implementer".
*/

/*! \file Tree.h

This file defines the components for a general tree structure.
*/

#ifndef _TREE_
#define _TREE_

#include <cstdint>
#include <iostream>
#include <stack>
#include <vector>

using namespace std;


template<class Unit>
class UnitList : public std::vector<Unit> {};

/**
* This exception is launched when a set of units reaches the end.
*/
class EndOfSet {};

/**
* \brief GenericTreeIterator class : Iterator to traverse a Tree.
*
* \details This class represents an iterator to traverse a tree.
* The type of tree is defined by the unitList.
*/
template<class Unit, class UnitSet, class Shadow, class OutputRepresentation, class CostFunction>
class GenericTreeIterator {
protected:
	/** The set of units */
	UnitSet& unitSet;
	/** The unit-list representing the current node. */
	vector<Unit> unitList;
	/** The cache representation of the current node. */
	Shadow shadow;
	/** The output representation of the current node. */
	OutputRepresentation out;
	/** The cost function. */
	CostFunction& costFunction;
	/** The maximum cost allowed when traversing the tree. */
	unsigned int maxCost;

	/** Attribute that indicates whether the iterator has reached the end. */
	bool end;
	/** Attribute that indicates whether the iterator has been initialized. */
	bool initialized;
	/** Attribute that indicates whether the tree is empty. */
	bool empty;
	/** Number of the current iteration. */
	uint64_t index;

public:

	/** The constructor.
   * @param  aUnitSet      The set of units.
   * @param  aShadow       The context needed to initialize the shadow and output representations.
   * @param  aCostFunction The cost function.
   * @param  aMaxCost      The maximum cost.
   */
	GenericTreeIterator(UnitSet& aUnitSet, Shadow& aShadow, CostFunction& aCostFunction, unsigned int aMaxCost)
		: unitSet(aUnitSet), shadow(aShadow), out(aShadow), costFunction(aCostFunction), maxCost(aMaxCost)
	{
		empty = true;
		end = false;
		initialized = false;
		index = 0;
	}

	/** This method indicates whether the iterator has reached the end of the tree.
	*  @return True if there are no more nodes to consider.
	*/
	bool isEnd()
	{
		if (!initialized) 
			initialize();
		return end;
	}

	/** This method indicates whether the tree is empty.
	*  @return True if the tree is empty.
	*/
	bool isEmpty()
	{
		if (!initialized) 
			initialize();
		return empty;
	}

	/** This method returns the index of the current node considered.
	* @return  The current node index.
	*/
	uint64_t getIndex() const
	{
		return index;
	}

	/** The '++' operator moves the iterator to the next node in the tree.
	*/
	void operator++()
	{
		if (!initialized) {
			initialize();
		}
		else {
			if (!end) {
				++index;
				if (!next())
					end = true;
			}
		}
	}

	/** The '*' operator gives a constant reference to the current node.
	*  @return A constant reference to the current node.
	*/
	const OutputRepresentation& operator*()
	{
		out.set(unitList, shadow);
		return out;
	}

private:

	/** This method initializes the iterator.
	 * It goes to the first acceptable node,
     * or sets end = empty = true if none exists.
	*/
	void initialize()
	{
		index = 0;
		if (first()) {
			end = false;
			empty = false;
		}
		else {
			end = true;
			empty = true;
		}
		initialized = true;
	}

	/** This method returns the first node of the tree.
	* @return true if the first node exists, false otherwise.
	*/
	bool first()
	{
		while (!canAcceptNode()) {
			if (!next()) {
				return false;
			}
		}
		return true;
	}

	/** Method to determine whether the current node is acceptable.
	 * It might test if the node is well-formed (using the unit set), or
	 * if the cost is not above the maximum (using the cost function), or
	 * if the node is canonical (using the unit set).
	 */
	virtual bool canAcceptNode()
	{
		if (costFunction.getSubtreeLowerBound(unitList, shadow) > maxCost) {
			return false;
		}
		return true;
	}

	/** This method returns the next node of the tree.
	* @return true if the next node exists, false otherwise.
	*/
	bool next()
	{
		if (toChild()) {
			if(canAcceptNode())
				return true;
		}
		do{
			while (toSibling()) {
				if (canAcceptNode())
					return true;			
			}
		} while (toParent());
		return false;
	}

	/** This method move to the first child of the current node.
	* A child is obtained adding a new unit to the current node.
	* @return true if a child is found, false otherwise.
	*/
	bool toChild()
	{
		try {
			Unit newUnit = unitSet.getFirstChildUnit(unitList, shadow);
			push(newUnit);
			return true;
		}
		catch (EndOfSet) {
			return false;
		}
	}

	/** This method move to the first sibling of the current node.
	* Siblings are obtained iterating the higher unit of the current node.
	* @return true if a sibling is found, false otherwise.
	*/
	bool toSibling()
	{
		try {
			if (unitList.empty())
				return false;
			else {
				Unit lastUnit = unitList.back();
				unitSet.iterateUnit(unitList, lastUnit, shadow);
				pop();
				push(lastUnit);
				return true;
			}
		}
		catch (EndOfSet) {
			return false;
		}
	}

	/** This method move to the parent of the current node.
	* The parent is obtained by removing the higher unit of the current node.
	* @return true if the node has a parent, false if it is the root.
	*/
	bool toParent()
	{
		if (unitList.empty())
			return false;
		else {
			pop();
			return true;
		}
	}

	/**
	* This method pushes a new unit to the unit list and updates the cost function.
	* @param newUnit the unit to be pushed.
	*/
	void push(Unit& newUnit)
	{			
		unitList.push_back(newUnit);
		shadow.push(newUnit);
	}

	/**
	* This method pops the highest unit from the unit list and updates the cost function.
	* @return true if a unit is popped, false if the unit list is empty.
	*/
	void pop()
	{
		shadow.pop(unitList.back());
		unitList.pop_back();
	}
	
};


#endif