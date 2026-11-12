#####################################
##         AsconTrailTool          ##
#####################################

This file contains a description of how to use AsconTrailTool.
Several examples are given.
Notably, examples are given to replicate the results for 3 rounds, which is the cheapest search we performed.

CONTENT OF THIS FILE
====================
- PACKAGE CONTENT
- DESCRIPTION
- USAGE
- HOW TO REPLICATE THE SEARCH FOR 3 ROUNDS


PACKAGE CONTENT
===============

This package contains:
- the source files of AsconTrailTool in folder src/
- a makefile 
- this readme file


DESCRIPTION
===============

The user can use the AsconTrailTool to
A) generate 2 round trail cores with weight up to T
B) extend trail cores by one round up to weight T
C) extract from a file the trail cores with weight in a given range
D) display trail cores


USAGE
===============

To use the tool, the user can perform the following steps 

0) prepare the config.h file
	The constant DTS on line 9 has to be set with one of the following two values:
		1 if the user wants to perform differential trail search
		0 if the user wants to perform linear trail search

1) compile the code from ./ folder with the command  
	> make
   A folder bin/ will be created containing the executable AsconTrailTool

2) execute the tool from ./ folder with proper inputs with the command
    > .bin/AsconTrailTool <OPERATION> [INPUTS]
  Output files will be produced based on the operation executed

We now explain how to properly set <OPERATION> and [INPUTS] for performing each of the steps given in DESCRIPTION.

A) generate 2 round trail cores with weight up to a given value
---------------------------------------------------------------

OPERATION = generate
[INPUTS] = no input

Only for this command, the config.h file in folder src/ has to be set.

The following constants have to be set:
line 47: limitWeight, which defines the maximum weight T
line 48: weightedWeights, which defines whether the 2-round trail cores will be used for the search over 3 rounds or more.
         It can assume one of the following values:
			11 which means that 2-round trail cores with wrev(a)+w(b) <= T will be generated
            12 which means that 2-round trail cores with wrev(a)+2w(b) <= T will be generated
		    21 which means that 2-round trail cores with 2wrev(a)+w(b) <= T will be generated
		Please refer to the paper, sec 6, for more details.
		
The program will output 3 files, whose name starts with "DTS" if DTS=1 or with "LTS" if DTS=0.
Here we use "DTS" for the sake of explanation.
- DTS-columnHistogramAscon.csv: this file contains an histogram with the number of 2-round trail cores per number of active S-boxes at a and b
- DTS-trailCoresHistogramAscon.csv: this file contains an histogram with the number of 2-round trail cores per weight at a and b
- DTS-2RTrailCoresUpToWeight<T>_<weights>: this file contains all 2-round trail cores (a,b) up to weight T and weight profile <weights>, in a compact representation, as a list of values that are:
	<number of rounds> <total weight> <wrev(a)> <w(b)> <row 0 of b> <row 1 of b> <row 2 of b> <row 3 of b> <row 4 of b>
	The name of the file changes based on the values set for limitWeight and weightedWeights.
	
The name of the files DTS-columnHistogramAscon.csv and DTS-trailCoresHistogramAscon.csv can be modified in the file config.h.
If files DTS-columnHistogramAscon.csv and DTS-trailCoresHistogramAscon.csv already exist, the new results will be appended at the end.
The third file will be overwritten.
	
*******	
EXAMPLE 1

To generate all 2-round linear trail cores with weight wrev(a)+w(b) <= 20 , the user must
a) set the constants in the config file as follows:
  #define DTS 0
  static const int limitWeight = 20;
  static const int weightedWeights = 11;
b) recompile
c) launch the command 
	> ./bin/AsconTrailTool generate
This will ouptut the three files:
- LTS-columnHistogramAscon.csv		
- LTS-trailCoresHistogramAscon.csv
- LTS-2RTrailCoresUpToWeight20_w0+w1

END OF EXAMPLE
**************

*******	
EXAMPLE 2

To generate the 2-round linear trail cores to prove bounds for 3 rounds (as in sec 6.1), the user can perform the following steps.

