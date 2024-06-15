#pragma once

#include <cstdint> 
#include <fstream>
#include <string>
#include <vector>

#include "DataTypes.h"
#include "Map.h"
#include "DisplayManager.h"

class WADLoader
{
public:
	WADLoader() {}
	~WADLoader() {}
	void SetWADFilePath(const std::string &sWADFilePath) {  m_sWADFilePath = sWADFilePath; }
	bool LoadWADToMemory() { return OpenAndLoad() && ReadDirectories(); }
    bool LoadMapData(Map *pMap);
    bool LoadPalette(DisplayManager *pDisplayManager);
    bool LoadPatch(const std::string &sPatchName);
    bool LoadTextures(const std::string &sTextureName);
    bool LoadPNames();
    

protected:
	struct lump {const uint8_t *ptr {nullptr}; size_t size {0};};
    bool OpenAndLoad();
    bool ReadDirectories();
    bool ReadMapVertexes(Map *pMap);
    bool ReadMapLinedefs(Map *pMap);
    bool ReadMapThings(Map *pMap);
    bool ReadMapNodes(Map *pMap);
    bool ReadMapSubsectors(Map *pMap);
    bool ReadMapSectors(Map *pMap);
    bool ReadMapSidedefs(Map *pMap);
    bool ReadMapSegs(Map *pMap);

    int FindMapIndex(Map *pMap);
	lump FindMapLump(Map *pMap, const std::string& lumpName);
    int FindLumpByName(const std::string &LumpName, size_t start = 0);

    std::string m_sWADFilePath;
    std::ifstream m_WADFile;
    std::vector<Directory> m_WADDirectories;
    std::unique_ptr<uint8_t[]> m_pWADData;
};
