#pragma once

#include <vector>
#include "DataTypes.hpp"

class WADLoader
{
public:
	WADLoader(const std::string &filename)
	{
		FILE *f = fopen(filename.c_str(), "rb");
		if (!f) return;
		fseek(f, 0, SEEK_END);
		size_t length = ftell(f);
		fseek(f, 0, SEEK_SET);
		data = new uint8_t[length];
		fread(data, 1, length, f);
		fclose(f);

		numLumps = ((const uint32_t*)data)[1];
		dirs = (const Directory*)(data + ((const uint32_t*)data)[2]);
	}

	~WADLoader() { delete[] data; }
    bool LoadMapData(class Map *pMap) const;
	std::vector<uint8_t> GetLumpNamed(const std::string& name, size_t after = 0) const
	{
		int id = FindLumpByName(name, after);
		return (id == -1) ? std::vector<uint8_t>() : std::vector<uint8_t>(data + dirs[id].LumpOffset, data + dirs[id].LumpOffset + dirs[id].LumpSize);
	}
	int FindLumpByName(const std::string &LumpName, size_t after = 0) const
	{
		for (size_t i = after; i < numLumps; ++i) if (!strncasecmp(dirs[i].LumpName, LumpName.c_str(), 8)) return (int)i;
		return -1;
	}

protected:
	const Directory* dirs {nullptr};
	size_t numLumps {0};
	uint8_t *data {nullptr};
};
