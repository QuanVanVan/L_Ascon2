/*
AsconTrailTool, a set of C++ classes for analyzing ASCON.

For specifications, please refer to https://tosc.iacr.org/index.php/ToSC/article/view/9975

Implementation by Silvia Mella, hereby denoted as "the implementer".
*/

#include "asconParts.h"

inline uint64_t ROR(uint64_t x, int n) {
	return (x << (nrCols - n)) | (x >> n);
}

State ROR(State inputState, int n) {
	State rotatedState;
	for (unsigned int i = 0; i < nrRows; i++) {
		rotatedState.setRow(i, ROR(inputState.getRow(i), n));
	}
	return rotatedState;
}

void displayRow(ostream& fout, Row x, bool compact) {
	if (compact) {
		fout << uppercase << hex;
		fout.width(16);
		fout.fill('0');
		right(fout);
		fout << x << endl;
	}
	else {
		for (int j = 0; j < nrCols; j++) {
			string s = ((x >> j) & 1) ? "1" : ".";
			fout << s;
		}
		fout << endl;
	}
}

void asconLinearLayer(State& s)
{
	Row x0 = s.getRow(0);
	Row x1 = s.getRow(1);
	Row x2 = s.getRow(2);
	Row x3 = s.getRow(3);
	Row x4 = s.getRow(4);

	x0 ^= ROR(x0, 19) ^ ROR(x0, 28);
	x1 ^= ROR(x1, 61) ^ ROR(x1, 39);
	x2 ^= ROR(x2, 1) ^ ROR(x2, 6);
	x3 ^= ROR(x3, 10) ^ ROR(x3, 17);
	x4 ^= ROR(x4, 7) ^ ROR(x4, 41);

	s.setRow(0, x0);
	s.setRow(1, x1);
	s.setRow(2, x2);
	s.setRow(3, x3);
	s.setRow(4, x4);
}

