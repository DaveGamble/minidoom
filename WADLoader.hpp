#pragma once

#include <map>
#include "DataTypes.hpp"
#include "Texture.hpp"
#include "Flat.hpp"

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

		std::vector<uint8_t> lump = getLumpNamed("PNAMES");
		
		int32_t count = *(int32_t*)lump.data();
		char Name[9] {};
		for (int i = 0; i < count; ++i)
		{
			memcpy(Name, lump.data() + 4 + 8 * i, 8);
			pnames.push_back(Name);
		}
		
		const char *toload[2] = {"TEXTURE1", "TEXTURE2"};
		for (int i = 0; i < 2; i++)
		{
			std::vector<uint8_t> data = getLumpNamed(toload[i]);
			if (!data.size()) continue;
			
			const int32_t *asint = (const int32_t*)data.data();
			int32_t numTextures = asint[0];
			for (int i = 0; i < numTextures; ++i)
			{
				char name[9] {}; memcpy(name, data.data() + asint[i + 1], 8);
				textures[name] = std::unique_ptr<Texture>(new Texture(data.data() + asint[i + 1], [&](int idx) { return getPatch(pnames[idx]); }));
			}
		}
		
		for (int flat = findLumpByName("F_START") + 1; strncasecmp(dirs[flat].lumpName, "F_END", 8); flat++)
		{
			if (dirs[flat].lumpSize != 4096) continue;
			char name[9] {}; memcpy(name, dirs[flat].lumpName, 8);
			flats[name] = std::unique_ptr<Flat>(new Flat(data + dirs[flat].lumpOffset));
	   }
	}

	~WADLoader() { delete[] data; }

	std::vector<uint8_t> getLumpNamed(const std::string& name, size_t after = 0) const
	{
		int id = findLumpByName(name, after);
		return (id == -1) ? std::vector<uint8_t>() : std::vector<uint8_t>(data + dirs[id].lumpOffset, data + dirs[id].lumpOffset + dirs[id].lumpSize);
	}
	int findLumpByName(const std::string &LumpName, size_t after = 0) const
	{
		for (size_t i = after; i < numLumps; ++i) if (!strncasecmp(dirs[i].lumpName, LumpName.c_str(), 8)) return (int)i;
		return -1;
	}
	
	const Patch *getPatch(const std::string &name)
	{
		if (!patches.count(name))
		{
			std::vector<uint8_t> lump = getLumpNamed(name);
			if (lump.size()) patches[name] = std::unique_ptr<Patch>(new Patch(lump.data()));
		}
		return patches[name].get();
	}
	const Texture *getTexture(const std::string &name) const { return textures.count(name) ? textures.at(name).get() : nullptr; }
	const Flat *getFlat(const std::string &name) const { return flats.count(name) ? flats.at(name).get() : nullptr; }
protected:
	uint8_t *data {nullptr};
	size_t numLumps {0};
	struct Directory { uint32_t lumpOffset, lumpSize; char lumpName[8] {}; };
	const Directory* dirs {nullptr};
	std::map<std::string, std::unique_ptr<Flat>> flats;
	std::map<std::string, std::unique_ptr<Patch>> patches;
	std::map<std::string, std::unique_ptr<Texture>> textures;
	std::vector<std::string> pnames;
};

