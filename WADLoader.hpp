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
			m_PNameLookup.push_back(Name);
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
				memcpy(TextureData.TextureName, ptr, 22);
				TextureData.pTexturePatch = (WADTexturePatch*)(ptr + 22);
				m_TexturesCache[TextureData.TextureName] = std::unique_ptr<Texture>(new Texture(TextureData, this));
			}
		}
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
	
	Patch* GetPatch(const std::string &sPatchName) { if (m_PatchesCache.count(sPatchName) <= 0) LoadPatch(sPatchName); return m_PatchesCache[sPatchName].get(); }
	Patch* GetPatch(int index) { return GetPatch(m_PNameLookup[index]); }
	Texture* GetTexture(const std::string &sTextureName) { return m_TexturesCache[sTextureName].get(); }

protected:
	const Directory* dirs {nullptr};
	size_t numLumps {0};
	uint8_t *data {nullptr};
	void LoadPatch(const std::string &sPatchName)
	{
		std::vector<uint8_t> lump = GetLumpNamed(sPatchName);
		if (!lump.size()) return;
		m_PatchesCache[sPatchName] = std::unique_ptr<Patch>(new Patch(lump.data()));
	}
	std::map<std::string, std::unique_ptr<Patch>> m_PatchesCache;
	std::map<std::string, std::unique_ptr<Texture>> m_TexturesCache;
	std::vector<std::string> m_PNameLookup;
};