void asconInverseLinearLayer(State& s)
{
	Row x0 = s.getRow(0);
	Row x1 = s.getRow(1);
	Row x2 = s.getRow(2);
	Row x3 = s.getRow(3);
	Row x4 = s.getRow(4);

	x0 ^= ROR(x0, 3) ^ ROR(x0, 6) ^ ROR(x0, 9) ^ ROR(x0, 11) ^ ROR(x0, 12) ^ ROR(x0, 14) ^ ROR(x0, 15) ^ ROR(x0, 17) ^ ROR(x0, 18) ^ ROR(x0, 19) ^ ROR(x0, 21) ^ ROR(x0, 22) ^ ROR(x0, 24) ^ ROR(x0, 25) ^ ROR(x0, 27) ^ ROR(x0, 30) ^ ROR(x0, 33) ^ ROR(x0, 36) ^ ROR(x0, 38) ^ ROR(x0, 39) ^ ROR(x0, 41) ^ ROR(x0, 42) ^ ROR(x0, 44) ^ ROR(x0, 45) ^ ROR(x0, 47) ^ ROR(x0, 50) ^ ROR(x0, 53) ^ ROR(x0, 57) ^ ROR(x0, 60) ^ ROR(x0, 63);
	x1 ^= ROR(x1, 1) ^ ROR(x1, 2) ^ ROR(x1, 3) ^ ROR(x1, 4) ^ ROR(x1, 8) ^ ROR(x1, 11) ^ ROR(x1, 13) ^ ROR(x1, 14) ^ ROR(x1, 16) ^ ROR(x1, 19) ^ ROR(x1, 21) ^ ROR(x1, 23) ^ ROR(x1, 24) ^ ROR(x1, 25) ^ ROR(x1, 27) ^ ROR(x1, 28) ^ ROR(x1, 29) ^ ROR(x1, 30) ^ ROR(x1, 35) ^ ROR(x1, 39) ^ ROR(x1, 43) ^ ROR(x1, 44) ^ ROR(x1, 45) ^ ROR(x1, 47) ^ ROR(x1, 48) ^ ROR(x1, 51) ^ ROR(x1, 53) ^ ROR(x1, 54) ^ ROR(x1, 55) ^ ROR(x1, 57) ^ ROR(x1, 60) ^ ROR(x1, 61);
	x2 ^= ROR(x2, 2) ^ ROR(x2, 4) ^ ROR(x2, 6) ^ ROR(x2, 7) ^ ROR(x2, 10) ^ ROR(x2, 11) ^ ROR(x2, 13) ^ ROR(x2, 14) ^ ROR(x2, 15) ^ ROR(x2, 17) ^ ROR(x2, 18) ^ ROR(x2, 20) ^ ROR(x2, 23) ^ ROR(x2, 26) ^ ROR(x2, 27) ^ ROR(x2, 28) ^ ROR(x2, 32) ^ ROR(x2, 34) ^ ROR(x2, 35) ^ ROR(x2, 36) ^ ROR(x2, 37) ^ ROR(x2, 40) ^ ROR(x2, 42) ^ ROR(x2, 46) ^ ROR(x2, 47) ^ ROR(x2, 52) ^ ROR(x2, 58) ^ ROR(x2, 59) ^ ROR(x2, 60) ^ ROR(x2, 61) ^ ROR(x2, 62) ^ ROR(x2, 63);
	x3 = ROR(x3, 1) ^ ROR(x3, 2) ^ ROR(x3, 4) ^ ROR(x3, 6) ^ ROR(x3, 7) ^ ROR(x3, 9) ^ ROR(x3, 12) ^ ROR(x3, 17) ^ ROR(x3, 18) ^ ROR(x3, 21) ^ ROR(x3, 22) ^ ROR(x3, 23) ^ ROR(x3, 24) ^ ROR(x3, 26) ^ ROR(x3, 27) ^ ROR(x3, 28) ^ ROR(x3, 29) ^ ROR(x3, 31) ^ ROR(x3, 32) ^ ROR(x3, 33) ^ ROR(x3, 35) ^ ROR(x3, 36) ^ ROR(x3, 37) ^ ROR(x3, 40) ^ ROR(x3, 42) ^ ROR(x3, 44) ^ ROR(x3, 47) ^ ROR(x3, 48) ^ ROR(x3, 49) ^ ROR(x3, 53) ^ ROR(x3, 58) ^ ROR(x3, 61) ^ ROR(x3, 63);
	x4 ^= ROR(x4, 1) ^ ROR(x4, 2) ^ ROR(x4, 3) ^ ROR(x4, 4) ^ ROR(x4, 5) ^ ROR(x4, 9) ^ ROR(x4, 10) ^ ROR(x4, 11) ^ ROR(x4, 13) ^ ROR(x4, 16) ^ ROR(x4, 20) ^ ROR(x4, 21) ^ ROR(x4, 22) ^ ROR(x4, 24) ^ ROR(x4, 25) ^ ROR(x4, 28) ^ ROR(x4, 29) ^ ROR(x4, 30) ^ ROR(x4, 31) ^ ROR(x4, 35) ^ ROR(x4, 36) ^ ROR(x4, 40) ^ ROR(x4, 41) ^ ROR(x4, 44) ^ ROR(x4, 45) ^ ROR(x4, 46) ^ ROR(x4, 47) ^ ROR(x4, 48) ^ ROR(x4, 50) ^ ROR(x4, 53) ^ ROR(x4, 55) ^ ROR(x4, 60) ^ ROR(x4, 61) ^ ROR(x4, 63);

	s.setRow(0, x0);
	s.setRow(1, x1);
	s.setRow(2, x2);
	s.setRow(3, x3);
	s.setRow(4, x4);
}

