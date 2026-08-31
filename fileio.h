#pragma once

#include <vector>
#include <string>

extern std::vector<std::string> fileList;
extern std::string currentLoadedFile;
extern std::string currentDirectory;
extern std::vector<std::string> foundSequenceFiles;
extern int seqStartIndex;
extern int seqEndIndex;

void LoadFiles();
void LoadSettings();
void SaveSettings();
void ScanDirectoryForSequence(std::string folderPath, std::string referenceFile);

