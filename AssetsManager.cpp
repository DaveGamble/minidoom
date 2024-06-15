#include "AssetsManager.h"
#include "Patch.h"
#include "Texture.h"
#include "WADLoader.h"

using namespace std;

bool AssetsManager::m_bInitialized = false;
std::unique_ptr<AssetsManager> AssetsManager::m_pInstance = nullptr;


AssetsManager* AssetsManager::GetInstance()
{
    if (!m_bInitialized)
    {
        m_pInstance = std::unique_ptr<AssetsManager>(new AssetsManager());
        m_bInitialized = true;
    }
    return m_pInstance.get();
}

