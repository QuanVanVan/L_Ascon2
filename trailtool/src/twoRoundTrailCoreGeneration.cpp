/*
AsconTrailTool, a set of C++ classes for analyzing ASCON.

For specifications, please refer to https://tosc.iacr.org/index.php/ToSC/article/view/9975

Implementation by Solane El Hirch, hereby denoted as "the implementer".

DATN modification: the tree of canonical 2-round trail cores is split into the 5 independent
sub-trees "first active row = y0" (y0 = 0..4) that are traversed in parallel with OpenMP.
Each sub-tree writes to its own temporary file; the files are concatenated in the order
y0 = 0..4, which is exactly the order of the original sequential traversal, so the output
file is identical (byte for byte) to the single-threaded one.
*/

#include <iostream>
#include <fstream>
#include <sstream>
#include <cstdint>
#include <cstdio>
#include <omp.h>

#include "twoRoundTrailCoreGeneration.h"
#include "config.h"
#include "histogram.h"

static string outputFileName(){
  string prefix = (DTS == 1) ? "DTS" : "LTS";
  string suffix;
  if (weightedWeights == 11) suffix = "_w0+w1";
  else if (weightedWeights == 12) suffix = "_w0+2w1";
  else if (weightedWeights == 21) suffix = "_2w0+w1";
  return prefix + "-2RTrailCoresUpToWeight" + std::to_string(limitWeight) + suffix;
}

static void traverseSubTree(int firstRow, int secondUnit, ostream& fout, Histogram& histogram, long& count, long& countTrails){
  int weightAtB, nrColB;
  StateAsUnits state(firstRow, secondUnit);
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
}

int generateTwoRoundTrailCores(){
  string ofilename = outputFileName();
  // sub-trees in the order of the sequential traversal: for every first active row y0,
  // first the states whose first row is {0}, then those whose first row starts with {0, j}
  vector<pair<int,int>> parts;
  for (int y0 = 0; y0 < nrRows; y0++)
    for (int j = 0; j <= nrCols/2; j++)
      parts.push_back(make_pair(y0, j));
  const int nParts = (int)parts.size();
  vector<long> count(nParts, 0), countTrails(nParts, 0);
  vector<Histogram> histograms(nParts);
  vector<double> seconds(nParts, 0.0);

  #pragma omp parallel for schedule(dynamic, 1)
  for (int p = 0; p < nParts; p++) {
    double t0 = omp_get_wtime();
    ofstream part(ofilename + ".part" + std::to_string(p));
    traverseSubTree(parts[p].first, parts[p].second, part, histograms[p], count[p], countTrails[p]);
    part.close();
    seconds[p] = omp_get_wtime() - t0;
  }

  // merge in the sequential order
  ofstream fout(ofilename);
  Histogram histogram;
  long totalCount = 0, totalTrails = 0;
  double maxSeconds = 0, sumSeconds = 0;
  for (int p = 0; p < nParts; p++) {
    string partName = ofilename + ".part" + std::to_string(p);
    {
      ifstream part(partName);
      if (part.peek() != ifstream::traits_type::eof()) fout << part.rdbuf();
    }
    std::remove(partName.c_str());
    for (size_t k = 0; k < histogram.histogramOfActCol.size(); k++) histogram.histogramOfActCol[k] += histograms[p].histogramOfActCol[k];
    for (size_t k = 0; k < histogram.histogramOfTrailCores.size(); k++) histogram.histogramOfTrailCores[k] += histograms[p].histogramOfTrailCores[k];
    totalCount += count[p];
    totalTrails += countTrails[p];
    sumSeconds += seconds[p];
    if (seconds[p] > maxSeconds) maxSeconds = seconds[p];
  }
  fout.close();
  histogram.getHistogram(FILE_NAME_COL, FILE_DESCRIPTION_NRCOL, IN_VAL_COL, OUT_VAL_COL, limit, true);
  histogram.getHistogram(FILE_NAME_WEIGHT, FILE_DESCRIPTION_WEIGHT, IN_VAL_CORES, OUT_VAL_CORES, limitWeight, false);
  cout << "number of states: " << totalCount << endl;
  cout << "number of trails: " << totalTrails << endl;
  cout << "threads: " << omp_get_max_threads() << ", sub-trees: " << nParts
       << ", CPU time " << sumSeconds << " s, largest sub-tree " << maxSeconds << " s" << endl;
  return 1;
}
