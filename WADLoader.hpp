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

		int npnames = findLumpByName("PNAMES");
		std::vector<const char *> pnames;
		const char *pn = (const char *)(data + dirs[npnames].lumpOffset);
		int32_t count = *(int32_t*)pn;
		for (int i = 0; i < count; ++i) pnames.push_back(pn + 4 + 8 * i);
		
		int cycle = -1;
		const char *toload[2] = {"TEXTURE1", "TEXTURE2"};
		static constexpr const char *specialtextures[kNumTextureCycles][2] = { {"BLODGR1", "BLODGR4"}, {"BLODRIP1", "BLODRIP4"}, {"FIREBLU1", "FIREBLU2"}, {"FIRELAV3", "FIRELAVA"},
			{"FIREMAG1", "FIREMAG3"}, {"FIREWALA", "FIREWALL"}, {"GSTFONT1", "GSTFONT3"}, {"ROCKRED1", "ROCKRED3"}, {"SLADRIP1", "SLADRIP3"}, {"BFALL1", "BFALL4"},
			{"SFALL1", "SFALL4"}, {"WFALL1", "WFALL4"}, {"DBRAIN1", "DBRAIN4"} }; // From UDS1.666.

		for (int i = 0; i < 2; i++)
		{
			int n = findLumpByName(toload[i]);
			const uint8_t *lump = data + dirs[n].lumpOffset;
			if (!dirs[n].lumpSize) continue;
			
			const int32_t *asint = (const int32_t*)lump;
			int32_t numTextures = asint[0];
			for (int j = 0; j < numTextures; j++)
			{
				const char *name = (const char *)lump + asint[j + 1];
				for (int k = 0; cycle == -1 && k < kNumTextureCycles; k++) if (!strncasecmp(name, specialtextures[k][0], 8)) cycle = k;
				textures.push_back(new Texture(name, lump + asint[j + 1], [&](int idx) { return getPatch(pnames[idx]); }));
				if (cycle != -1) texturecycles[cycle].push_back(textures[textures.size() - 1]);
				for (int k = 0; cycle != -1 && k < kNumTextureCycles; k++) if (!strncasecmp(name, specialtextures[k][1], 8)) cycle = -1;
			}
		}
		
		static constexpr const char *specialflats[kNumFlatCycles][2] = {{"NUKAGE1", "NUKAGE3"}, {"FWATER1", "FWATER4"}, {"SWATER1", "SWATER4"}, {"LAVA1", "LAVA4"},
			{"BLOOD1", "BLOOD3"}, {"RROCK05", "RROCK08"}, {"SLIME01", "SLIME04"}, {"SLIME05", "SLIME08"}, {"SLIME09", "SLIME12"}}; // From UDS1.666.

		cycle = -1;
		for (int flat = findLumpByName("F_START") + 1; strncasecmp(dirs[flat].lumpName, "F_END", 8); flat++)
		{
			if (dirs[flat].lumpSize != 4096) continue;
			const char *name = dirs[flat].lumpName;	// THIS GOES IN
			for (int k = 0; cycle == -1 && k < kNumFlatCycles; k++) if (!strncasecmp(name, specialflats[k][0], 8)) cycle = k;
			flats.push_back(new Flat(name, data + dirs[flat].lumpOffset));
			if (cycle != -1) flatcycles[cycle].push_back(flats[flats.size() - 1]);
			for (int k = 0; cycle != -1 && k < kNumFlatCycles; k++) if (!strncasecmp(name, specialflats[k][1], 8)) cycle = -1;
		}
	}

	~WADLoader()
	{
		delete[] data;
		for (Patch *p : patches) delete p;
		for (Texture *t : textures) delete t;
		for (Flat *f : flats) delete f;
	}
	
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
		for (const Patch *p : patches) if (!strncasecmp(name, p->getName(), 8)) return p;
		int n = findLumpByName(name);
		if (dirs[n].lumpSize) patches.push_back(new Patch(dirs[n].lumpName, data + dirs[n].lumpOffset));
		return patches[patches.size() - 1];
	}
	std::vector<const Texture *> getTexture(const char *name) const
	{
		std::vector<const Texture *> ts;
		for (const Texture *t : textures)
		{
			if (strncasecmp(name, t->getName(), 8)) continue;
			for (int i = 0; i < kNumTextureCycles; i++) for (const Texture *tt : texturecycles[i]) if (tt == t) return texturecycles[i];
			ts.push_back(t);
		}
		return ts;
	}
	std::vector<const Flat *> getFlat(const char *name) const
	{
		std::vector<const Flat *> fs;
		for (const Flat *f : flats)
		{
			if (strncasecmp(name, f->getName(), 8)) continue;
			for (int i = 0; i < kNumFlatCycles; i++) for (const Flat *ff : flatcycles[i]) if (ff == f) return flatcycles[i];
			fs.push_back(f);
		}
		return fs;
	}
protected:
	static constexpr int kNumTextureCycles = 13, kNumFlatCycles = 9;
	uint8_t *data {nullptr};
	size_t numLumps {0};
	struct Directory { uint32_t lumpOffset, lumpSize; char lumpName[8] {}; };
	const Directory* dirs {nullptr};
	std::vector<const Texture *> texturecycles[kNumTextureCycles];
	std::vector<const Flat *> flatcycles[kNumFlatCycles];
	std::vector<Flat *> flats;
	std::vector<Patch *> patches;
	std::vector<Texture *> textures;
};

