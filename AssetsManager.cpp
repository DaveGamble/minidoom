#include "AssetsManager.h"
#include "Patch.h"
#include "Texture.h"
#include "WADLoader.h"

AssetsManager::AssetsManager(WADLoader *l) : m_pWADLoader(l)
{
	std::vector<uint8_t> lump = l->GetLumpNamed("PNAMES");
	
	int32_t count = *(int32_t*)lump.data();
	char Name[9] {};
	for (int i = 0; i < count; ++i)
	{
		memcpy(Name, lump.data() + 4 + 8 * i, 8);
		m_PNameLookup.push_back(Name);
	}

	m_pWADLoader->LoadTextures("TEXTURE1", this);
	m_pWADLoader->LoadTextures("TEXTURE2", this);
}
