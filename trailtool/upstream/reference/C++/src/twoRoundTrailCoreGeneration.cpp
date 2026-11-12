/*
AsconTrailTool, a set of C++ classes for analyzing ASCON.

For specifications, please refer to https://tosc.iacr.org/index.php/ToSC/article/view/9975

Implementation by Solane El Hirch, hereby denoted as "the implementer".
*/

#include <iostream>
#include <fstream>
#include <cstdint>

#include "twoRoundTrailCoreGeneration.h"
#include "config.h"
#include "histogram.h"

int generateTwoRoundTrailCores(){
  int weightAtB, nrColB;
  long count = 0;
  long countTrails = 0;
  StateAsUnits state;
  Histogram histogram;
  
  string ofilename;
  
#if DTS == 1
  if (weightedWeights == 11)
    ofilename = "DTS-2RTrailCoresUpToWeight"+std::to_string(limitWeight)+"_w0+w1";
  else if (weightedWeights == 12)
    ofilename = "DTS-2RTrailCoresUpToWeight"+std::to_string(limitWeight)+"_w0+2w1";
  else if (weightedWeights == 21)
    ofilename = "DTS-2RTrailCoresUpToWeight"+std::to_string(limitWeight)+"_2w0+w1";  
#else
  if (weightedWeights == 11)
    ofilename = "LTS-2RTrailCoresUpToWeight"+std::to_string(limitWeight)+"_w0+w1";
  else if (weightedWeights == 12)
    ofilename = "LTS-2RTrailCoresUpToWeight"+std::to_string(limitWeight)+"_w0+2w1";
  else if (weightedWeights == 21)
    ofilename = "LTS-2RTrailCoresUpToWeight"+std::to_string(limitWeight)+"_2w0+w1";  
#endif

  ofstream fout(ofilename);
  
  do{
    nrColB = state.getNrActColAtB(); // nr of active col
    weightAtB = state.getWeightAtB(); // get real weight
    if(state.getTotalActCol(nrColB)){ // return real weight
      histogram.setValueInHistogramActCol(state.getNrOfActColAtA(), nrColB); 

      if(state.getTotalWeight(weightAtB)){
        histogram.setValueInHistogramTrailCores(state.getReverseWeight(), weightAtB); 
        fout << dec << 2 << " ";
        fout << state.getReverseWeight()+weightAtB << " " << state.getReverseWeight() << " " << weightAtB << " ";
        fout << hex << state.stateAtB.getRow(0) << " " << state.stateAtB.getRow(1) << " " << state.stateAtB.getRow(2) << " " << state.stateAtB.getRow(3) << " " << state.stateAtB.getRow(4) << endl;
        countTrails++;
      }
      
      count++;
    }
  }while(state.nextState());
  histogram.getHistogram(FILE_NAME_COL, FILE_DESCRIPTION_NRCOL, IN_VAL_COL, OUT_VAL_COL, limit );
  histogram.getHistogram(FILE_NAME_WEIGHT, FILE_DESCRIPTION_WEIGHT, IN_VAL_CORES, OUT_VAL_CORES, limitWeight);
  cout << "number of states: " << count << endl;
  cout << "number of trails: " << countTrails << endl;
  return 1;
}
