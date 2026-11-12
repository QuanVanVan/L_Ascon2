/*
AsconTrailTool, a set of C++ classes for analyzing ASCON.

For specifications, please refer to https://tosc.iacr.org/index.php/ToSC/article/view/9975

Implementation by Silvia Mella, hereby denoted as "the implementer".
*/

#include <iostream>
#include <chrono>
#include <ctime>
#include <fstream>

#include <execution>
#include <omp.h>

#include "twoRoundTrailCoreGeneration.h"
#include "treeExtension.h"

using namespace std;

// Get current date/time, format is YYYY-MM-DD.HH:mm:ss
const string currentDateTime() {
    time_t     now = time(0);
    struct tm  tstruct;
    char       buf[80];
    tstruct = *localtime(&now);
    // Visit http://en.cppreference.com/w/cpp/chrono/c/strftime
    // for more information about date/time format
    strftime(buf, sizeof(buf), "%Y-%m-%d.%X", &tstruct);

    return buf;
}

unsigned int getTotWeight(string& line)
{
    stringstream ss(line);
    unsigned int info;
    ss >> dec >> info;
    ss >> dec >> info;
    return info;
}

void generate2RTrailCores()
{
    string currentDandT = currentDateTime();
    cout << "currentDateTime()=" << currentDandT << endl;
    FILE* fptr = fopen(FILE_NAME_COL, "a");
    fprintf(fptr, "Start at: %s", currentDandT.c_str());
    fprintf(fptr, "\n");
    fclose(fptr);

    auto start = chrono::system_clock::now();
    generateTwoRoundTrailCores();
    auto end = chrono::system_clock::now();

    chrono::duration<double> elapsed_seconds = end - start;
    time_t end_time = chrono::system_clock::to_time_t(end);

    fptr = fopen(FILE_NAME_COL, "a");
    fprintf(fptr, "finished computation at %s", ctime(&end_time));
    fprintf(fptr, "\n");
    fprintf(fptr, "elapsed time: %f s", elapsed_seconds.count());
    fprintf(fptr, ",\n\n");
    fclose(fptr);

    cout << "finished computation at " << ctime(&end_time)
        << "elapsed time: " << elapsed_seconds.count() << "s\n";

}

void extend2RTrailCores(bool backward, unsigned int maxWeight)
{
    Propagation prop;
    string iFileName;
	ofstream fileOut;
    if (backward) {
		#if DTS == 1
			iFileName = "DTS-2RTrailCoresUpToWeight" + std::to_string(maxWeight) + "_w0+2w1";
		#else
			iFileName = "LTS-2RTrailCoresUpToWeight" + std::to_string(maxWeight) + "_w0+2w1";
		#endif
        cout << "Extending backward " << iFileName << endl;
	    fileOut = ofstream(iFileName + "-backUpTo" + std::to_string(maxWeight));
    }
    else {
		#if DTS == 1
			iFileName = "DTS-2RTrailCoresUpToWeight" + std::to_string(maxWeight-1) + "_2w0+w1";
		#else
			iFileName = "LTS-2RTrailCoresUpToWeight" + std::to_string(maxWeight-2) + "_2w0+w1";
		#endif
        cout << "Extending forward " << iFileName << endl;
		fileOut = ofstream(iFileName + "-forwUpTo" + std::to_string(maxWeight));
    }
    ifstream fin(iFileName);
    if (!fin.is_open()) {
        cout << "File not found" << endl;
        return;
    }
	
	string line;
	int countLines = 0;
	while (getline(fin, line)) { 
		stringstream ss(line);
		TrailCore trail(prop);
		trail.load(ss);
		if (backward) {
			unsigned int minWeightFound = 999;
			prop.backwardExtendTrail(fileOut, trail, maxWeight, maxWeight-2*trail.weights.back(), minWeightFound, true);
		}
		else {
			unsigned int minWeightFound = 999;
			extendTrailForward(fileOut, trail, maxWeight, minWeightFound, true);
		}
		countLines++;					 
    }
	cout << countLines << " trails extended" << endl;
	fin.close();
	fileOut.close();
}

void forwardExtendTrailCores(string& iFileName, unsigned int maxWeight)
{
    Propagation prop;
    ifstream fin(iFileName);
    if (!fin.is_open()) {
        cout << "File not found" << endl;
        return;
    }
	
    ofstream fileOut(iFileName + "-forwUpTo" + std::to_string(maxWeight));
	
	string line;
	int countLines = 0;
   
	while (getline(fin, line)) { 
		stringstream ss(line);
		TrailCore trail(prop);
		trail.load(ss);
		unsigned int minWeightFound = 999;
		extendTrailForward(fileOut, trail, maxWeight, minWeightFound, false);
		countLines++;
	}
	cout << countLines << " trails extended" << endl;
	fin.close();
    fileOut.close();
}

