// CdrToQRouter.cpp : This file contains the 'main' function. Program execution begins and ends there.
#include <cstring>
#include <algorithm>
#include "crc-hash.h"
//#define ZLIB_WINAPI
#include "zconf.h"
#include "zlib.h"

#include "CdrToQRouterLib.h"

// Parsing info and setting
constexpr auto IMSI_FIELD_POS = 2;
constexpr auto SKIP_FIRST_LINE = true;
constexpr auto REPLACE_EOL_CHAR = false;
constexpr auto COMPRESSED_FILE_SUFF = ".cdrgz";
constexpr auto TXT_FILE_SUFF = ".txt";

using namespace std;
namespace fs = std::filesystem;

//=================================================================================
bool createDirIfNotExist(string outDir) {
    if (fs::exists(outDir)) {
        std::cout << "Directory already exists." << std::endl;
        return true;
    }
    else {
        if (fs::create_directory(outDir)) {
            return true;
        }
    }

    return false;
}


uint64_t get_hash(char* imsi_ascii)
{
    uint64_t imsi = atoll(imsi_ascii);
    uint64_t hash = CrCRC32_Calc((byte*)&imsi, 8, 0);
    hash <<= 8;
    return hash;
}


size_t find_folder_separator(string filePath) {
    // Find the position of the last path separator ("/" or "\") to separate the folder and file names
    size_t lastSeparatorPos = filePath.find_last_of("/\\");
    if (lastSeparatorPos == std::string::npos) {
        std::cerr << "Invalid file path." << std::endl;
        return 1;
    }
    return lastSeparatorPos;

}

bool get_imsi_str(const char* source, int field_pos, char* dest_imsi, size_t max_len) {
    // Get imsi field from line and shrink line to the remain
     // Find the IMSI field
    const char* foundChar = source; // Starting point
    for (int k = 0; k < field_pos; k++) {
        foundChar = std::strchr(foundChar, ',');
        foundChar++;
    }

    std::copy_n(foundChar, max_len, dest_imsi);
    //strncpy_s(imsi_str, foundChar, 64);
    char* foundEnd = std::strchr(dest_imsi, ',');
    if (foundEnd - dest_imsi > 0)
        *foundEnd = 0; // End of string

    // COMMENT:
    //The terminating null - character is considered part of the C string.Therefore, it can also be located in order to retrieve a pointer to the end of a string.
    return true;
}


bool processCsv2QRouter(string filePath, string outputDir, const int max_lines) {

    bool DEBUG = false;

    // Open the file using fopen_s
    FILE* inputFile;
    if (fopen_s(&inputFile, filePath.c_str(), "r") != 0) {
        std::fprintf(stderr, "Error opening the file.\n");
        return false;
    }

    // Extract the folder and file names
    size_t separator_pos = find_folder_separator(filePath);
    std::string folderName = filePath.substr(0, separator_pos);
    std::string fileName = filePath.substr(separator_pos + 1);
    size_t pos = fileName.rfind(".csv");
    fileName.replace(pos, 4, "");   // File name without the suffix

    int i = 0;
    int i_part = 0;
    char line[MAX_LINE_SIZE];
    char* buffer = new char[MAX_BUFFER_SIZE];
    int i_offset = 0;

    unsigned long compressedSizeBound = MAX_BUFFER_SIZE * 2;
    char* compressedBuffer = new char[compressedSizeBound];
    unsigned long compressed_size;


    uint32_t slng, llng;
    uint64_t hash;
    string outFilePath;
    string outFilePathTxt;
    char num_str[64];

    // If there is a header, skip it
    if (SKIP_FIRST_LINE)
        fgets(line, sizeof(line), inputFile);

    // Read the file line by line using fgets
    while (fgets(line, sizeof(line) - 1, inputFile) && (i < max_lines)) {
        i++;
        slng = (uint32_t)strlen(line);
        if (DEBUG)
            cout << i << " " << line;

        // Remove the newline character at the end of the line, if present (suitable for csv format)
        if (REPLACE_EOL_CHAR) {
            if (slng > 0 && line[slng - 1] == '\n') {
                line[slng - 1] = '\0'; // Terninate the string
            }
        }

        // Get imsi field from line and shrink line to the remain
        char imsi_str[64];
        get_imsi_str(line, IMSI_FIELD_POS, imsi_str, 64);

        // Get the hash
        hash = get_hash(imsi_str);

        // Calc line length
        llng = 1 + 4 + slng;

        if (i_offset + sizeof(llng) + 5 + slng + sizeof(llng) >= MAX_BUFFER_SIZE) {
            // Reached buffer limit
            sprintf_s(num_str, "%d", i_part);
            string numberString = num_str;
            outFilePath = outputDir + "/" + fileName + "-part_" + numberString + COMPRESSED_FILE_SUFF;
            outFilePathTxt = outputDir + "/" + fileName + "-part_" + numberString + TXT_FILE_SUFF;

            // Debug
            if (DEBUG)
                memoryCdrsToCsv(buffer, i_offset, outFilePathTxt, 50);

            /*
            compress and save to file collected(CDRs mem)
            */
            compressed_size = compressBound((uLong)i_offset);
            if (compressed_size > compressedSizeBound) {
                std::fprintf(stderr, "CompressBound larger than allocated complressed buffer.\n");
                return false;
            }

            int iReturn = compress((Bytef*)compressedBuffer, &compressed_size, (Bytef*)buffer, i_offset);
            writeCompressedToFile(compressedBuffer, compressed_size, outFilePath);


            i_part++;
            i_offset = 0;
        }

        std::memcpy(&buffer[i_offset], &llng, sizeof(llng)); i_offset += sizeof(llng);  // 4 byts
        std::memcpy(&buffer[i_offset], &hash, 5);            i_offset += 5;             // 5 bytes
        std::memcpy(&buffer[i_offset], &line, slng);         i_offset += slng;          // line of chars
        std::memcpy(&buffer[i_offset], &llng, sizeof(llng)); i_offset += sizeof(llng);  // 4 bytes
    }

    if (i_offset > 0) {
        // Save the remaining part
        sprintf_s(num_str, "%d", i_part);
        string numberString = num_str;
        outFilePath = outputDir + "/" + fileName + "-part_" + numberString + COMPRESSED_FILE_SUFF;
        outFilePathTxt = outputDir + "/" + fileName + "-part_" + numberString + TXT_FILE_SUFF;

        //Debug
        if (DEBUG)
            memoryCdrsToCsv(buffer, i_offset, outFilePathTxt, 50);

        /*
         compress and save to file collected(CDRs mem)
         */
        compressed_size = compressBound((uLong)i_offset);
        if (compressed_size > compressedSizeBound) {
            std::fprintf(stderr, "CompressBound larger than allocated complressed buffer.\n");
            return false;
        }

        int iReturn = compress((Bytef*)compressedBuffer, &compressed_size, (Bytef*)buffer, i_offset);
        writeCompressedToFile(compressedBuffer, compressed_size, outFilePath);

        // DEBUG
        /*
        unsigned char buff2[210000];
        uLongf len;
        int ireturn = uncompress((Bytef*)buff2, &len, (Bytef*)compressedBuffer, (uLong)compressed_size);
        cout << ireturn;
        */
    }

    fclose(inputFile);
    delete[] compressedBuffer;
    delete[] buffer;

    return true;
}


