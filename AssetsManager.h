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

    Texture* GetTexture(const std::string &sTextureName) { return m_TexturesCache[sTextureName].get(); }
	std::string GetPName(int PNameIndex) { return m_PNameLookup[PNameIndex]; }
protected:

	void LoadPatch(const std::string &sPatchName);


    std::map<std::string, std::unique_ptr<Patch>> m_PatchesCache;
    std::map<std::string, std::unique_ptr<Texture>> m_TexturesCache;
    

    std::vector<std::string> m_PNameLookup;

	WADLoader* m_pWADLoader {nullptr};
};