void asconTransposeLinearLayer(State& s)
{
	Row x0 = s.getRow(0);
	Row x1 = s.getRow(1);
	Row x2 = s.getRow(2);
	Row x3 = s.getRow(3);
	Row x4 = s.getRow(4);

	x0 ^= ROR(x0, 45) ^ ROR(x0, 36); // -19 -28
	x1 ^= ROR(x1, 3) ^ ROR(x1, 25); // -61 -39
	x2 ^= ROR(x2, 63) ^ ROR(x2, 58); // -1 -6
	x3 ^= ROR(x3, 54) ^ ROR(x3, 47); // -10 -17
	x4 ^= ROR(x4, 57) ^ ROR(x4, 23); // -7 -41

	s.setRow(0, x0);
	s.setRow(1, x1);
	s.setRow(2, x2);
	s.setRow(3, x3);
	s.setRow(4, x4);
}

void asconInverseTransposeLinearLayer(State& s)
{
	Row x0 = s.getRow(0);
	Row x1 = s.getRow(1);
	Row x2 = s.getRow(2);
	Row x3 = s.getRow(3);
	Row x4 = s.getRow(4);

	x0 = ROR(x0, 0) ^ ROR(x0, 1) ^ ROR(x0, 4) ^ ROR(x0, 7) ^ ROR(x0, 11) ^ ROR(x0, 14) ^ ROR(x0, 17) ^ ROR(x0, 19) ^ ROR(x0, 20) ^ ROR(x0, 22) ^ ROR(x0, 23) ^ ROR(x0, 25) ^ ROR(x0, 26) ^ ROR(x0, 28) ^ ROR(x0, 31) ^ ROR(x0, 34) ^ ROR(x0, 37) ^ ROR(x0, 39) ^ ROR(x0, 40) ^ ROR(x0, 42) ^ ROR(x0, 43) ^ ROR(x0, 45) ^ ROR(x0, 46) ^ ROR(x0, 47) ^ ROR(x0, 49) ^ ROR(x0, 50) ^ ROR(x0, 52) ^ ROR(x0, 53) ^ ROR(x0, 55) ^ ROR(x0, 58) ^ ROR(x0, 61);
	x1 = ROR(x1, 0) ^ ROR(x1, 3) ^ ROR(x1, 4) ^ ROR(x1, 7) ^ ROR(x1, 9) ^ ROR(x1, 10) ^ ROR(x1, 11) ^ ROR(x1, 13) ^ ROR(x1, 16) ^ ROR(x1, 17) ^ ROR(x1, 19) ^ ROR(x1, 20) ^ ROR(x1, 21) ^ ROR(x1, 25) ^ ROR(x1, 29) ^ ROR(x1, 34) ^ ROR(x1, 35) ^ ROR(x1, 36) ^ ROR(x1, 37) ^ ROR(x1, 39) ^ ROR(x1, 40) ^ ROR(x1, 41) ^ ROR(x1, 43) ^ ROR(x1, 45) ^ ROR(x1, 48) ^ ROR(x1, 50) ^ ROR(x1, 51) ^ ROR(x1, 53) ^ ROR(x1, 56) ^ ROR(x1, 60) ^ ROR(x1, 61) ^ ROR(x1, 62) ^ ROR(x1, 63);
	x2 = ROR(x2, 0) ^ ROR(x2, 1) ^ ROR(x2, 2) ^ ROR(x2, 3) ^ ROR(x2, 4) ^ ROR(x2, 5) ^ ROR(x2, 6) ^ ROR(x2, 12) ^ ROR(x2, 17) ^ ROR(x2, 18) ^ ROR(x2, 22) ^ ROR(x2, 24) ^ ROR(x2, 27) ^ ROR(x2, 28) ^ ROR(x2, 29) ^ ROR(x2, 30) ^ ROR(x2, 32) ^ ROR(x2, 36) ^ ROR(x2, 37) ^ ROR(x2, 38) ^ ROR(x2, 41) ^ ROR(x2, 44) ^ ROR(x2, 46) ^ ROR(x2, 47) ^ ROR(x2, 49) ^ ROR(x2, 50) ^ ROR(x2, 51) ^ ROR(x2, 53) ^ ROR(x2, 54) ^ ROR(x2, 57) ^ ROR(x2, 58) ^ ROR(x2, 60) ^ ROR(x2, 62);
	x3 = ROR(x3, 1) ^ ROR(x3, 3) ^ ROR(x3, 6) ^ ROR(x3, 11) ^ ROR(x3, 15) ^ ROR(x3, 16) ^ ROR(x3, 17) ^ ROR(x3, 20) ^ ROR(x3, 22) ^ ROR(x3, 24) ^ ROR(x3, 27) ^ ROR(x3, 28) ^ ROR(x3, 29) ^ ROR(x3, 31) ^ ROR(x3, 32) ^ ROR(x3, 33) ^ ROR(x3, 35) ^ ROR(x3, 36) ^ ROR(x3, 37) ^ ROR(x3, 38) ^ ROR(x3, 40) ^ ROR(x3, 41) ^ ROR(x3, 42) ^ ROR(x3, 43) ^ ROR(x3, 46) ^ ROR(x3, 47) ^ ROR(x3, 52) ^ ROR(x3, 55) ^ ROR(x3, 57) ^ ROR(x3, 58) ^ ROR(x3, 60) ^ ROR(x3, 62) ^ ROR(x3, 63);
	x4 = ROR(x4, 0) ^ ROR(x4, 1) ^ ROR(x4, 3) ^ ROR(x4, 4) ^ ROR(x4, 9) ^ ROR(x4, 11) ^ ROR(x4, 14) ^ ROR(x4, 16) ^ ROR(x4, 17) ^ ROR(x4, 18) ^ ROR(x4, 19) ^ ROR(x4, 20) ^ ROR(x4, 23) ^ ROR(x4, 24) ^ ROR(x4, 28) ^ ROR(x4, 29) ^ ROR(x4, 33) ^ ROR(x4, 34) ^ ROR(x4, 35) ^ ROR(x4, 36) ^ ROR(x4, 39) ^ ROR(x4, 40) ^ ROR(x4, 42) ^ ROR(x4, 43) ^ ROR(x4, 44) ^ ROR(x4, 48) ^ ROR(x4, 51) ^ ROR(x4, 53) ^ ROR(x4, 54) ^ ROR(x4, 55) ^ ROR(x4, 59) ^ ROR(x4, 60) ^ ROR(x4, 61) ^ ROR(x4, 62) ^ ROR(x4, 63);
    
	s.setRow(0, x0);
	s.setRow(1, x1);
	s.setRow(2, x2);
	s.setRow(3, x3);
	s.setRow(4, x4);
}

