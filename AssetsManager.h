#pragma once

#include <map>
#include <string>
#include <vector>
#include "DataTypes.h"
#include "WADLoader.h"
#include "Patch.h"
#include "Texture.h"

class AssetsManager
{
public:
	AssetsManager(WADLoader *l);
	~AssetsManager() {}

	Patch* GetPatch(const std::string &sPatchName) { if (m_PatchesCache.count(sPatchName) <= 0) LoadPatch(sPatchName); return m_PatchesCache[sPatchName].get(); }
	Patch* GetPatch(int index) { return GetPatch(m_PNameLookup[index]); }
    Texture* GetTexture(const std::string &sTextureName) { return m_TexturesCache[sTextureName].get(); }
protected:
	void LoadPatch(const std::string &sPatchName);
    std::map<std::string, std::unique_ptr<Patch>> m_PatchesCache;
    std::map<std::string, std::unique_ptr<Texture>> m_TexturesCache;
    std::vector<std::string> m_PNameLookup;
	WADLoader* wadLoader {nullptr};
};
