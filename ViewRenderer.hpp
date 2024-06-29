#pragma once

#include <list>
#include <cstdint>
#include <vector>

enum { kUpperTextureUnpeg = 8, kLowerTextureUnpeg = 16 };
enum { thing_collectible = 1, thing_obstructs = 2, thing_hangs = 4, thing_artefact = 8 };

class Patch
{
public:
	Patch(const char *_name, const uint8_t *ptr) : name(_name)
	{
		struct WADPatchHeader { uint16_t width, height; int16_t leftOffset, topOffset; };

		WADPatchHeader *patchHeader = (WADPatchHeader*)ptr;
		width = patchHeader->width;
		height = patchHeader->height;
		xoffset = patchHeader->leftOffset;
		yoffset = patchHeader->topOffset;

		uint32_t *columnOffsets = (uint32_t*)(ptr + 8);
		for (int i = 0; i < width; ++i)
		{
			int off = columnOffsets[i];
			index.push_back((int)cols.size());
			PatchColumnData patchColumn;
			do
			{
				patchColumn.top = ptr[off++];
				if (patchColumn.top != 0xFF)
				{
					patchColumn.length = ptr[off++];
					patchColumn.paddingPre = ptr[off++];
					patchColumn.data = new uint8_t[patchColumn.length];
					memcpy(patchColumn.data, ptr + off, patchColumn.length);
					off += patchColumn.length;
					patchColumn.paddingPost = ptr[off++];
				}
				cols.push_back(patchColumn);
			} while (patchColumn.top != 0xFF);
		}
	}

	~Patch()
	{
		for (size_t i = 0; i < cols.size(); ++i) if (cols[i].top != 0xFF) delete[] cols[i].data;
	}


	void render(uint8_t *buf, int rowlen, int screenx, int screeny, const uint8_t *lut, float scale = 1) const
	{
		buf += rowlen * screeny + screenx;
		for (int x = 0, tox = 0; x < width; x++) while (tox < (x + 1) * scale) renderColumn(buf + tox++, rowlen, index[x], INT_MAX, 0, scale, lut);
	}
	void renderColumn(uint8_t *buf, int rowlen, int firstColumn, int maxHeight, int yOffset, float scale, const uint8_t *lut) const
	{
		if (scale < 0) return;
		while (cols[firstColumn].top != 0xFF)
		{
			int y = (cols[firstColumn].top + yOffset < 0) ? - cols[firstColumn].top - yOffset : 0;
			int sl = floor(scale * (cols[firstColumn].top + y + yOffset));
			int el = std::min(floor((cols[firstColumn].length - y) * scale) + sl, (float)maxHeight);
			int run = std::max(el - sl, 0), start = rowlen * sl;
			const uint8_t *from = cols[firstColumn].data + y;
			for (int i = 0, to = 0; to < run && i < cols[firstColumn].length; i++)
				while (to < (i + 1) * scale && to < run) buf[start + (to++) * rowlen] = lut[from[i]];
			++firstColumn;
		}
	}
	uint16_t pixel(int x, int y) const
	{
		for ( ; cols[x].top != 0xff && cols[x].top <= y; x++) { int o = y - cols[x].top; if (o >= 0 && o < cols[x].length) return cols[x].data[o]; } return 256;
	}
	void composeColumn(uint8_t *buf, int iHeight, int firstColumn, int yOffset) const
	{
		while (cols[firstColumn].top != 0xFF)
		{
			int y = yOffset + cols[firstColumn].top, iMaxRun = cols[firstColumn].length;
			if (y < 0) { iMaxRun += y; y = 0; }
			iMaxRun = std::min(iHeight - y, iMaxRun);
			if (iMaxRun > 0) memcpy(buf + y, cols[firstColumn].data, iMaxRun);
			++firstColumn;
		}
	}

	int getWidth() const { return width; }
	int getHeight() const { return height; }
	int getXOffset() const { return xoffset; }
	int getYOffset() const { return yoffset; }
	int getColumnDataIndex(int x) const { return index[x]; }
	const char *getName() const {return name;}

protected:
	const char *name;
	int height {0}, width {0}, xoffset {0}, yoffset {0};
	struct PatchColumnData { uint8_t top, length, paddingPre, *data, paddingPost; };

	std::vector<PatchColumnData> cols;
	std::vector<int> index;
};


