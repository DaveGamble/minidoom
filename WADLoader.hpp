#pragma once

#include <map>
#include "DataTypes.hpp"
#include "Texture.h"

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

		std::vector<uint8_t> lump = GetLumpNamed("PNAMES");
		
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
			std::vector<uint8_t> data = GetLumpNamed(toload[i]);
			if (!data.size()) continue;
			
			const int32_t *asint = (const int32_t*)data.data();
			int32_t numTextures = asint[0];
			WADTextureData TextureData;
			for (int i = 0; i < numTextures; ++i)
			{
				const uint8_t *ptr = data.data() + asint[i + 1];
				memcpy(TextureData.textureName, ptr, 22);
				TextureData.texturePatch = (WADTexturePatch*)(ptr + 22);
				textures[TextureData.textureName] = std::unique_ptr<Texture>(new Texture(TextureData, this));
			}
		}
	}

	~WADLoader() { delete[] data; }

	std::vector<uint8_t> GetLumpNamed(const std::string& name, size_t after = 0) const
	{
		int id = FindLumpByName(name, after);
		return (id == -1) ? std::vector<uint8_t>() : std::vector<uint8_t>(data + dirs[id].lumpOffset, data + dirs[id].lumpOffset + dirs[id].lumpSize);
	}
	int FindLumpByName(const std::string &LumpName, size_t after = 0) const
	{
		for (size_t i = after; i < numLumps; ++i) if (!strncasecmp(dirs[i].lumpName, LumpName.c_str(), 8)) return (int)i;
		return -1;
	}
	
	Patch* GetPatch(const std::string &name)
	{
		if (!patches.count(name))
		{
			std::vector<uint8_t> lump = GetLumpNamed(name);
			if (lump.size()) patches[name] = std::unique_ptr<Patch>(new Patch(lump.data()));
		}
		return patches[name].get();
	}
	Patch* GetPatch(int index) { return GetPatch(pnames[index]); }
	Texture* GetTexture(const std::string &sTextureName) { return textures[sTextureName].get(); }

protected:
	uint8_t *data {nullptr};
	size_t numLumps {0};
	struct Directory { uint32_t lumpOffset, lumpSize; char lumpName[8] {}; };
	const Directory* dirs {nullptr};
	std::map<std::string, std::unique_ptr<Patch>> patches;
	std::map<std::string, std::unique_ptr<Texture>> textures;
	std::vector<std::string> pnames;
};

