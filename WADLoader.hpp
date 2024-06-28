#pragma once

#include <map>
#include "DataTypes.hpp"
#include "Texture.hpp"
#include "Flat.hpp"

class WADLoader
{
public:
	WADLoader(const char *filename)
	{
		FILE *f = fopen(filename, "rb");
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
		std::vector<const char *> pnames;

		int32_t count = *(int32_t*)lump.data();
		for (int i = 0; i < count; ++i) pnames.push_back((const char *)lump.data() + 4 + 8 * i);
		
		int cycle = -1;
		const char *toload[2] = {"TEXTURE1", "TEXTURE2"};
		static constexpr const char *specialtextures[kNumTextureCycles][2] = { {"BLODGR1", "BLODGR4"}, {"BLODRIP1", "BLODRIP4"}, {"FIREBLU1", "FIREBLU2"}, {"FIRELAV3", "FIRELAVA"},
			{"FIREMAG1", "FIREMAG3"}, {"FIREWALA", "FIREWALL"}, {"GSTFONT1", "GSTFONT3"}, {"ROCKRED1", "ROCKRED3"}, {"SLADRIP1", "SLADRIP3"}, {"BFALL1", "BFALL4"},
			{"SFALL1", "SFALL4"}, {"WFALL1", "WFALL4"}, {"DBRAIN1", "DBRAIN4"} }; // From UDS1.666.

		for (int i = 0; i < 2; i++)
		{
			std::vector<uint8_t> data = getLumpNamed(toload[i]);
			if (!data.size()) continue;
			
			const int32_t *asint = (const int32_t*)data.data();
			int32_t numTextures = asint[0];
			for (int j = 0; j < numTextures; j++)
			{
				char name[9] {}; memcpy(name, data.data() + asint[j + 1], 8);	// THIS COMES OUT
//				const char *name = (const char *)data.data() + asint[j + 1];	// THIS GOES IN
				for (int k = 0; cycle == -1 && k < kNumTextureCycles; k++) if (!strncasecmp(name, specialtextures[k][0], 8)) cycle = k;
				textures.emplace(toupper(name), Texture(data.data() + asint[j + 1], [&](int idx) { return getPatch(pnames[idx]); }));	// THIS CHANGES
				if (cycle != -1) texturecycles[cycle].push_back(&textures.at(toupper(name)));
				for (int k = 0; cycle != -1 && k < kNumTextureCycles; k++) if (!strncasecmp(name, specialtextures[k][1], 8)) cycle = -1;
			}
		}
		
		static constexpr const char *specialflats[kNumFlatCycles][2] = {{"NUKAGE1", "NUKAGE3"}, {"FWATER1", "FWATER4"}, {"SWATER1", "SWATER4"}, {"LAVA1", "LAVA4"},
			{"BLOOD1", "BLOOD3"}, {"RROCK05", "RROCK08"}, {"SLIME01", "SLIME04"}, {"SLIME05", "SLIME08"}, {"SLIME09", "SLIME12"}}; // From UDS1.666.

		cycle = -1;
		for (int flat = findLumpByName("F_START") + 1; strncasecmp(dirs[flat].lumpName, "F_END", 8); flat++)
		{
			if (dirs[flat].lumpSize != 4096) continue;
			char name[9] {}; memcpy(name, dirs[flat].lumpName, 8);	// THIS COMES OUT
//			const char *name = dirs[flat].lumpName;	// THIS GOES IN
			for (int k = 0; cycle == -1 && k < kNumFlatCycles; k++) if (!strncasecmp(name, specialflats[k][0], 8)) cycle = k;
			flats.emplace(name, data + dirs[flat].lumpOffset);	// THIS CHANGES
			if (cycle != -1) flatcycles[cycle].push_back(&flats.at(name));
			for (int k = 0; cycle != -1 && k < kNumFlatCycles; k++) if (!strncasecmp(name, specialflats[k][1], 8)) cycle = -1;
		}
	}

	~WADLoader() { delete[] data; }
	
	bool didLoad() const { return data != nullptr; }
	
	void release() { delete[] data; data = nullptr; dirs = nullptr;
		for (int i = 0; i < kNumTextureCycles; i++) texturecycles[i].clear();
		for (int i = 0; i < kNumFlatCycles; i++) flatcycles[i].clear();
	}

	std::vector<uint8_t> getLumpNamed(const char *name, size_t after = 0) const
	{
		int id = findLumpByName(name, after);
		return (id == -1) ? std::vector<uint8_t>() : std::vector<uint8_t>(data + dirs[id].lumpOffset, data + dirs[id].lumpOffset + dirs[id].lumpSize);
	}
	int findLumpByName(const char *lumpName, size_t after = 0) const
	{
		for (size_t i = after; i < numLumps; ++i) if (!strncasecmp(dirs[i].lumpName, lumpName, 8)) return (int)i;
		return -1;
	}

	const std::vector<const char*> getPatchesStartingWith(const char *name)
	{
		std::vector<const char*> all;
		for (int i = 0; i < numLumps; i++) if (!strncasecmp(name, dirs[i].lumpName, 4)) all.push_back(dirs[i].lumpName);
		return all;
	}
	
	const Patch *getPatch(const char *name)
	{
		if (!patches.count(name))
		{
			std::vector<uint8_t> lump = getLumpNamed(name);
			if (lump.size()) patches.emplace(name, lump.data());
		}
		return &patches.at(name);
	}
	std::vector<const Texture *> getTexture(const char *name) const
	{
		std::vector<const Texture *> t;
		std::string Name = toupper(name);	// THIS CHANGES
		if (textures.count(Name))
		{
			const Texture *tex = &textures.at(Name);
			for (int i = 0; i < kNumTextureCycles; i++) for (const Texture *tt : texturecycles[i]) if (tt == tex) return texturecycles[i];
			t.push_back(tex);
		}
		return t;
	}
	std::vector<const Flat *> getFlat(const std::string &name) const
	{
		std::vector<const Flat *> f;
		std::string Name = toupper(name);	// THIS CHANGES
		if (flats.count(Name))
		{
			const Flat *flat = &flats.at(Name);
			for (int i = 0; i < kNumFlatCycles; i++) for (const Flat *ff : flatcycles[i]) if (ff == flat) return flatcycles[i];
			f.push_back(flat);
		}
		return f;
	}
protected:
	static constexpr int kNumTextureCycles = 13, kNumFlatCycles = 9;
	std::string toupper(const std::string &s) const {std::string S = s; std::transform(S.begin(), S.end(), S.begin(), ::toupper); return S;}	// THIS GOES
	uint8_t *data {nullptr};
	size_t numLumps {0};
	struct Directory { uint32_t lumpOffset, lumpSize; char lumpName[8] {}; };
	const Directory* dirs {nullptr};
	std::vector<const Texture *> texturecycles[kNumTextureCycles];
	std::vector<const Flat *> flatcycles[kNumFlatCycles];
	std::map<std::string, Flat> flats;
	std::map<std::string, Patch> patches;
	std::map<std::string, Texture> textures;
};

