/*
AsconTrailTool, a set of C++ classes for analyzing ASCON.

For specifications, please refer to https://tosc.iacr.org/index.php/ToSC/article/view/9975

Implementation by Silvia Mella, hereby denoted as "the implementer".
*/

#ifndef _ASCONPARTS_H_
#define _ASCONPARTS_H_

#include <cstdint>
#include <vector>
#include <iostream>

#include "config.h"

using namespace std;


/** The number of rows is 5
  
const int nrRows = 5;*/

/** The number of columns is 64
  
const int nrCols = 64;*/

/** The Row type is 64 bits
  */
typedef uint64_t Row;

/** The Column type is one byte, containing the 5
  * bits of a column, in the least significant bits of the byte.
  */
typedef unsigned char Column;

class Coordinates {
public:
    unsigned int x; // the row
    unsigned int z; // the column
public:
    Coordinates()
        : x(0), z(0) {}
    Coordinates(unsigned int ax, unsigned int az)
        : x(ax), z(az) {}
};

class State {
protected:
    Row rows[5]; // DATN: fixed array instead of vector<Row> (no heap allocation per State)
public:
    State()
        : rows{0, 0, 0, 0, 0} {}

     /** This method clears the state.
      */
    void clear();
    /** This method returns the value of a given row in the state.
      *
      * @param  nrow The row index
      */
    Row getRow(unsigned int nrow) const { return rows[nrow]; }
    /** This method sets the value of a given row in the state.
      *
      * @param  nrow The row index
      * @param  rowVal The row value
      */
    void setRow(unsigned int nrow, Row rowVal) { rows[nrow] = rowVal; }
    /** This method returns the value of a given bit in the state.
      *
      * @param  nrow The row index
      * @param  ncol The column index
      */
    inline int getBit(unsigned int nrow, unsigned int ncol) const { return (rows[nrow] >> ncol) & 1; }
    /** This method sets to 0 a particular bit in the state.
      *
      * @param  col   The col coordinate.
      * @param  row   The row coordinate.
      */
    inline void setBitToZero(unsigned int row, unsigned int col)
    {
        rows[row] &= ~((Row)1 << col);
    }

    /** This method sets to 1 a particular bit in the state.
      *
      * @param  col   The col coordinate.
      * @param  row   The row coordinate.
      */
    inline void setBitToOne(unsigned int row, unsigned int col)
    {
        rows[row] |= (Row)1 << col;
    }
    /** This method returns the value of a given column in the state, where the MSB is in row 0 and the LSB in row 4.
      *
      * @param  ncol The column index
      */
    inline Column getColumn(unsigned int ncol) const { 
        Column column = getBit(4, ncol) + 2 * getBit(3, ncol) + 4 * getBit(2, ncol) + 8 * getBit(1, ncol) + 16 * getBit(0, ncol);
        return column;
    }
    /** This method sets the value of a given column in the state. Note that the MSB goes to row 0 and the LSB to row 4.
      *
      * @param  ncol The row index
      * @param  value The row value
      */
    inline void setColumn(unsigned int ncol, const Column& value)
    {
        if (value & 1) setBitToOne(4, ncol); else setBitToZero(4, ncol);
        if (value & 2) setBitToOne(3, ncol); else setBitToZero(3, ncol);
        if (value & 4) setBitToOne(2, ncol); else setBitToZero(2, ncol);
        if (value & 8) setBitToOne(1, ncol); else setBitToZero(1, ncol);
        if (value & 16) setBitToOne(0, ncol); else setBitToZero(0, ncol);
    }

    /** This method returns the number of active columns in the state.
    */
    unsigned int getNrActiveColumns();

    State& operator^=(const State& other);
    State& operator|=(const State& other);
    State& operator&=(const State& other);

    Row getParity() const;

    Row getActiveColumns() const;

    void display(ostream& fout, bool compact = true) const;

};

/* linear diffusion layer */
inline uint64_t ROR(uint64_t x, int n);
void displayRow(ostream& fout, Row x, bool compact = true);
void asconLinearLayer(State& s);
void asconInverseLinearLayer(State& s);
void asconTransposeLinearLayer(State& s);
void asconInverseTransposeLinearLayer(State& s);


#endif