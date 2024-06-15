#pragma once

#include <vector>
#include "DataTypes.h"

class WADLoader
{
public:
	WADLoader(const std::string &sWADFilePath);
	~WADLoader() { delete[] data; }
    bool LoadMapData(class Map *pMap) const;
	std::vector<uint8_t> GetLumpNamed(const std::string& name, size_t after = 0) const
	{
		int id = FindLumpByName(name, after);
		return (id == -1) ? std::vector<uint8_t>() : std::vector<uint8_t>(data + dirs[id].LumpOffset, data + dirs[id].LumpOffset + dirs[id].LumpSize);
	}

protected:
    int FindLumpByName(const std::string &LumpName, size_t after = 0) const
	{
		for (size_t i = after; i < numLumps; ++i) if (!strncasecmp(dirs[i].LumpName, LumpName.c_str(), 8)) return (int)i;
		return -1;
	}

	const Directory* dirs {nullptr};
	size_t numLumps {0};
	uint8_t *data {nullptr};
};
