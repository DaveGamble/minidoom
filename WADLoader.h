#pragma once

#include <cstdint> 
#include <fstream>
#include <string>
#include <vector>

#include "DataTypes.h"

class WADLoader
{
public:
	WADLoader(const std::string &sWADFilePath);
	~WADLoader() {}
    bool LoadMapData(class Map *pMap);
    bool LoadPalette(class DisplayManager *pDisplayManager);
    bool LoadPatch(const std::string &sPatchName);
    bool LoadTextures(const std::string &sTextureName);
    bool LoadPNames();
    

protected:
    int FindLumpByName(const std::string &LumpName, size_t start = 0);
    std::vector<Directory> m_WADDirectories;
    std::unique_ptr<uint8_t[]> m_pWADData;
};
