#pragma once

#include <cstdint>
#include <list>
#include <vector>

struct Patch
{
	Patch(const char *_name, const uint8_t *ptr);

	uint16_t pixel(int x, int y) const { for (const colData &c : cols[x]) if (y >= c.top && y < c.top + c.length) return c.data[y - c.top]; return 256; }

	void render(uint8_t *buf, int rowlen, int screenx, int screeny, const uint8_t *lut, float scale = 1) const;
	void composeColumn(uint8_t *buf, int iHeight, int firstColumn, int yOffset) const;

	struct colData { uint8_t top, length, *data; };
	const char *name; int height {0}, width {0}, xoffset {0}, yoffset {0}; std::vector<std::vector<colData>> cols; std::vector<uint8_t> pixels;
};

struct Texture
{
	Texture(const char *_name, const uint8_t *ptr, class WADLoader *wad);
	const Patch *getPatchForColumn(int x, int &col, int &yOffset) const {
		if (!cols[x].overlap.size()) {col = cols[x].x; yOffset = cols[x].dy; return cols[x].patch;} return nullptr;
	}
	uint16_t pixel(int x, int y) const { x %= width; y %= height; if (x < 0) x += width; if (y < 0) y += height;
		return (cols[x].overlap.size()) ? cols[x].overlap[y] : cols[x].patch->pixel(cols[x].x, y - cols[x].dy);
	}
	const char *name; int width, height;
	struct colData { int x, dy; const Patch *patch; std::vector<uint8_t> overlap; };
	std::vector<colData> cols;
};

struct Flat
{
	Flat(const char *_name, const uint8_t *_data) : name(_name) { memcpy(data, _data, 4096); }
	uint8_t pixel(int u, int v) const { return data[(64 * v + (u & 63)) & 4095]; }
	const char *name; uint8_t data[4096];
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
	WADLoader(const char *filename);
	~WADLoader();
	bool didLoad() const { return data != nullptr; }
	void release();
	std::vector<uint8_t> getLumpNamed(const char *name, size_t after = 0) const;
	int findLumpByName(const char *lumpName, size_t after = 0) const;
	const std::vector<const char*> getPatchesStartingWith(const char *name);
	const Patch *getPatch(int idx) { return getPatch(pnames[idx]); }
	const Patch *getPatch(const char *name);
	std::vector<const Texture *> getTexture(const char *name) const;
	std::vector<const Flat *> getFlat(const char *name) const;
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
	std::vector<const char *> pnames;
};

class ViewRenderer
{
public:
    ViewRenderer(int renderXSize, int renderYSize, const char *wadname, const char *mapName);
	~ViewRenderer();

	void render(uint8_t *pScreenBuffer, int iBufferPitch);
	void rotateBy(float dt);
	void moveBy(float fwd, float side);
	void updatePitch(float dp);
	bool didLoadOK() const {return didload;}

protected:
	struct Subsector { uint16_t numSegs, firstSeg; };
	struct BBox { int16_t top, bottom, left, right; };
	struct Node { int16_t x, y, dx, dy; BBox rBox, lBox; uint16_t rChild, lChild; };
	struct SolidSegmentRange { int start, end; };
	struct renderMark {int from, to; float zfrom, zto; };
	struct renderLater {const Patch *patch; int column, from, to; float v, dv, z; const uint8_t *light;};

	static constexpr uint16_t kSubsectorIdentifier = 0x8000; // Subsector Identifier is the 16th bit which indicate if the node ID is a subsector. The node ID is stored as uint16 0x8000
	enum { kUpperTextureUnpeg = 8, kLowerTextureUnpeg = 16 };
	enum { thing_collectible = 1, thing_obstructs = 2, thing_hangs = 4, thing_artefact = 8 };

	const Thing* getThing(int id) const { for (const Thing& t : things) if (t.type == id) return &t; return nullptr; }
	void updatePlayerSubSectorHeight();
	
	std::vector<const Linedef *> getBlock(int x, int y) const;
	bool doesLineIntersect(int x1, int y1, int x2, int y2) const;
	bool isPointOnLeftSide(const Viewpoint &v, int node) const;
	
	void renderBSPNodes(int iNodeID);
    void storeWallRange(const Seg &seg, int x1, int x2, float ux1, float ux2, float z1, float z2);
	void addWallInFOV(const Seg &seg);
	void addThing(const Thing &thing, const Seg &seg);
	void mark(int x, int from, int to, float zfrom, float zto) { if (to >= from) renderMarks[x].push_back({from, to, zfrom, zto}); }
	
	template<typename F> static F clamp(const F &val, const F &min, const F &max) { return (val < min) ? min : (val > max) ? max : val; }
	bool didload {false};
	// Map:
	std::vector<Thing> things;
	std::vector<Sector> sectors;
	std::vector<Sidedef> sidedefs;
	std::vector<Linedef> linedefs;
	std::vector<Seg> segs;
	std::vector<Subsector> subsectors;
	std::vector<Node> nodes;
	std::vector<std::vector<std::vector<const Linedef *>>> blockmap;
	int16_t blockmap_x, blockmap_y;
	WADLoader wad;
	// Render:
	int renderWidth, renderHeight, halfRenderWidth, halfRenderHeight, distancePlayerToScreen;
	float invRenderWidth, invRenderHeight;
	std::vector<std::vector<renderMark>> renderMarks;
    std::list<SolidSegmentRange> solidWallRanges;
    std::vector<int> floorClipHeight, ceilingClipHeight;
	std::vector<std::vector<renderLater>> renderLaters;
	uint8_t lights[34][256], *screenBuffer;
	int rowlen, frame {0}, texframe {0}, pal[256];
	Viewpoint view;
	const Patch *weapon {nullptr};
};