bool memoryCdrsToCsv(const char* buffer, int bufferlen, string outfpath, const int write_n_lines) {
    FILE* outFile;
    if (fopen_s(&outFile, outfpath.c_str(), "w") != 0) {
        fprintf(stderr, "Error opening the file.\n");
        return 1;
    }
    //Write to file untill index equals the length:
    char line_string[MAX_LINE_SIZE];
    int index = 0;
    uint32_t llng1, llng2;
    const char* hash2;
    const char* line2;
    int n = 0;
    while (index < bufferlen) {
        n++;
        std::memcpy(&llng1, &buffer[index], sizeof(uint32_t));  index += sizeof(uint32_t);  // 4 byts
        hash2 = &buffer[index];                                 index += 5;             // 5 bytes
        line2 = &buffer[index];                                 index += (llng1 - 4 - 1);          // line of chars
        std::memcpy(&llng2, &buffer[index], sizeof(uint32_t));  index += sizeof(uint32_t);  // 4 byts

        // Handle the data line for printing
        int line_length = (llng1 - 4 - 1);
        std::memcpy(line_string, line2, line_length);
        line_string[line_length] = '\0'; // Have a proper string termination for printing
        std::printf("%d %s", n, line_string);
        fprintf(outFile, "%s", line_string);
        if (n >= write_n_lines)
            break;
    }

    fclose(outFile);
    return true;
}


bool writeCompressedToFile(const char* compressedBuffer, unsigned long compressed_bound, string outFilePath) {
    FILE* fp;
    if (fopen_s(&fp, outFilePath.c_str(), "wb") == 0) {

        uint32_t leng = 1 + 4 + compressed_bound;
        fwrite(&leng, 4, 1, fp);

        byte qpid = (byte)8;
        fwrite(&qpid, 1, 1, fp);

        uint32_t num = 0; // (shall be hash32 of the IMSI, but now it may be 0)
        fwrite(&num, 1, 4, fp);

        fwrite(compressedBuffer, 1, compressed_bound, fp);
        fwrite(&leng, 1, 4, fp);

        fclose(fp);
        return true;
    }
    return false;
}

bool compressedFile2memory(string inputCompressedFile, char* buffer, unsigned long* bufferlen) {
    FILE* infp;

    if (fopen_s(&infp, inputCompressedFile.c_str(), "rb") != 0) {
        std::fprintf(stderr, "Error opening file for reading.\n");
        return false;
    }


    // Read the compressed data according to the write format
    uint32_t leng;
    fread(&leng, 4, 1, infp);

    byte qpid;
    fread(&qpid, 1, 1, infp);

    uint32_t num; // (shall be hash32 of the IMSI, but now it may be 0)
    fread(&num, 4, 1, infp);

    unsigned long compressed_size = leng - 4 - 1;
    unsigned char* compressedBuffer = new unsigned char[compressed_size];
    fread(compressedBuffer, 1, compressed_size, infp);

    int leng2;
    fread(&leng2, 4, 1, infp);
    fclose(infp);

    // Validity check
    if (leng2 != leng) {
        std::fprintf(stderr, "leng2 not equal to leng!");
        return false;
    }

    // Uncompress the data into the output buffer
    uLongf len;
    int ireturn = uncompress((Bytef*)buffer, &len, (Bytef*)compressedBuffer, (uLong)compressed_size);
    *bufferlen = (unsigned long)len;

    delete[]compressedBuffer;
    if (ireturn == 0)
        return true;

    return false;
}