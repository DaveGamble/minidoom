#include "WADLoader.h"
#include "AssetsManager.h"
#include "Map.h"


int WADLoader::FindLumpByName(const std::string &LumpName, size_t offset) const
{
	for (size_t i = offset; i < m_WADDirectories.size(); ++i) if (m_WADDirectories[i].LumpName == LumpName) return (int)i;
	return -1;
}

WADLoader::WADLoader(const std::string& filename)
{
	FILE *f = fopen(filename.c_str(), "rb");
	if (!f) return;
	fseek(f, 0, SEEK_END);
	size_t length = ftell(f);
	fseek(f, 0, SEEK_SET);
	m_pWADData = std::unique_ptr<uint8_t[]>(new uint8_t[length]);
	fread(m_pWADData.get(), 1, length, f);
	fclose(f);

	struct Header { char WADType[4]; uint32_t DirectoryCount, DirectoryOffset; } *header = (Header*)m_pWADData.get();
	for (unsigned int i = 0; i < header->DirectoryCount; ++i)
	{
		Directory directory;
		memcpy(&directory, m_pWADData.get() + header->DirectoryOffset + i * 16, 16);
		m_WADDirectories.push_back(directory);
	}
}

bool WADLoader::LoadMapData(Map *pMap)
{
	int li = pMap->GetLumpIndex();
	if (li <= -1) pMap->SetLumpIndex(li = FindLumpByName(pMap->GetName()));
	const uint8_t *ptr;
	size_t size;
	auto seek = [&](const std::string& name) {
		int f = FindLumpByName(name, li);
		if (f == -1) return (const uint8_t*)nullptr;
		ptr = m_pWADData.get() + m_WADDirectories[f].LumpOffset;
		size = m_WADDirectories[f].LumpSize;
		return ptr;
	};
	
	if (seek("VERTEXES")) for (int i = 0; i < size; i += sizeof(Vertex)) pMap->AddVertex(*(Vertex*)(ptr + i));
	if (seek("SECTORS")) for (int i = 0; i < size; i += sizeof(WADSector)) pMap->AddSector(*(WADSector*)(ptr + i));
	if (seek("SIDEDEFS")) for (int i = 0; i < size; i += sizeof(WADSidedef)) pMap->AddSidedef(*(WADSidedef*)(ptr + i));
	if (seek("LINEDEFS")) for (int i = 0; i < size; i += sizeof(WADLinedef)) pMap->AddLinedef(*(WADLinedef*)(ptr + i));
	if (seek("SEGS")) for (int i = 0; i < size; i += sizeof(WADSeg)) pMap->AddSeg(*(WADSeg*)(ptr + i));
	if (seek("THINGS")) for (int i = 0; i < size; i += sizeof(Thing)) pMap->GetThings()->AddThing(*(Thing*)(ptr + i));
	if (seek("NODES")) for (int i = 0; i < size; i += sizeof(Node)) pMap->AddNode(*(Node*)(ptr + i));
	if (seek("SSECTORS")) for (int i = 0; i < size; i += sizeof(Subsector)) pMap->AddSubsector(*(Subsector*)(ptr + i));
    return true;
}


