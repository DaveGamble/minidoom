#include "WADLoader.h"
#include "AssetsManager.h"
#include "Map.h"


int WADLoader::FindLumpByName(const std::string &LumpName, size_t offset) const
{
	for (size_t i = offset; i < m_WADDirectories.size(); ++i) if (!strncasecmp(m_WADDirectories[i].LumpName, LumpName.c_str(), 8)) return (int)i;
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
		m_WADDirectories.push_back(*(Directory*)(m_pWADData.get() + header->DirectoryOffset + i * 16));
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

std::vector<uint8_t> WADLoader::GetLumpNamed(const std::string& name) const
{
	int id = FindLumpByName(name);
	if (id == -1) return {};
	return std::vector<uint8_t>(m_pWADData.get() + m_WADDirectories[id].LumpOffset, m_pWADData.get() + m_WADDirectories[id].LumpOffset + m_WADDirectories[id].LumpSize);
}

