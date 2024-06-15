#pragma once

#include <vector>
#include "DataTypes.h"

class WADLoader
{
public:
	WADLoader(const std::string &sWADFilePath);
	~WADLoader() { delete[] data; }
    bool LoadMapData(class Map *pMap) const;
	std::vector<uint8_t> GetLumpNamed(const std::string& name, size_t after = 0) const;

protected:
    int FindLumpByName(const std::string &LumpName, size_t after = 0) const;
	const Directory* dirs {nullptr};
	size_t numLumps {0};
	uint8_t *data {nullptr};
};