class Texture
{
public:
	template<typename F> Texture(const char *_name, const uint8_t *ptr, F &&getPatch) : name(_name)
	{
		struct WADTextureData { char textureName[8]; uint32_t alwayszero; uint16_t width, height; uint32_t alwayszero2; uint16_t patchCount; };
		WADTextureData *textureData = (WADTextureData*)ptr;
		struct WADTexturePatch { int16_t dx, dy; uint16_t pnameIndex, alwaysone, alwayszero; };
		WADTexturePatch *texturePatch = (WADTexturePatch*)(ptr + 22);

		width = textureData->width;
		height = textureData->height;
		columns.resize(width);

		for (int i = 0; i < textureData->patchCount; ++i)
		{
			const Patch *patch = getPatch(texturePatch[i].pnameIndex);	// Get the patch
			for (int x = std::max(texturePatch[i].dx, (int16_t)0); x < std::min(width, texturePatch[i].dx + patch->getWidth()); x++)
			{
				if (columns[x].patch)	// This column already has something in
				{
					if (!columns[x].overlap.size())	// Need to render off what's in there!
					{
						columns[x].overlap.resize(height);
						columns[x].patch->composeColumn(columns[x].overlap.data(), height, columns[x].column, columns[x].yOffset);
					}
					patch->composeColumn(columns[x].overlap.data(), height, patch->getColumnDataIndex(x - texturePatch[i].dx), texturePatch[i].dy);	// Render your goodies on top.
				}
				else columns[x] = { patch->getColumnDataIndex(x - texturePatch[i].dx), texturePatch[i].dy, patch, {}};	// Save this as the handler for this column.
			}
		}
	}

	const Patch *getPatchForColumn(int x, int &col, int &yOffset) const {
		if (!columns[x].overlap.size()) {col = columns[x].column; yOffset = columns[x].yOffset; return columns[x].patch;} return nullptr;
	}
	uint16_t pixel(int x, int y) const { x %= width; y %= height; if (x < 0) x += width; if (y < 0) y += height;
		return (columns[x].overlap.size()) ? columns[x].overlap[y] : columns[x].patch->pixel(columns[x].column, y - columns[x].yOffset);
	}
	int getWidth() const { return width; }
	int getHeight() const { return height; }
	const char *getName() const {return name;}
protected:
	const char *name;
	int width, height;
	struct TextureColumn { int column, yOffset; const Patch *patch; std::vector<uint8_t> overlap; };
	std::vector<TextureColumn> columns;
};

class Flat
{
public:
	Flat(const char *_name, const uint8_t *_data) : name(_name) { memcpy(data, _data, 4096); }
	uint8_t pixel(int u, int v) const { return data[(64 * v + (u & 63)) & 4095]; }
	const char *getName() const {return name;}
protected:
	const char *name;
	uint8_t data[4096];
};
struct Thing { int16_t x, y; uint16_t angle, type, flags; std::vector<const Patch *> imgs; int attr; };
struct Vertex { int16_t x, y; };
struct Sector { int16_t floorHeight, ceilingHeight; std::vector<const Flat *> floortexture, ceilingtexture; uint16_t lightlevel, maxlightlevel, minlightlevel, type, tag; const Patch *sky; std::vector<const struct Linedef*> linedefs; std::vector<Thing *> things; bool thingsThisFrame; };
struct Sidedef { int16_t dx, dy; std::vector<const Texture *> uppertexture, middletexture, lowertexture; Sector *sector; };
struct Linedef { Vertex start, end; uint16_t flags, type, sectorTag; Sidedef *rSidedef, *lSidedef; };
struct Seg { Vertex start, end; float slopeAngle; const Linedef *linedef; Sidedef *sidedef; uint16_t direction; float offset, len; Sector *rSector, *lSector; }; // Direction: 0 same as linedef, 1 opposite of linedef. Offset: distance along linedef to start of seg.
struct Viewpoint { int16_t x, y, z; float angle, cosa, sina, pitch; };

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
			if (n == -1) continue;
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
		if (n != -1 && dirs[n].lumpSize) patches.push_back(new Patch(dirs[n].lumpName, data + dirs[n].lumpOffset));
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

class ViewRenderer
{
public:
    ViewRenderer(int renderXSize, int renderYSize, const char *wadname, const char *mapName);
	~ViewRenderer() {delete[] screenBuffer;}

	void render(uint8_t *pScreenBuffer, int iBufferPitch);

	void rotateBy(float dt)
	{
		view.angle += dt;
		view.angle -= M_PI * 2 * floorf(0.5 * view.angle * M_1_PI);
		view.cosa = cos(view.angle);
		view.sina = sin(view.angle);
	};

	void moveBy(float fwd, float side)
	{
		float dx = fwd * view.cosa + side * view.sina, dy = fwd * view.sina - side * view.cosa;
		if (doesLineIntersect(view.x, view.y, view.x + dx * 4, view.y + dy * 4)) return;
		view.x += dx;
		view.y += dy;
	};

	void updatePitch(float dp) { view.pitch = std::clamp(view.pitch - dp, -1.f, 1.f); }
	
	bool didLoadOK() const {return didload;}
protected:
	bool didload {false};
	void addWallInFOV(const Seg &seg);
	const Thing* getThing(int id) const { for (const Thing& t : things) if (t.type == id) return &t; return nullptr; }

