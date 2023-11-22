#pragma once

#include <cstdio>
#include <iostream>
#include <filesystem> // For filesystem operations

// Buffer setting
#define MAX_BUFFER_SIZE (int)200000 // limited up to 200K

// Processing setting
#define MAX_LINE_SIZE (int)10000
#define DEFAULT_MAX_LINES  (int)1000000000

size_t find_folder_separator(string filePath);
bool get_imsi_str(const char* source, int field_pos, char* dest_imsi, size_t max_len);
bool processCsv2QRouter(string inputPath, string outputDir, const int max_lines = DEFAULT_MAX_LINES);
bool memoryCdrsToCsv(const char* buffer, int bufferlen, string outfpath, const int write_n_lines = 100);
bool writeCompressedToFile(const char* compressedBuffer, unsigned long compressed_bound, string outFilePath);
bool compressedFile2memory(string inputCompressedFile, char* buffer, unsigned long* bufferlen);
bool createDirIfNotExist(string outDir);
uint64_t get_hash(char* imsi_ascii);
