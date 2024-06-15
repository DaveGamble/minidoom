#include "WADLoader.h"
#include "AssetsManager.h"
#include "Map.h"

WADLoader::WADLoader(const std::string& filename)
{
	FILE *f = fopen(filename.c_str(), "rb");
	if (!f) return;
	fseek(f, 0, SEEK_END);
	size_t length = ftell(f);
	fseek(f, 0, SEEK_SET);
	data = new uint8_t[length];
	fread(data, 1, length, f);
	fclose(f);

	numLumps = ((const uint32_t*)data)[1];
	dirs = (const Directory*)(data + ((const uint32_t*)data)[2]);
}

bool WADLoader::LoadMapData(Map *pMap) const
{
	int li = pMap->GetLumpIndex();
	if (li <= -1) pMap->SetLumpIndex(li = FindLumpByName(pMap->GetName()));
	const uint8_t *ptr;
	size_t size;
	auto seek = [&](const std::string& name) {
		int f = FindLumpByName(name, li);
		if (f == -1) return (const uint8_t*)nullptr;
		ptr = data + dirs[f].LumpOffset;
		size = dirs[f].LumpSize;
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

int WADLoader::FindLumpByName(const std::string &LumpName, size_t after) const
{
	for (size_t i = after; i < numLumps; ++i) if (!strncasecmp(dirs[i].LumpName, LumpName.c_str(), 8)) return (int)i;
	return -1;
}

std::vector<uint8_t> WADLoader::GetLumpNamed(const std::string& name, size_t after) const
{
	int id = FindLumpByName(name, after);
	return (id == -1) ? std::vector<uint8_t>() : std::vector<uint8_t>(data + dirs[id].LumpOffset, data + dirs[id].LumpOffset + dirs[id].LumpSize);
}

