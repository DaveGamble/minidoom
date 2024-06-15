#pragma once

#include <vector>
#include "DataTypes.h"

class WADLoader
{
public:
	WADLoader(const std::string &sWADFilePath);
	~WADLoader() {}
    bool LoadMapData(class Map *pMap) const;
	std::vector<uint8_t> GetLumpNamed(const std::string& name) const;

protected:
    int FindLumpByName(const std::string &LumpName, size_t start = 0) const;
	const Directory* m_WADDirectories {nullptr};
	size_t numLumps {0};
    std::unique_ptr<uint8_t[]> m_pWADData;
};
