#pragma once

#include <vector>

#include "DataTypes.h"

class WADLoader
{
public:
	WADLoader(const std::string &sWADFilePath);
	~WADLoader() {}
    bool LoadMapData(class Map *pMap);
	std::vector<uint8_t> GetLumpNamed(const std::string& name) const;
    bool LoadPatch(const std::string &sPatchName, class AssetsManager *assets);
    bool LoadTextures(const std::string &sTextureName, class AssetsManager *assets);
    

protected:
    int FindLumpByName(const std::string &LumpName, size_t start = 0) const;
    std::vector<Directory> m_WADDirectories;
    std::unique_ptr<uint8_t[]> m_pWADData;
};
