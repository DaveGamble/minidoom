#include "WADLoader.h"

#include "AssetsManager.h"

#include <iostream>

using namespace std;

bool WADLoader::OpenAndLoad()
{
	FILE *f = fopen(m_sWADFilePath.c_str(), "rb");
	if (!f) return false;
	fseek(f, 0, SEEK_END);
	size_t length = ftell(f);
	fseek(f, 0, SEEK_SET);
	m_pWADData = std::unique_ptr<uint8_t[]>(new uint8_t[length]);
	fread(m_pWADData.get(), 1, length, f);
	fclose(f);
    return true;
}

bool WADLoader::ReadDirectories()
{
	struct Header { char WADType[4]; uint32_t DirectoryCount, DirectoryOffset; } *header = (Header*)m_pWADData.get();
    for (unsigned int i = 0; i < header->DirectoryCount; ++i)
    {
		Directory directory;
		memcpy(&directory, m_pWADData.get() + header->DirectoryOffset + i * 16, 16);
        m_WADDirectories.push_back(directory);
    }
    return true;
}

bool WADLoader::LoadMapData(Map *pMap)
{
    std::cout << "Info: Parsing Map: " << pMap->GetName() << endl;

    std::cout << "Info: Processing Map Vertexes" << endl;
    if (!ReadMapVertexes(pMap))
    {
        cout << "Error: Failed to load map vertexes data MAP: " << pMap->GetName() << endl;
        return false;
    }

    // Load Sector
    std::cout << "Info: Processing Map Sectors" << endl;
    if (!ReadMapSectors(pMap))
    {
        cout << "Error: Failed to load map sectors data MAP: " << pMap->GetName() << endl;
        return false;
    }

    // Load Sidedef
    std::cout << "Info: Processing Map Sidedefs" << endl;
    if (!ReadMapSidedefs(pMap))
    {
        cout << "Error: Failed to load map Sidedefs data MAP: " << pMap->GetName() << endl;
        return false;
    }

    std::cout << "Info: Processing Map Linedefs" << endl;
    if (!ReadMapLinedefs(pMap))
    {
        cout << "Error: Failed to load map linedefs data MAP: " << pMap->GetName() << endl;
        return false;
    }

    std::cout << "Info: Processing Map Segs" << endl;
    if (!ReadMapSegs(pMap))
    {
        cout << "Error: Failed to load map segs data MAP: " << pMap->GetName() << endl;
        return false;
    }

    std::cout << "Info: Processing Map Things" << endl;
    if (!ReadMapThings(pMap))
    {
        cout << "Error: Failed to load map things data MAP: " << pMap->GetName() << endl;
        return false;
    }

    std::cout << "Info: Processing Map Nodes" << endl;
    if (!ReadMapNodes(pMap))
    {
        cout << "Error: Failed to load map nodes data MAP: " << pMap->GetName() << endl;
        return false;
    }

    std::cout << "Info: Processing Map Subsectors" << endl;
    if (!ReadMapSubsectors(pMap))
    {
        cout << "Error: Failed to load map subsectors data MAP: " << pMap->GetName() << endl;
        return false;
    }

    return true;
}


WADLoader::lump WADLoader::FindMapLump(Map *pMap, const std::string& lumpName)
{
	WADLoader::lump l;
	int f = FindLumpByName(lumpName, FindMapIndex(pMap));
	if (f != -1)
	{
		l.ptr = m_pWADData.get() + m_WADDirectories[f].LumpOffset;
		l.size = m_WADDirectories[f].LumpSize;
	}
	return l;
}

int WADLoader::FindMapIndex(Map *pMap)
{
    if (pMap->GetLumpIndex() > -1) return pMap->GetLumpIndex();
    pMap->SetLumpIndex(FindLumpByName(pMap->GetName()));
    return pMap->GetLumpIndex();
}

int WADLoader::FindLumpByName(const string &LumpName, size_t offset)
{
    for (size_t i = offset; i < m_WADDirectories.size(); ++i) if (m_WADDirectories[i].LumpName == LumpName) return (int)i;
    return -1;
}

bool WADLoader::ReadMapVertexes(Map *pMap)
{
	lump l = FindMapLump(pMap, "VERTEXES");
	if (!l.ptr) return false;
    int iVertexesCount = l.size / sizeof(Vertex);
    for (int i = 0; i < iVertexesCount; ++i)
    {
		Vertex vertex;
		memcpy(&vertex, l.ptr + i * sizeof(vertex), sizeof(vertex));
        pMap->AddVertex(vertex);
    }
    return true;
}