1) generate all 2-round linear trail cores with 2wrev(a)+w(b) <= 26. To this end:
	a) set the constants in the config file as follows:
	  #define DTS 0
	  static const int limitWeight = 26;
	  static const int weightedWeights = 21;
	b) recompile 
		> make -B
	c) launch the command 
		> ./bin/AsconTrailTool generate
		
	This will ouptut the three files:
	- LTS-columnHistogramAscon.csv		
	- LTS-trailCoresHistogramAscon.csv
	- LTS-2RTrailCoresUpToWeight26_2w0+w1, here w0 stays for wrev(a) and w1 stays for w(b)
	
2) generate all 2-round linear trail cores with wrev(a)+2w(b) <= 28. To this end:
	a) set the constants in the config file as follows:
	  #define DTS 0
	  static const int limitWeight = 28;
	  static const int weightedWeights = 12;
	b) recompile 
		> make -B
	c) launch the command 
		> ./bin/AsconTrailTool generate
		
	This will update the two files:
	- LTS-columnHistogramAscon.csv		
	- LTS-trailCoresHistogramAscon.csv
	and will output the new file:
	- LTS-2RTrailCoresUpToWeight28_w0+2w1, here w0 stays for wrev(a) and w1 stays for w(b)

END OF EXAMPLE
**************

B) extend trail cores by one round up to weight T
---------------------------------------------------------------

There are two commands for extension.
The first one is used only to extend 2-round trail cores with 2wrev(a)+w(b) <= T or wrev(a)+2w(b) <= T.
The second one is used to extend 2-round trail cores with wrev(a)+w(b) <= T or any r-round trail core with r>2.

B1) extend 2-round trail cores with 2wrev(a)+w(b) <= T or wrev(a)+2w(b) <= T
----------------------------------------------------------------------------

OPERATION = extendWW
[INPUTS] = <direction> <targetWeight>
			- <direction>: the direction for extension. It can assume one of the following two values:
				forward: keyword to perform forward extension
				backward: keyword to perform backward extension
			- <targetWeight>: the target weight T up to which trail cores are extended

This automatically looks for the proper input file depending on the keyword <direction> specified.
It outputs a file whose name depends on the input file, direction and targetWeight.

*******	
EXAMPLE 3

To extend all 2-round linear trail cores as in sec 6.1, the user can perform the following steps.

1) extend all 2-round linear trail cores with 2wrev(a)+w(b) <= 26 up to 28. To this end, launch the command 
		> ./bin/AsconTrailTool extendWW forward 28
		
	This will ouptut the file:
	- LTS-2RTrailCoresUpToWeight26_2w0+w1-forwUpTo28
	
2) extend all 2-round linear trail cores with wrev(a)+2w(b) <= 28 up to 28. To this end, launch the command 
		> ./bin/AsconTrailTool extendWW backward 28
		
	This will ouptut the file:
	- LTS-2RTrailCoresUpToWeight28_w0+2w1-backUpTo28

END OF EXAMPLE
**************

B2) extend any other trail core 
---------------------------------

OPERATION = extend
[INPUTS] = a list of values that depends on whether the user wants to perform forward or backward extension. The values are
			- <direction>: keyword that specifies the direction for extension. It can assume one of the following two values:
				forward: keyword to perform forward extension
				backward: keyword to perform backward extension
			- <iFileName>: file containing the trail cores to be extended
		    - <maxWeight>: the target weight up to which trail cores are extended
			- <envelope>: boolean value to specify whether backward extension is performed with envelope space or not (see sec. 5 for more details).
				false: backward extension is performed using compatible patterns (sec. 5.3.1)
				true: backward extension is performed using the envelope space (sec 5.3.2)
			- <maxWeightStart>: integer value to limit the growth of w(b0) (sec. 5.3.1 for more details)

It outputs a file whose name depends on the input file, direction and targetWeight.

*******	
EXAMPLE 4

To extend all 2-round linear trail cores of EXAMPLE 1 up to weight 40, the user can perform the following steps.

1) extend in the forward direction. To this end, launch the command 
		> ./bin/AsconTrailTool extend forward LTS-2RTrailCoresUpToWeight20_w0+w1 40
		
	This will ouptut the file LTS-2RTrailCoresUpToWeight20_w0+w1-forwUpTo40
	
