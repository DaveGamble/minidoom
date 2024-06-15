#pragma once

#include <map>
#include <string>
#include <vector>
#include "DataTypes.h"

class Patch;
class Texture;
class WADLoader;

class AssetsManager
{
public:
    static AssetsManager* GetInstance();
    void Init(WADLoader* pWADLoader);

    ~AssetsManager();

    Patch* AddPatch(const std::string &sPatchName, WADPatchHeader &PatchHeader);
    Patch* GetPatch(const std::string &sPatchName);

    Texture* AddTexture(WADTextureData &TextureData);
    Texture* GetTexture(const std::string &sTextureName);

    void AddPName(const std::string &PName);
    std::string GetPName(int PNameIndex);


protected:
    static bool m_bInitialized;
    static std::unique_ptr <AssetsManager> m_pInstance;

    AssetsManager();

    void LoadPatch(const std::string &sPatchName);
    void LoadTextures();


    std::map<std::string, std::unique_ptr<Patch>> m_PatchesCache;
    std::map<std::string, std::unique_ptr<Texture>> m_TexturesCache;
    

    std::vector<std::string> m_PNameLookup;

	WADLoader* m_pWADLoader {nullptr};
};