bool WADLoader::ReadMapSectors(Map *pMap)
{
	lump l = FindMapLump(pMap, "SECTORS");
    if (!l.ptr) return false;
    int iSectorsCount = l.size / sizeof(WADSector);
    for (int i = 0; i < iSectorsCount; ++i)
    {
		WADSector sector;
		memcpy(&sector, l.ptr + i * sizeof(sector), sizeof(sector));
        pMap->AddSector(sector);
    }

    return true;
}

bool WADLoader::ReadMapSidedefs(Map *pMap)
{
	lump l = FindMapLump(pMap, "SIDEDEFS");
    if (!l.ptr) return false;
    int iSidedefsCount = l.size / sizeof(WADSidedef);
    for (int i = 0; i < iSidedefsCount; ++i)
    {
		WADSidedef sidedef;
		memcpy(&sidedef, l.ptr + i * sizeof(sidedef), sizeof(sidedef));
        pMap->AddSidedef(sidedef);
    }

    return true;
}

bool WADLoader::ReadMapLinedefs(Map *pMap)
{
	lump l = FindMapLump(pMap, "LINEDEFS");
    if (!l.ptr) return false;
    int iLinedefCount = l.size / sizeof(WADLinedef);
    for (int i = 0; i < iLinedefCount; ++i)
    {
		WADLinedef linedef;
		memcpy(&linedef, l.ptr + i * sizeof(linedef), sizeof(linedef));
        pMap->AddLinedef(linedef);
    }
    return true;
}

bool WADLoader::ReadMapThings(Map *pMap)
{
	lump l = FindMapLump(pMap, "THINGS");
    if (!l.ptr) return false;
    int iThingsCount = l.size / sizeof(Thing);
    for (int i = 0; i < iThingsCount; ++i)
    {
		Thing thing;
		memcpy(&thing, l.ptr + i * sizeof(thing), sizeof(thing));
        (pMap->GetThings())->AddThing(thing);
    }
    return true;
}

bool WADLoader::ReadMapNodes(Map *pMap)
{
	lump l = FindMapLump(pMap, "NODES");
	if (!l.ptr) return false;
    int iNodesCount = l.size / sizeof(Node);
    for (int i = 0; i < iNodesCount; ++i)
    {
		Node node;
		memcpy(&node, l.ptr + i * sizeof(node), sizeof(node));
        pMap->AddNode(node);
    }
    return true;
}

bool WADLoader::ReadMapSubsectors(Map *pMap)
{
	lump l = FindMapLump(pMap, "SSECTORS");
	if (!l.ptr) return false;
    int iSubsectorsCount = l.size / sizeof(Subsector);
    for (int i = 0; i < iSubsectorsCount; ++i)
    {
		Subsector subsector;
		memcpy(&subsector, l.ptr + i * sizeof(subsector), sizeof(subsector));
        pMap->AddSubsector(subsector);
    }
    return true;
}

bool WADLoader::ReadMapSegs(Map *pMap)
{
	lump l = FindMapLump(pMap, "SEGS");
	if (!l.ptr) return false;
    int iSegsCount = l.size / sizeof(WADSeg);
    for (int i = 0; i < iSegsCount; ++i)
    {
		WADSeg seg;
		memcpy(&seg, l.ptr + i * sizeof(seg), sizeof(seg));
        pMap->AddSeg(seg);
    }
    return true;
}

bool WADLoader::LoadPalette(DisplayManager *pDisplayManager)
{
    std::cout << "Info: Loading PLAYPAL (Color Palettes)" << endl;
    int iPlaypalIndex = FindLumpByName("PLAYPAL");

    if (strcmp(m_WADDirectories[iPlaypalIndex].LumpName, "PLAYPAL") != 0) return false;
    for (int i = 0; i < 14; ++i)
    {
		WADPalette palette;
		memcpy(&palette, m_pWADData.get() + m_WADDirectories[iPlaypalIndex].LumpOffset + (i * 768), sizeof(palette));
        pDisplayManager->AddColorPalette(palette);
    }
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
    if (strcmp(m_WADDirectories[iPNameIndex].LumpName, "PNAMES") != 0)
    {
        return false;
    }

    WADPNames PNames;
	memcpy(&PNames, m_pWADData.get() + m_WADDirectories[iPNameIndex].LumpOffset, sizeof(uint32_t));
	PNames.PNameOffset = m_WADDirectories[iPNameIndex].LumpOffset + 4;

    char Name[9];
    Name[8] = '\0';
    for (int i = 0; i < PNames.PNameCount; ++i)
    {
		memcpy(Name, m_pWADData.get() + PNames.PNameOffset, 8);
        pAssetsManager->AddPName(Name);
        PNames.PNameOffset += 8;
    }

    return true;
}

