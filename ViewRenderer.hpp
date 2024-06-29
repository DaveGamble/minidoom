#pragma once

#include <list>
#include <cstdint>
#include <vector>

enum { kUpperTextureUnpeg = 8, kLowerTextureUnpeg = 16 };
enum { thing_collectible = 1, thing_obstructs = 2, thing_hangs = 4, thing_artefact = 8 };

struct Patch
{
	Patch(const char *_name, const uint8_t *ptr);
	~Patch();

	uint16_t pixel(int x, int y) const {
		for ( ; cols[x].top != 0xff && cols[x].top <= y; x++) { int o = y - cols[x].top; if (o >= 0 && o < cols[x].length) return cols[x].data[o]; } return 256;
	}

	void render(uint8_t *buf, int rowlen, int screenx, int screeny, const uint8_t *lut, float scale = 1) const;
	void renderColumn(uint8_t *buf, int rowlen, int firstColumn, int maxHeight, int yOffset, float scale, const uint8_t *lut) const;
	void composeColumn(uint8_t *buf, int iHeight, int firstColumn, int yOffset) const;

	struct PatchColumnData { uint8_t top, length, paddingPre, *data, paddingPost; };
	const char *name; int height {0}, width {0}, xoffset {0}, yoffset {0}; std::vector<PatchColumnData> cols; std::vector<int> index;
};


struct Texture
{
	Texture(const char *_name, const uint8_t *ptr, class WADLoader *wad);
	const Patch *getPatchForColumn(int x, int &col, int &yOffset) const {
		if (!columns[x].overlap.size()) {col = columns[x].column; yOffset = columns[x].yOffset; return columns[x].patch;} return nullptr;
	}
	uint16_t pixel(int x, int y) const { x %= width; y %= height; if (x < 0) x += width; if (y < 0) y += height;
		return (columns[x].overlap.size()) ? columns[x].overlap[y] : columns[x].patch->pixel(columns[x].column, y - columns[x].yOffset);
	}
	const char *name; int width, height;
	struct TextureColumn { int column, yOffset; const Patch *patch; std::vector<uint8_t> overlap; };
	std::vector<TextureColumn> columns;
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
	static constexpr uint16_t kSubsectorIdentifier = 0x8000; // Subsector Identifier is the 16th bit which indicate if the node ID is a subsector. The node ID is stored as uint16 0x8000
	bool didload {false};
	void addWallInFOV(const Seg &seg);
	const Thing* getThing(int id) const { for (const Thing& t : things) if (t.type == id) return &t; return nullptr; }

	void updatePlayerSubSectorHeight();
	void renderBSPNodes(int iNodeID);
	
	std::vector<const Linedef *> getBlock(int x, int y) const;
	bool doesLineIntersect(int x1, int y1, int x2, int y2) const;
	bool isPointOnLeftSide(const Viewpoint &v, int node) const;

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
    std::vector<int> floorClipHeight, ceilingClipHeight;
	std::vector<std::vector<renderLater>> renderLaters;
	uint8_t lights[34][256];
	int pal[256];
	uint8_t *screenBuffer;
	int rowlen, frame {0}, texframe {0};
	
	Viewpoint view;
	
	WADLoader wad;
	const Patch *weapon {nullptr};
};
