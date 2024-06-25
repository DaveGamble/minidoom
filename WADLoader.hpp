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
				char name[9] {}; memcpy(name, data.data() + asint[j + 1], 8);
				for (int k = 0; cycle == -1 && k < kNumTextureCycles; k++) if (!strcasecmp(name, specialtextures[k][0])) cycle = k;
				textures[toupper(name)] = std::unique_ptr<Texture>(new Texture(data.data() + asint[j + 1], [&](int idx) { return getPatch(pnames[idx]); }));
				if (cycle != -1) texturecycles[cycle].push_back(textures[toupper(name)].get());
				for (int k = 0; cycle != -1 && k < kNumTextureCycles; k++) if (!strcasecmp(name, specialtextures[k][1])) cycle = -1;
			}
		}
		
		static constexpr const char *specialflats[kNumFlatCycles][2] = {{"NUKAGE1", "NUKAGE3"}, {"FWATER1", "FWATER4"}, {"SWATER1", "SWATER4"}, {"LAVA1", "LAVA4"},
			{"BLOOD1", "BLOOD3"}, {"RROCK05", "RROCK08"}, {"SLIME01", "SLIME04"}, {"SLIME05", "SLIME08"}, {"SLIME09", "SLIME12"}}; // From UDS1.666.

		cycle = -1;
		for (int flat = findLumpByName("F_START") + 1; strncasecmp(dirs[flat].lumpName, "F_END", 8); flat++)
		{
			if (dirs[flat].lumpSize != 4096) continue;
			char name[9] {}; memcpy(name, dirs[flat].lumpName, 8);
			for (int k = 0; cycle == -1 && k < kNumFlatCycles; k++) if (!strcasecmp(name, specialflats[k][0])) cycle = k;
			flats[name] = std::unique_ptr<Flat>(new Flat(data + dirs[flat].lumpOffset));
			if (cycle != -1) flatcycles[cycle].push_back(flats[name].get());
			for (int k = 0; cycle != -1 && k < kNumFlatCycles; k++) if (!strcasecmp(name, specialflats[k][1])) cycle = -1;
		}
	}

	~WADLoader() { delete[] data; }
	
	void release() { delete[] data; data = nullptr; dirs = nullptr; pnames.clear();
		for (int i = 0; i < kNumTextureCycles; i++) texturecycles[i].clear();
		for (int i = 0; i < kNumFlatCycles; i++) flatcycles[i].clear();
	}

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

	const std::vector<std::string> getPatchesStartingWith(const char *name)
	{
		std::vector<std::string> all;
		for (int i = 0; i < numLumps; i++)
		{
			if (strncmp(name, dirs[i].lumpName, 4)) continue;
			char name[9] {}; memcpy(name, dirs[i].lumpName, 8);
			all.push_back(name);
		}
		return all;
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
		if (textures.count(Name))
		{
			const Texture *tex = textures.at(Name).get();
			for (int i = 0; i < kNumTextureCycles; i++) for (const Texture *tt : texturecycles[i]) if (tt == tex) return texturecycles[i];
			t.push_back(tex);
		}
		return t;
	}
	std::vector<const Flat *> getFlat(const std::string &name) const
	{
		std::vector<const Flat *> f;
		std::string Name = toupper(name);
		if (flats.count(Name))
		{
			const Flat *flat = flats.at(Name).get();
			for (int i = 0; i < kNumFlatCycles; i++) for (const Flat *ff : flatcycles[i]) if (ff == flat) return flatcycles[i];
			f.push_back(flat);
		}
		return f;
	}
protected:
	static constexpr int kNumTextureCycles = 13, kNumFlatCycles = 9;
	std::string toupper(const std::string &s) const {std::string S = s; std::transform(S.begin(), S.end(), S.begin(), ::toupper); return S;}
	uint8_t *data {nullptr};
	size_t numLumps {0};
	struct Directory { uint32_t lumpOffset, lumpSize; char lumpName[8] {}; };
	const Directory* dirs {nullptr};
	std::vector<const Texture *> texturecycles[kNumTextureCycles];
	std::vector<const Flat *> flatcycles[kNumFlatCycles];
	std::map<std::string, std::unique_ptr<Flat>> flats;
	std::map<std::string, std::unique_ptr<Patch>> patches;
	std::map<std::string, std::unique_ptr<Texture>> textures;
	std::vector<std::string> pnames;
};