bool WADLoader::LoadPatch(const std::string &sPatchName)
{
    AssetsManager *pAssetsManager = AssetsManager::GetInstance();
    int iPatchIndex = FindLumpByName(sPatchName);
    if (strcmp(m_WADDirectories[iPatchIndex].LumpName, sPatchName.c_str()) != 0)
    {
        return false;
    }

    WADPatchHeader PatchHeader;
	const uint8_t *ptr = m_pWADData.get() + m_WADDirectories[iPatchIndex].LumpOffset;
	memcpy(&PatchHeader, ptr, 8);
	PatchHeader.pColumnOffsets = new uint32_t[PatchHeader.Width];
	memcpy(PatchHeader.pColumnOffsets, ptr + 8, PatchHeader.Width * sizeof(uint32_t));

    Patch *pPatch = pAssetsManager->AddPatch(sPatchName, PatchHeader);

    PatchColumnData PatchColumn;

    for (int i = 0; i < PatchHeader.Width; ++i)
    {
        int Offset = m_WADDirectories[iPatchIndex].LumpOffset + PatchHeader.pColumnOffsets[i];
        pPatch->AppendColumnStartIndex();
        do
        {
			PatchColumn.TopDelta = m_pWADData[Offset++];
			if (PatchColumn.TopDelta != 0xFF)
			{
				PatchColumn.Length = m_pWADData[Offset++];
				PatchColumn.PaddingPre = m_pWADData[Offset++];
				// TODO: use smart pointer
				PatchColumn.pColumnData = new uint8_t[PatchColumn.Length];
				memcpy(PatchColumn.pColumnData, m_pWADData.get() + Offset, PatchColumn.Length);
				Offset += PatchColumn.Length;
				PatchColumn.PaddingPost = m_pWADData[Offset++];
			}
            pPatch->AppendPatchColumn(PatchColumn);
        } while (PatchColumn.TopDelta != 0xFF);
    }

    return true;
}

bool WADLoader::LoadTextures(const std::string &sTextureName)
{
    AssetsManager *pAssetsManager = AssetsManager::GetInstance();
    int iTextureIndex = FindLumpByName(sTextureName);
	
	if (iTextureIndex < 0)
	{
		return false;
	}

    if (strcmp(m_WADDirectories[iTextureIndex].LumpName, sTextureName.c_str()) != 0)
    {
        return false;
    }

    WADTextureHeader TextureHeader;
	const uint8_t *ptr = m_pWADData.get() + m_WADDirectories[iTextureIndex].LumpOffset;
	memcpy(&TextureHeader, ptr, 8);
	TextureHeader.pTexturesDataOffset = new uint32_t[TextureHeader.TexturesCount];
	memcpy(TextureHeader.pTexturesDataOffset, ptr + 4, TextureHeader.TexturesCount * sizeof(uint32_t));	// <-- How is this +4 correct???

	WADTextureData TextureData;
    for (int i = 0; i < TextureHeader.TexturesCount; ++i)
    {
		ptr = m_pWADData.get() + m_WADDirectories[iTextureIndex].LumpOffset + TextureHeader.pTexturesDataOffset[i];
		memcpy(TextureData.TextureName, ptr, 8);
		TextureData.TextureName[8] = '\0';
		memcpy(&TextureData.Flags, ptr + 8, 14);
		TextureData.pTexturePatch = new WADTexturePatch[TextureData.PatchCount];
		memcpy(TextureData.pTexturePatch, ptr + 22, TextureData.PatchCount * 10);
        pAssetsManager->AddTexture(TextureData);
        delete[] TextureData.pTexturePatch;
        TextureData.pTexturePatch = nullptr;
    }

    delete[] TextureHeader.pTexturesDataOffset;
    TextureHeader.pTexturesDataOffset = nullptr;
    return true;
}

bool WADLoader::LoadPNames()
{
    AssetsManager *pAssetsManager = AssetsManager::GetInstance();
    int iPNameIndex = FindLumpByName("PNAMES");
    if (strcmp(m_WADDirectories[iPNameIndex].LumpName, "PNAMES") != 0) return false;

    WADPNames PNames;
	memcpy(&PNames, m_pWADData.get() + m_WADDirectories[iPNameIndex].LumpOffset, sizeof(uint32_t));
	PNames.PNameOffset = m_WADDirectories[iPNameIndex].LumpOffset + 4;

	char Name[9] {};
    for (int i = 0; i < PNames.PNameCount; ++i)
    {
		memcpy(Name, m_pWADData.get() + PNames.PNameOffset, 8);
        pAssetsManager->AddPName(Name);
        PNames.PNameOffset += 8;
    }

    return true;
}