	void updatePlayerSubSectorHeight()
	{
		int subsector = (int)(nodes.size() - 1);
		while (!(subsector & kSubsectorIdentifier)) subsector = isPointOnLeftSide(view, subsector) ? nodes[subsector].lChild : nodes[subsector].rChild;
		view.z = 41 + segs[subsectors[subsector & (~kSubsectorIdentifier)].firstSeg].rSector->floorHeight;
	}

	static constexpr uint16_t kSubsectorIdentifier = 0x8000; // Subsector Identifier is the 16th bit which indicate if the node ID is a subsector. The node ID is stored as uint16 0x8000

	void renderBSPNodes(int iNodeID, const Viewpoint &v)
	{
		if (iNodeID & kSubsectorIdentifier) // Masking all the bits exipt the last one to check if this is a subsector
		{
			iNodeID &= ~kSubsectorIdentifier;
			for (int i = 0; i < subsectors[iNodeID].numSegs; i++) addWallInFOV(segs[subsectors[iNodeID].firstSeg + i]);
			return;
		}
		const bool left = isPointOnLeftSide(v, iNodeID);
		renderBSPNodes(left ? nodes[iNodeID].lChild : nodes[iNodeID].rChild, v);
		renderBSPNodes(left ? nodes[iNodeID].rChild : nodes[iNodeID].lChild, v);
	}
	
	std::vector<const Linedef *> getBlock(int x, int y) const
	{
		if (y < blockmap_y || ((y - blockmap_y) >> 7) >= blockmap.size()) return {};
		if (x < blockmap_x || ((x - blockmap_x) >> 7) >= blockmap[0].size()) return {};
		return blockmap[(y - blockmap_y) >> 7][(x - blockmap_x) >> 7];
	}

	bool doesLineIntersect(int x1, int y1, int x2, int y2) const
	{
		std::vector<const Linedef *> tests = getBlock(x1, y1);
		if (x1 >> 7 != x2 >> 7 || y1 >> 7 != y2 >> 7)
		{
			std::vector<const Linedef *> tests2 = getBlock(x2, y2);
			tests.insert(tests.end(), tests2.begin(), tests2.end());
		}
		for (const Linedef *l : tests)	// Graphics Gems 3.
		{
			if (l->lSidedef && !(l->flags & 1)) continue;	// Could test for doors here.
			const float Ax = x2 - x1, Ay = y2 - y1, Bx = l->start.x - l->end.x, By = l->start.y - l->end.y, Cx = x1 - l->start.x, Cy = y1 - l->start.y;
			float den = Ay * Bx - Ax * By, tn = By * Cx - Bx * Cy;
			if (den > 0) { if (tn < 0 || tn > den) continue; } else if (tn > 0 || tn < den) continue;
			float un = Ax * Cy - Ay * Cx;
			if (den > 0) { if (un < 0 || un > den) continue; } else if (un > 0 || un < den) continue;
			return true;
		}
		// Test thing collisions here?
		return false;
	}
	
	bool isPointOnLeftSide(const Viewpoint &v, int node) const
	{
		return ((((v.x - nodes[node].x) * nodes[node].dy) - ((v.y - nodes[node].y) * nodes[node].dx)) <= 0);
	}
	
	struct Subsector { uint16_t numSegs, firstSeg; };
	struct BBox { int16_t top, bottom, left, right; };
	struct Node { int16_t x, y, dx, dy; BBox rBox, lBox; uint16_t rChild, lChild; };

	std::vector<Thing> things;
	std::vector<Sector> sectors;
	std::vector<Sidedef> sidedefs;
	std::vector<Linedef> linedefs;
	std::vector<Seg> segs;
	std::vector<Subsector> subsectors;
	std::vector<Node> nodes;
	
	std::vector<std::vector<std::vector<const Linedef *>>> blockmap;
	int16_t blockmap_x, blockmap_y;
	
    struct SolidSegmentRange { int start, end; };

    void storeWallRange(const Seg &seg, int x1, int x2, float ux1, float ux2, float z1, float z2);
	void addThing(const Thing &thing, const Seg &seg);

	int renderWidth, renderHeight, halfRenderWidth, halfRenderHeight, distancePlayerToScreen;
	float invRenderWidth, invRenderHeight;

	struct renderMark {int from, to; float zfrom, zto; };
	std::vector<std::vector<renderMark>> renderMarks;
	void mark(int x, int from, int to, float zfrom, float zto) { if (to >= from) renderMarks[x].push_back({from, to, zfrom, zto}); }
	struct renderLater {const Patch *patch; int column, from, to; float v, dv, z; const uint8_t *light;};
    std::list<SolidSegmentRange> solidWallRanges;
    std::vector<int> floorClipHeight;
    std::vector<int> ceilingClipHeight;
	std::vector<std::vector<renderLater>> renderLaters;
	uint8_t lights[34][256];
	int pal[256];
	uint8_t *screenBuffer;
	int rowlen, frame {0}, texframe {0};
	
	Viewpoint view;
	
	WADLoader wad;
	const Patch *weapon {nullptr};
};