void State::clear()
{
	for (unsigned int i = 0; i < rows.size(); i++)
		rows[i] = 0;
}

unsigned int State::getNrActiveColumns()
{
	unsigned int a = 0;
	Row activeColumns = rows[0] | rows[1] | rows[2] | rows[3] | rows[4];
	for (unsigned int i = 0; i < nrCols; i++)
		a += ((activeColumns >> i) & 1) != 0 ? 1 : 0;
	return a;
}

State& State::operator^=(const State& other)
{
	for (unsigned int i = 0; i < rows.size(); i++)
		rows[i] ^= other.rows[i];
	return *this;
}

State& State::operator|=(const State& other)
{
	for (unsigned int i = 0; i < rows.size(); i++)
		rows[i] |= other.rows[i];
	return *this;
}

State& State::operator&=(const State& other)
{
	for (unsigned int i = 0; i < rows.size(); i++)
		rows[i] &= other.rows[i];
	return *this;
}

Row State::getParity() const
{
	Row parity(0);
	parity ^= getRow(0);
	parity ^= getRow(1);
	parity ^= getRow(2);
	parity ^= getRow(3);
	parity ^= getRow(4);
	
	return parity;
}

Row State::getActiveColumns() const
{
	Row activeCols = getRow(0);
	activeCols |= getRow(1);
	activeCols |= getRow(2);
	activeCols |= getRow(3);
	activeCols |= getRow(4);
	return activeCols;
}

void State::display(ostream& fout, bool compact) const
{
	for (int i = 0; i < nrRows; i++) {
		Row x = getRow(i);
		displayRow(fout,x,compact);
	}
	fout << endl;
}
