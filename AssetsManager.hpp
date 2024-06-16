#pragma once

#include <map>
#include <string>
#include <vector>
#include "DataTypes.hpp"
#include "WADLoader.hpp"
#include "Patch.h"
#include "Texture.h"

class AssetsManager
{
public:
	AssetsManager(WADLoader *l) : wadLoader(l)
	{
		std::vector<uint8_t> lump = l->GetLumpNamed("PNAMES");
		
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
			std::vector<uint8_t> data = l->GetLumpNamed(toload[i]);
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
	~AssetsManager() {}

	Patch* GetPatch(const std::string &sPatchName) { if (m_PatchesCache.count(sPatchName) <= 0) LoadPatch(sPatchName); return m_PatchesCache[sPatchName].get(); }
	Patch* GetPatch(int index) { return GetPatch(m_PNameLookup[index]); }
    Texture* GetTexture(const std::string &sTextureName) { return m_TexturesCache[sTextureName].get(); }
protected:
	void LoadPatch(const std::string &sPatchName)
	{
		std::vector<uint8_t> lump = wadLoader->GetLumpNamed(sPatchName);
		if (!lump.size()) return;
		m_PatchesCache[sPatchName] = std::unique_ptr<Patch>(new Patch(lump.data()));
	}
    std::map<std::string, std::unique_ptr<Patch>> m_PatchesCache;
    std::map<std::string, std::unique_ptr<Texture>> m_TexturesCache;
    std::vector<std::string> m_PNameLookup;
	WADLoader* wadLoader {nullptr};
};