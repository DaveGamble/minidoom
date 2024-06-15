#include "AssetsManager.h"
#include "Patch.h"
#include "Texture.h"
#include "WADLoader.h"

AssetsManager::AssetsManager(WADLoader *l) : wadLoader(l)
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
			auto texture = std::unique_ptr<Texture>(new Texture(TextureData));
			if (!texture->IsComposed()) texture->Compose(this);
			m_TexturesCache[TextureData.TextureName] = std::move(texture);
		}
	}
}

void AssetsManager::LoadPatch(const std::string &sPatchName)
{
	std::vector<uint8_t> lump = wadLoader->GetLumpNamed(sPatchName);
	if (!lump.size()) return;

	WADPatchHeader PatchHeader;
	const uint8_t *ptr = lump.data();
	memcpy(&PatchHeader, ptr, 8);
	PatchHeader.pColumnOffsets = new uint32_t[PatchHeader.Width];
	memcpy(PatchHeader.pColumnOffsets, ptr + 8, PatchHeader.Width * sizeof(uint32_t));

	m_PatchesCache[sPatchName] = std::unique_ptr<Patch> (new Patch(sPatchName));
	m_PatchesCache[sPatchName]->Initialize(PatchHeader);
	Patch *pPatch = m_PatchesCache[sPatchName].get();

	PatchColumnData PatchColumn;

	for (int i = 0; i < PatchHeader.Width; ++i)
	{
		int Offset = PatchHeader.pColumnOffsets[i];
		pPatch->AppendColumnStartIndex();
		do
		{
			PatchColumn.TopDelta = ptr[Offset++];
			if (PatchColumn.TopDelta != 0xFF)
			{
				PatchColumn.Length = ptr[Offset++];
				PatchColumn.PaddingPre = ptr[Offset++];
				// TODO: use smart pointer
				PatchColumn.pColumnData = new uint8_t[PatchColumn.Length];
				memcpy(PatchColumn.pColumnData, ptr + Offset, PatchColumn.Length);
				Offset += PatchColumn.Length;
				PatchColumn.PaddingPost = ptr[Offset++];
			}
			pPatch->AppendPatchColumn(PatchColumn);
		} while (PatchColumn.TopDelta != 0xFF);
	}
}