void backwardExtendTrailCores(string& iFileName, unsigned int maxWeight, bool envelope, unsigned int maxWeightStart)
{
    Propagation prop;
    ifstream fin(iFileName);
    if (!fin.is_open()) {
        cout << "File not found" << endl;
        return;
    }

    ofstream fileOut;
	if (envelope)
		fileOut = ofstream(iFileName + "-backUpTo" + std::to_string(maxWeight)+"envelope");
	else
		fileOut = ofstream(iFileName + "-backUpTo" + std::to_string(maxWeight));
	
	string line;
	int countLines = 0;
	while (getline(fin, line)) { 
		TrailCore trail(prop);
		stringstream ss(line);
		trail.load(ss);
		unsigned int minWeightFound = 999;
		if (envelope)
			extendTrailBackward(fileOut, trail, maxWeight, minWeightFound, false);
		else
			prop.backwardExtendTrail(fileOut, trail, maxWeight, maxWeightStart-(trail.totalWeight - trail.minRevWeight), minWeightFound, false);
		
		countLines++;
	}    
	cout << countLines << " trails extended" << endl;
	
	fin.close();
	fileOut.close();
}

void findTrailsInCores(unsigned int maxWeight, string& iFileName)
{
	Propagation prop;
	
	ifstream fin(iFileName);
	if (!fin.is_open()) {
		cout << "File " << iFileName << " not found" << endl;
		return;
	}
	ofstream fileOut(iFileName+"-trails");
	 
	string line;
	int countLines = 1;
	while (getline(fin, line))
	{
		stringstream ss(line);
		TrailCore trail(prop);
		trail.load(ss);
		generateTrailsInCore(fileOut, trail, maxWeight);
		countLines++;
	}
}
 
void displayTrailCores(string& iFileName){
    
  Propagation prop;
  ifstream fin(iFileName);
  ofstream fileOut(iFileName+"-display");
  string line;
  
  while (getline(fin, line)) {
        stringstream ss(line);
        TrailCore trail(prop);
        trail.load(ss);
        trail.display(fileOut);        
  }
}

void filterTrailCores(string& inFileName, string& outFileName, unsigned int minWeight, unsigned int maxWeight)
{
	cout << "*** Filter trail cores ***" << endl;
	unsigned int countTrails = 0;
	
	ifstream fin(inFileName);
    if (!fin.is_open()) {
        cout << "File not found" << endl;
        return;
    }
	else {
		ofstream fout(outFileName);
		string line;
		while (getline(fin, line)) {
			unsigned int weight = getTotWeight(line);
			if(weight >= minWeight && weight <= maxWeight){
				fout << line << endl;
				countTrails++;
			}
		}
		cout << "Number of trails kept: " << countTrails << endl;
	}
}

int main(int argc, char** argv) 
{
	// generate 2R (no options, this reads the config.h file)
	// extendWW direction maxWeight (at this moment this reads the file of 2r cores)
	// extend forward inputFile maxWeight 
	// extend backward inputFile maxWeight envelope (maxWeightAtB)
	// filter iFileName oFileName minWeight maxWeight (extract trails)
	// findTrails iFileName maxWeight (find trails in trail cores)
	
	if(argc < 2){
		cout << "one or more arguments are missing" << endl;
		return EXIT_SUCCESS;
	}
	
	string operation = argv[1];	
	
	if(operation == "generate"){	
		generate2RTrailCores();
	}
	else if(operation == "extendWW"){
		string direction = argv[2];
		int maxW = atoi(argv[3]);
		if(direction == "forward")
			extend2RTrailCores(false, maxW);
		else
			extend2RTrailCores(true, maxW);
	}
	else if(operation == "extend"){	
		string direction = argv[2];	
		string iFileName = argv[3];	
		int maxW = atoi(argv[4]);	
		
		if(direction == "forward"){
			cout << "forward extension of " << iFileName << " up to " << maxW << endl;
			forwardExtendTrailCores(iFileName, maxW);
		}
		else if(direction == "backward"){
				string envelope = argv[5];
				if(envelope == "true"){
					cout << "backward extension of " << iFileName << " with envelope up to " << maxW << endl;
					backwardExtendTrailCores(iFileName, maxW, true, 42);
				}
				if(envelope == "false"){
					int maxB = atoi(argv[6]);	
					cout << "backward extension of " << iFileName << " without envelope up to " << maxW << endl;
					backwardExtendTrailCores(iFileName, maxW, false, maxB);
				}
		}
		else
			cout << "extension direction is wrongly specified" << endl;
	}
	else if(operation == "filter"){	
		string iFileName = argv[2];	
		string oFileName = argv[3];	
		int minW = atoi(argv[4]);	
		int maxW = atoi(argv[5]);	
		filterTrailCores(iFileName,oFileName,minW,maxW);
	}
	else if(operation == "display"){
		string iFileName = argv[2];	
		displayTrailCores(iFileName);
	}
	else{
		cout << "operation not known" << endl;
	}
	return EXIT_SUCCESS;
}