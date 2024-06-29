#pragma once

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
