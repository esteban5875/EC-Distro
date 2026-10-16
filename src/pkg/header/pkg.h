#pragma once

#include <string>
#include <vector>

struct Config {
    std::string root = "/";
    std::string repo = "/mnt/repo";
};

struct IndexEntry {
    std::string name, version, filename, sha256;   
};

struct Record {
    std::string version;
    std::vector<std::string> files;   
    std::vector<std::string> dirs;    
};

// util.cpp
std::string joinPath(const std::string& a, const std::string& b);
std::vector<std::string> splitPath(const std::string& s);
bool mkdirP(const std::string& absPath);
std::string normalizeRel(std::string n);
bool isSafeRel(const std::string& n);
bool validName(const std::string& n);
bool readFile(const std::string& path, std::vector<char>& out);

// index.cpp
bool loadIndex(const Config& cfg, std::vector<IndexEntry>& out);
const IndexEntry* findEntry(const std::vector<IndexEntry>& idx, const std::string& name);
bool fetchPackage(const Config& cfg, const IndexEntry& e, std::string& localPath);

// db.cpp
bool isInstalled(const Config& cfg, const std::string& name);
bool saveRecord(const Config& cfg, const std::string& name, const Record& rec);
bool loadRecord(const Config& cfg, const std::string& name, Record& rec);
void removeRecordFiles(const Config& cfg, const Record& rec);
bool deleteRecord(const Config& cfg, const std::string& name);

// tar.cpp
bool extractTar(const Config& cfg, const std::vector<char>& data, Record& rec);