2) extend in the backward direction. To this end, launch one of the following commands:
		
		a) to extend using compatible patterns (sec. 5.3.1)
			> ./bin/AsconTrailTool extend backward LTS-2RTrailCoresUpToWeight20_w0+w1 40 false 20
		   This will ouptut the file LTS-2RTrailCoresUpToWeight20_w0+w1-backUpTo40
		
		b) to extend using the envelope space (sec 5.3.2)
			> ./bin/AsconTrailTool extend backward LTS-2RTrailCoresUpToWeight20_w0+w1 40 true
		   This will ouptut the file LTS-2RTrailCoresUpToWeight20_w0+w1-backUpTo40envelope
		   
	We recommend to use the first command when extending from 2 rounds to 3 rounds.
	We recommend to use the second command when extending from r rounds to r+1 rounds for r>2.
		
END OF EXAMPLE
**************

C) extract from a file the trail cores with weight in a given range
-------------------------------------------------------------------

OPERATION = filter
[INPUTS] = <iFileName> <oFileName> <minWeight> <maxWeight> where:
			- <iFileName> : name of the input file containing the trail cores
			- <oFileName> : name of the output file that will contain the filtered trail cores
			- <minWeight> : the minimum weight to consider
			- <maxWeight> : the maximum weight to consider

This will output a file with name <oFileName>

*******	
EXAMPLE 6

To extract from the cores of EXAMPLE 4 only those with weight in [32,36], the user can use the following command
		> ./bin/AsconTrailTool filter LTS-2RTrailCoresUpToWeight20_w0+w1-forwUpTo40 LTS-2RTrailCoresUpToWeight20_w0+w1-forwUpTo32-36 32 36
		
	This will ouptut the file LTS-2RTrailCoresUpToWeight20_w0+w1-forwUpTo32-36
		
END OF EXAMPLE
**************


D) display trail cores
---------------------------------

OPERATION = display
[INPUTS] = <iFileName> where:
			- <iFileName> : name of the input file containing the trail cores
			
This will output a file with name <iFileName>+"-display"

Each trail core contained in <iFileName> is displayed in the following format:

general description with number of rounds and weight
weigth profile through the rounds: wrev(a1) w(b1) w(b2) w(b3) etc.

*******	
EXAMPLE 7

To display the trail cores of EXAMPLE 4, the user can use the following command
		> ./bin/AsconTrailTool display cores LTS-2RTrailCoresUpToWeight20_w0+w1-forwUpTo40
		
	This will ouptut the file LTS-2RTrailCoresUpToWeight20_w0+w1-forwUpTo40-display
		
END OF EXAMPLE
**************


HOW TO REPLICATE THE SEARCH FOR 3 ROUNDS
========================================

Linear trail search: generate all 3-round linear trail cores with weight below 30 (i.e. up to 28)

1) in src/config.h file set the following:
    #define DTS 0
    static const int limitWeight = 26;
    static const int weightedWeights = 21;
2) > make -B
3) > ./bin/AsconTrailTool generate
4) > ./bin/AsconTrailTool extendWW forward 28
5) in src/config.h file set the following:
	static const int limitWeight = 28;
	static const int weightedWeights = 12;
6) > make -B
7) > ./bin/AsconTrailTool generate
8) > ./bin/AsconTrailTool extendWW backward 28

All 3-round linear trail cores with weight below 30 (i.e. up to 28) can be found in the following files:
- LTS-2RTrailCoresUpToWeight26_2w0+w1-forwUpTo28  (contains 1 trail core of weight 28)
- LTS-2RTrailCoresUpToWeight28_w0+2w1-backUpTo28 (empty)


Differential trail search: generate all 3-round differential trail cores with weight below 41 (i.e. up to 40)

1) in src/config.h file set the following:
    #define DTS 1
    static const int limitWeight = 39;
    static const int weightedWeights = 21;
2) > make -B
3) > ./bin/AsconTrailTool generate
4) > ./bin/AsconTrailTool extendWW forward 40
5) in src/config.h file set the following:
	static const int limitWeight = 40;
	static const int weightedWeights = 12;
6) > make -B
7) > ./bin/AsconTrailTool generate
8) > ./bin/AsconTrailTool extendWW backward 40

All 3-round differential trail cores with weight below 41 (i.e. up to 40) can be found in the following files:
- DTS-2RTrailCoresUpToWeight39_2w0+w1-forwUpTo40  (contains 2 trail core of weight 40)
- DTS-2RTrailCoresUpToWeight40_w0+2w1-backUpTo40 (empty)