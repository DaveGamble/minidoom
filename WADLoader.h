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
	const uint8_t *GetPalette() const { return m_pWADData.get() + m_WADDirectories[FindLumpByName("PLAYPAL")].LumpOffset; }
    bool LoadPatch(const std::string &sPatchName);
    bool LoadTextures(const std::string &sTextureName);
    bool LoadPNames();
    

protected:
    int FindLumpByName(const std::string &LumpName, size_t start = 0) const;
    std::vector<Directory> m_WADDirectories;
    std::unique_ptr<uint8_t[]> m_pWADData;
};
