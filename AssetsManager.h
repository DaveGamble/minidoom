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

    Patch* AddPatch(const std::string &sPatchName, WADPatchHeader &PatchHeader)
	{
		m_PatchesCache[sPatchName] = std::unique_ptr<Patch> (new Patch(sPatchName));
		m_PatchesCache[sPatchName]->Initialize(PatchHeader);
		return m_PatchesCache[sPatchName].get();
	}
	Patch* GetPatch(const std::string &sPatchName) { if (m_PatchesCache.count(sPatchName) <= 0) LoadPatch(sPatchName); return m_PatchesCache[sPatchName].get(); }

    void AddTexture(WADTextureData &TextureData) { m_TexturesCache[TextureData.TextureName] = std::unique_ptr<Texture>(new Texture(TextureData)); }
    Texture* GetTexture(const std::string &sTextureName)
	{
		if (!m_TexturesCache.count(sTextureName)) return nullptr;
		Texture* pTexture = m_TexturesCache[sTextureName].get();
		if (!pTexture->IsComposed()) pTexture->Compose(this);
		return pTexture;
	}

	std::string GetPName(int PNameIndex) { return m_PNameLookup[PNameIndex]; }


protected:

	void LoadPatch(const std::string &sPatchName) { m_pWADLoader->LoadPatch(sPatchName, this); }


    std::map<std::string, std::unique_ptr<Patch>> m_PatchesCache;
    std::map<std::string, std::unique_ptr<Texture>> m_TexturesCache;
    

    std::vector<std::string> m_PNameLookup;

	WADLoader* m_pWADLoader {nullptr};
};
