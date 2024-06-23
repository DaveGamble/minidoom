#pragma once

#include <map>
#include "DataTypes.hpp"
#include "Texture.hpp"
#include "Flat.hpp"

class WADLoader
{
public:
	WADLoader(const std::string &filename)
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

		std::vector<uint8_t> lump = getLumpNamed("PNAMES");
		int32_t count = *(int32_t*)lump.data();
		for (int i = 0; i < count; ++i)
		{
			char Name[9] {}; memcpy(Name, lump.data() + 4 + 8 * i, 8);
			pnames.push_back(Name);
		}
		
		const char *toload[2] = {"TEXTURE1", "TEXTURE2"};
		for (int i = 0; i < 2; i++)
		{
			std::vector<uint8_t> data = getLumpNamed(toload[i]);
			if (!data.size()) continue;
			
			const int32_t *asint = (const int32_t*)data.data();
			int32_t numTextures = asint[0];
			for (int i = 0; i < numTextures; ++i)
			{
				char name[9] {}; memcpy(name, data.data() + asint[i + 1], 8);
				textures[toupper(name)] = std::unique_ptr<Texture>(new Texture(data.data() + asint[i + 1], [&](int idx) { return getPatch(pnames[idx]); }));
			}
		}
		
		for (int flat = findLumpByName("F_START") + 1; strncasecmp(dirs[flat].lumpName, "F_END", 8); flat++)
		{
			if (dirs[flat].lumpSize != 4096) continue;
			char name[9] {}; memcpy(name, dirs[flat].lumpName, 8);
			flats[name] = std::unique_ptr<Flat>(new Flat(data + dirs[flat].lumpOffset));
		}
	}

	~WADLoader() { delete[] data; }

	std::vector<uint8_t> getLumpNamed(const std::string& name, size_t after = 0) const
	{
		int id = findLumpByName(name, after);
		return (id == -1) ? std::vector<uint8_t>() : std::vector<uint8_t>(data + dirs[id].lumpOffset, data + dirs[id].lumpOffset + dirs[id].lumpSize);
	}
	int findLumpByName(const std::string &LumpName, size_t after = 0) const
	{
		for (size_t i = after; i < numLumps; ++i) if (!strncasecmp(dirs[i].lumpName, LumpName.c_str(), 8)) return (int)i;
		return -1;
	}
	
	const Patch *getPatch(const std::string &name)
	{
		if (!patches.count(name))
		{
			std::vector<uint8_t> lump = getLumpNamed(name);
			if (lump.size()) patches[name] = std::unique_ptr<Patch>(new Patch(lump.data()));
		}
		return patches[name].get();
	}
	std::vector<const Texture *> getTexture(const std::string &name) const
	{
		std::vector<const Texture *> t;
		std::string Name = toupper(name);
		if (textures.count(Name)) t.push_back(textures.at(Name).get());
		return t;
	}
	std::vector<const Flat *> getFlat(const std::string &name) const { std::vector<const Flat *> f;
		if (flats.count(name)) f.push_back(flats.at(name).get());
		return f;
	}
protected:
	static constexpr const char *specialtextures[][2] = { {"BLODGR1", "BLODGR4"}, {"BLODRIP1", "BLODRIP4"}, {"FIREBLU1", "FIREBLU2"}, {"FIRELAV3", "FIRELAVA"},
		{"FIREMAG1", "FIREMAG3"}, {"FIREWALA", "FIREWALL"}, {"GSTFONT1", "GSTFONT3"}, {"ROCKRED1", "ROCKRED3"}, {"SLADRIP1", "SLADRIP3"}, {"BFALL1", "BFALL4"},
		{"SFALL1", "SFALL4"}, {"WFALL1", "WFALL4"}, {"DBRAIN1", "DBRAIN4"} }; // From UDS1.666.

	static constexpr const char *specialflats[][2] = {{"NUKAGE1", "NUKAGE3"}, {"FWATER1", "FWATER4"}, {"SWATER1", "SWATER4"}, {"LAVA1", "LAVA4"},
		{"BLOOD1", "BLOOD3"}, {"RROCK05", "RROCK08"}, {"SLIME01", "SLIME04"}, {"SLIME05", "SLIME08"}, {"SLIME09", "SLIME12"}};

	
	std::string toupper(const std::string &s) const {std::string S = s; std::transform(S.begin(), S.end(), S.begin(), ::toupper); return S;}
	uint8_t *data {nullptr};
	size_t numLumps {0};
	struct Directory { uint32_t lumpOffset, lumpSize; char lumpName[8] {}; };
	const Directory* dirs {nullptr};
	std::map<std::string, std::unique_ptr<Flat>> flats;
	std::map<std::string, std::unique_ptr<Patch>> patches;
	std::map<std::string, std::unique_ptr<Texture>> textures;
	std::vector<std::string> pnames;
};

