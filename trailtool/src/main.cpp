/*
AsconTrailTool, a set of C++ classes for analyzing ASCON.

For specifications, please refer to https://tosc.iacr.org/index.php/ToSC/article/view/9975

Implementation by Silvia Mella, hereby denoted as "the implementer".

DATN modifications (see trailtool/CHANGES.md):
  * trail-core extension is parallelised with OpenMP over the input trail cores; every core
    writes into its own buffer and buffers are flushed in input order, so the output file is
    identical to the one of the sequential program;
  * argument checking (the original read argv[] beyond argc and crashed);
  * "display" accepts both "display <file>" and the README form "display cores <file>";
  * new operation "trails <file> <maxWeight>" (generateTrailsInCore was not reachable);
  * timing of every operation is printed.
*/

#include <iostream>
#include <chrono>
#include <ctime>
#include <fstream>
#include <sstream>
#include <omp.h>

#include "twoRoundTrailCoreGeneration.h"
#include "treeExtension.h"
#include "genericLayer.h"
#include "generic2R.h"

using namespace std;

// Get current date/time, format is YYYY-MM-DD.HH:mm:ss
const string currentDateTime() {
    time_t     now = time(0);
    struct tm  tstruct;
    char       buf[80];
    tstruct = *localtime(&now);
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

static bool readLines(const string& iFileName, vector<string>& lines)
{
    ifstream fin(iFileName);
    if (!fin.is_open()) {
        cout << "File " << iFileName << " not found" << endl;
        return false;
    }
    string line;
    while (getline(fin, line))
        if (!line.empty()) lines.push_back(line);
    return true;
}

/* Runs job(line, out, minWeightFound) on every line in parallel and writes the outputs in input order. */
template<class Job>
static void parallelOverCores(const vector<string>& lines, ostream& fileOut, Job job)
{
    const long n = (long)lines.size();
    const long block = 4096;                  // bounded memory: flush after every block
    unsigned int globalMin = 999;
    for (long start = 0; start < n; start += block) {
        long end = (start + block < n) ? start + block : n;
        vector<string> outputs(end - start);
        #pragma omp parallel for schedule(dynamic, 1) reduction(min:globalMin)
        for (long k = start; k < end; k++) {
            ostringstream out;
            unsigned int minWeightFound = 999;
            job(lines[k], out, minWeightFound);
            outputs[k - start] = out.str();
            if (minWeightFound < globalMin) globalMin = minWeightFound;
        }
        for (auto& s : outputs) fileOut << s;
    }
    cout << n << " trails extended";
    if (globalMin != 999) cout << ", minimum weight found " << globalMin;
    cout << " (threads: " << omp_get_max_threads() << ")" << endl;
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
    string prefix = (DTS == 1) ? "DTS" : "LTS";
    if (backward) {
        iFileName = prefix + "-2RTrailCoresUpToWeight" + std::to_string(maxWeight) + "_w0+2w1";
        cout << "Extending backward " << iFileName << endl;
        fileOut = ofstream(iFileName + "-backUpTo" + std::to_string(maxWeight));
    }
    else {
        // forward: the 2-round cores must satisfy 2w0+w1 <= T-1 (differential) or <= T-2
        // (linear, all weights are even), see Section 6.1 of the paper.
        iFileName = prefix + "-2RTrailCoresUpToWeight" + std::to_string(maxWeight - ((DTS == 1) ? 1 : 2)) + "_2w0+w1";
        cout << "Extending forward " << iFileName << endl;
        fileOut = ofstream(iFileName + "-forwUpTo" + std::to_string(maxWeight));
    }
    vector<string> lines;
    if (!readLines(iFileName, lines)) return;
    parallelOverCores(lines, fileOut, [&](const string& line, ostream& out, unsigned int& minW) {
        stringstream ss(line);
        TrailCore trail(prop);
        trail.load(ss);
        if (backward)
            prop.backwardExtendTrail(out, trail, maxWeight, maxWeight - 2*trail.weights.back(), minW, true);
        else
            extendTrailForward(out, trail, maxWeight, minW, true);
    });
    fileOut.close();
}

void forwardExtendTrailCores(string& iFileName, unsigned int maxWeight)
{
    Propagation prop;
    vector<string> lines;
    if (!readLines(iFileName, lines)) return;
    ofstream fileOut(iFileName + "-forwUpTo" + std::to_string(maxWeight));
    parallelOverCores(lines, fileOut, [&](const string& line, ostream& out, unsigned int& minW) {
        stringstream ss(line);
        TrailCore trail(prop);
        trail.load(ss);
        extendTrailForward(out, trail, maxWeight, minW, false);
    });
    fileOut.close();
}

void backwardExtendTrailCores(string& iFileName, unsigned int maxWeight, bool envelope, unsigned int maxWeightStart)
{
    Propagation prop;
    vector<string> lines;
    if (!readLines(iFileName, lines)) return;
    ofstream fileOut;
    if (envelope)
        fileOut = ofstream(iFileName + "-backUpTo" + std::to_string(maxWeight) + "envelope");
    else
        fileOut = ofstream(iFileName + "-backUpTo" + std::to_string(maxWeight));
    parallelOverCores(lines, fileOut, [&](const string& line, ostream& out, unsigned int& minW) {
        TrailCore trail(prop);
        stringstream ss(line);
        trail.load(ss);
        if (envelope)
            extendTrailBackward(out, trail, maxWeight, minW, false);
        else {
            // FIX: the original computed maxWeightStart-(...) in unsigned arithmetic, which wraps
            // around (huge bound, i.e. no pruning) when the core is heavier than maxWeightStart.
            int bound = (int)maxWeightStart - (int)(trail.totalWeight - trail.minRevWeight);
            if (bound < 0) return;
            prop.backwardExtendTrail(out, trail, maxWeight, (unsigned int)bound, minW, false);
        }
    });
    fileOut.close();
}

void findTrailsInCores(unsigned int maxWeight, string& iFileName)
{
    Propagation prop;
    vector<string> lines;
    if (!readLines(iFileName, lines)) return;
    ofstream fileOut(iFileName + "-trails");
    parallelOverCores(lines, fileOut, [&](const string& line, ostream& out, unsigned int& minW) {
        (void)minW;
        stringstream ss(line);
        TrailCore trail(prop);
        trail.load(ss);
        generateTrailsInCore(out, trail, maxWeight);
    });
}

void displayTrailCores(string& iFileName){
    Propagation prop;
    vector<string> lines;
    if (!readLines(iFileName, lines)) return;
    ofstream fileOut(iFileName + "-display");
    for (auto& line : lines) {
        stringstream ss(line);
        TrailCore trail(prop);
        trail.load(ss);
        trail.display(fileOut);
        vector<unsigned int> nrSboxes = countActiveSboxesInCore(trail);
        fileOut << "active S-boxes per round: ";
        for (auto v : nrSboxes) fileOut << dec << v << " ";
        fileOut << endl << endl;
    }
}

void filterTrailCores(string& inFileName, string& outFileName, unsigned int minWeight, unsigned int maxWeight)
{
    cout << "*** Filter trail cores ***" << endl;
    unsigned int countTrails = 0;
    vector<string> lines;
    if (!readLines(inFileName, lines)) return;
    ofstream fout(outFileName);
    for (auto& line : lines) {
        unsigned int weight = getTotWeight(line);
        if (weight >= minWeight && weight <= maxWeight) {
            fout << line << endl;
            countTrails++;
        }
    }
    cout << "Number of trails kept: " << countTrails << endl;
}

static void usage()
{
    cout << "usage:\n"
         << "  generate\n"
         << "  extendWW forward|backward <maxWeight>\n"
         << "  extend forward <iFile> <maxWeight>\n"
         << "  extend backward <iFile> <maxWeight> true\n"
         << "  extend backward <iFile> <maxWeight> false <maxWeightStart>\n"
         << "  filter <iFile> <oFile> <minWeight> <maxWeight>\n"
         << "  display [cores] <iFile>\n"
         << "  trails <iFile> <maxWeight>\n";
}

int main(int argc, char** argv)
{
    // DATN: optional --layer=<spec> before the operation
    while (argc >= 2 && string(argv[1]).rfind("--layer=", 0) == 0) {
        try { g_layer = GenericLayer::fromSpec(string(argv[1]).substr(8)); }
        catch (const exception& e) { cout << "error: " << e.what() << endl; return EXIT_FAILURE; }
        argv++; argc--;
    }
    if (argc < 2) { usage(); return EXIT_FAILURE; }
    string operation = argv[1];
    auto t0 = chrono::steady_clock::now();
#if DTS == 1
    cout << "[DTS: differential trail search] ";
#else
    cout << "[LTS: linear trail search] ";
#endif
    cout << "limitWeight=" << limitWeight << " weightedWeights=" << weightedWeights
         << " layer=" << g_layer.describe() << endl;

    if (operation == "generate") {
        if (!g_layer.hardCodedAscon) {
            cout << "error: 'generate' (tree search of the paper) is specific to the Ascon linear layer;"
                 << " use 'generic2r' for other layers" << endl;
            return EXIT_FAILURE;
        }
        generate2RTrailCores();
    }
    else if (operation == "generic2r" && argc >= 4) {
        int ww = atoi(argv[2]), T = atoi(argv[3]);
        string suffix = (ww == 11) ? "_w0+w1" : (ww == 12) ? "_w0+2w1" : "_2w0+w1";
        string oFile = (argc >= 5) ? string(argv[4])
            : string((DTS == 1) ? "DTS" : "LTS") + "-2RTrailCoresUpToWeight" + to_string(T) + suffix;
        try { generateTwoRoundTrailCoresGeneric(ww, T, oFile); }
        catch (const exception& e) { cout << "error: " << e.what() << endl; return EXIT_FAILURE; }
    }
    else if (operation == "checklayer") {
        // L.Linv = Linv.L = I, LT.LinvT = I, <Lx,y> = <x,LT y>, rotation invariance, on random states
        uint64_t seed = 0x0123456789abcdefULL; int bad = 0;
        auto rnd = [&]() { seed ^= seed << 13; seed ^= seed >> 7; seed ^= seed << 17; return seed; };
        for (int t = 0; t < 20000; t++) {
            State x, y;
            for (int i = 0; i < 5; i++) { x.setRow(i, rnd()); y.setRow(i, rnd()); }
            State a = x; g_layer.applyForward(a); g_layer.applyInverse(a);
            State b = x; g_layer.applyInverse(b); g_layer.applyForward(b);
            State c = x; g_layer.applyTranspose(c); g_layer.applyInverseTranspose(c);
            State lx = x; g_layer.applyForward(lx); State lty = y; g_layer.applyTranspose(lty);
            int p1 = 0, p2 = 0;
            for (int i = 0; i < 5; i++) { p1 ^= __builtin_popcountll(lx.getRow(i) & y.getRow(i)) & 1; p2 ^= __builtin_popcountll(x.getRow(i) & lty.getRow(i)) & 1; }
            int r = (int)(rnd() & 63);
            State xr; for (int i = 0; i < 5; i++) { uint64_t v = x.getRow(i); xr.setRow(i, r ? ((v << r) | (v >> (64 - r))) : v); }
            State lxr = xr; g_layer.applyForward(lxr);
            for (int i = 0; i < 5; i++) {
                uint64_t v = lx.getRow(i);
                if (a.getRow(i) != x.getRow(i) || b.getRow(i) != x.getRow(i) || c.getRow(i) != x.getRow(i)
                    || lxr.getRow(i) != (r ? ((v << r) | (v >> (64 - r))) : v)) { bad++; break; }
            }
            if (p1 != p2) bad++;
        }
        cout << "checklayer: " << bad << " failures out of 20000 random tests" << endl;
        if (bad) return EXIT_FAILURE;
    }
    else if (operation == "extendWW" && argc >= 4) {
        string direction = argv[2];
        int maxW = atoi(argv[3]);
        if (direction == "forward") extend2RTrailCores(false, maxW);
        else if (direction == "backward") extend2RTrailCores(true, maxW);
        else { usage(); return EXIT_FAILURE; }
    }
    else if (operation == "extend" && argc >= 5) {
        string direction = argv[2];
        string iFileName = argv[3];
        int maxW = atoi(argv[4]);
        if (direction == "forward") {
            cout << "forward extension of " << iFileName << " up to " << maxW << endl;
            forwardExtendTrailCores(iFileName, maxW);
        }
        else if (direction == "backward" && argc >= 6) {
            string envelope = argv[5];
            if (envelope == "true") {
                cout << "backward extension of " << iFileName << " with envelope up to " << maxW << endl;
                backwardExtendTrailCores(iFileName, maxW, true, 0);
            }
            else if (envelope == "false" && argc >= 7) {
                int maxB = atoi(argv[6]);
                cout << "backward extension of " << iFileName << " without envelope up to " << maxW << endl;
                backwardExtendTrailCores(iFileName, maxW, false, maxB);
            }
            else { usage(); return EXIT_FAILURE; }
        }
        else { usage(); return EXIT_FAILURE; }
    }
    else if (operation == "filter" && argc >= 6) {
        string iFileName = argv[2];
        string oFileName = argv[3];
        filterTrailCores(iFileName, oFileName, atoi(argv[4]), atoi(argv[5]));
    }
    else if (operation == "display" && argc >= 3) {
        string iFileName = (argc >= 4 && string(argv[2]) == "cores") ? argv[3] : argv[2];
        displayTrailCores(iFileName);
    }
    else if (operation == "trails" && argc >= 4) {
        string iFileName = argv[2];
        findTrailsInCores(atoi(argv[3]), iFileName);
    }
    else {
        cout << "operation not known or arguments missing" << endl;
        usage();
        return EXIT_FAILURE;
    }
    chrono::duration<double> el = chrono::steady_clock::now() - t0;
    cout << "total time: " << el.count() << " s" << endl;
    return EXIT_SUCCESS;
}
