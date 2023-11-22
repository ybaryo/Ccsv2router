// CdrToQRouter.cpp : This file contains the 'main' function. Program execution begins and ends there.
#include <cstdio>
#include <iostream>
#include <filesystem> // For filesystem operations
using namespace std;

#include "CdrToQRouterLib.h"

constexpr bool debug_compression = true;
//==============================================================================
int main(int argc, char* argv[])
{
    //Check input arguments
    //Debug
    cout << "argc = " << argc << endl;
    for (int i = 0; i < argc; i++) {
        cout << "arg " << i << ": " << argv[i] << endl;
    }

    //Test
    if (argc < 3) {
        cout << "Usage: CdrToRouter.exe <input_file> <output_dir>" << endl;
        return -1;
    }
 
    // Running setting
    //string filePath = "C:/DATA/RADCOM/mrs/location_results_debug/part_0.csv";
    //string outDir = "C:/DATA/RADCOM/mrs/location_results_debug_2";
    string filePath = argv[1];
    string outDir = argv[2];

    
    //Output dir
    createDirIfNotExist(outDir);

    //processCsv2QRouter(filePath, outDir, DEFAULT_MAX_LINES);
    processCsv2QRouter(filePath, outDir);

    // Debug: restore the csv data from a compressed file
    if (debug_compression) {
        string inputCompressedFile = "C:/DATA/RADCOM/mrs/location_results_debug/part_0-part_0.cdrgz";
        string outCsvFile = "C:/DATA/RADCOM/mrs/location_results_debug/test.csv";
        unsigned long bufferlen = 0; // (Empty)
        char* buffer = new char[MAX_BUFFER_SIZE * 2]; //
        compressedFile2memory(inputCompressedFile, buffer, &bufferlen);
        memoryCdrsToCsv(buffer, bufferlen, outCsvFile, 150);
    }

    return 0;
}
