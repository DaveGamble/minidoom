#include "Map.hpp"
#include "WADLoader.hpp"

Map::Map(const std::string &mapName, WADLoader &wad)
{
	int li = wad.findLumpByName(mapName);
	std::vector<uint8_t> data;
	const uint8_t *ptr;
	size_t size;
	auto seek = [&](const std::string& name) {
		data = wad.getLumpNamed(name, li);
		ptr = data.data();
		size = data.size();
		return ptr;
	};
	
	std::vector<Vertex> vertices;
	if (seek("VERTEXES")) for (int i = 0; i < size; i += sizeof(Vertex)) vertices.push_back(*(Vertex*)(ptr + i));

	struct WADSector { int16_t fh, ch; char floorTexture[8], ceilingTexture[8]; uint16_t lightlevel, type, tag; };
	if (seek("SECTORS")) for (int i = 0; i < size; i += sizeof(WADSector))
	{
		WADSector *ws = (WADSector*)(ptr + i);
		char floorname[9] {}, ceilname[9] {};
		memcpy(floorname, ws->floorTexture, 8);
		memcpy(ceilname, ws->ceilingTexture, 8);
//		if (!wad.getFlat(floorname).size()) printf("Didn't find flat [%s]\n", floorname);
//		if (!wad.getFlat(ceilname).size()) printf("Didn't find flat [%s]\n", ceilname);
		const Patch *sky = (!strncmp(ceilname, "F_SKY", 5)) ? wad.getPatch(ceilname + 2) : nullptr;
		sectors.push_back({ws->fh, ws->ch, wad.getFlat(floorname), wad.getFlat(ceilname), ws->lightlevel, ws->lightlevel, ws->lightlevel, ws->type, ws->tag, sky});
	}

	struct WADSidedef { int16_t dx, dy; char upperTexture[8], lowerTexture[8], middleTexture[8]; uint16_t sector; };
	if (seek("SIDEDEFS")) for (int i = 0; i < size; i += sizeof(WADSidedef))
	{
		WADSidedef *ws = (WADSidedef*)(ptr + i);
		char uname[9] {}, lname[9] {}, mname[9] {};
		memcpy(uname, ws->upperTexture, 8);
		memcpy(lname, ws->lowerTexture, 8);
		memcpy(mname, ws->middleTexture, 8);
//		if (!wad.getTexture(uname).size() && strcmp(uname, "-")) printf("Didn't find texture [%s]\n",uname);
//		if (!wad.getTexture(lname).size() && strcmp(lname, "-")) printf("Didn't find texture [%s]\n",lname);
//		if (!wad.getTexture(mname).size() && strcmp(mname, "-")) printf("Didn't find texture [%s]\n",mname);
		sidedefs.push_back({ws->dx, ws->dy, wad.getTexture(uname), wad.getTexture(mname), wad.getTexture(lname), sectors.data() + ws->sector});
	}

	struct WADLinedef { uint16_t start, end, flags, type, sectorTag, rSidedef, lSidedef; }; // Sidedef 0xFFFF means there is no sidedef
	if (seek("LINEDEFS")) for (int i = 0; i < size; i += sizeof(WADLinedef))
	{
		WADLinedef *wl = (WADLinedef*)(ptr + i);
		linedefs.push_back({vertices[wl->start], vertices[wl->end], wl->flags, wl->type, wl->sectorTag,
			(wl->rSidedef == 0xFFFF) ? nullptr : sidedefs.data() + wl->rSidedef,
			(wl->lSidedef == 0xFFFF) ? nullptr : sidedefs.data() + wl->lSidedef
		});
	}

	struct WADSeg { uint16_t start, end, slopeAngle, linedef, dir, offset; }; // Direction: 0 same as linedef, 1 opposite of linedef Offset: distance along linedef to start of seg
	if (seek("SEGS")) for (int i = 0; i < size; i += sizeof(WADSeg))
	{
		WADSeg *ws = (WADSeg*)(ptr + i);
		const Linedef *pLinedef = &linedefs[ws->linedef];
		const Sidedef *pRightSidedef = ws->dir ? pLinedef->lSidedef : pLinedef->rSidedef;
		const Sidedef *pLeftSidedef = ws->dir ? pLinedef->rSidedef : pLinedef->lSidedef;
		segs.push_back({vertices[ws->start], vertices[ws->end], (float)(ws->slopeAngle * M_PI * 2 / 65536.f), pLinedef, pRightSidedef, ws->dir, (float)ws->offset,
			sqrtf((vertices[ws->start].x - vertices[ws->end].x) * (vertices[ws->start].x - vertices[ws->end].x) + (vertices[ws->start].y - vertices[ws->end].y) * (vertices[ws->start].y - vertices[ws->end].y)),
			(pRightSidedef) ? pRightSidedef->sector : nullptr, (pLeftSidedef) ? pLeftSidedef->sector : nullptr});
	}
	if (seek("THINGS")) for (int i = 0; i < size; i += sizeof(Thing)) things.push_back(*(Thing*)(ptr + i));
	if (seek("NODES")) for (int i = 0; i < size; i += sizeof(Node)) nodes.push_back(*(Node*)(ptr + i));
	if (seek("SSECTORS")) for (int i = 0; i < size; i += sizeof(Subsector)) subsectors.push_back(*(Subsector*)(ptr + i));
	struct WADBlockmap { int16_t x, y; uint16_t numCols, numRows; };
	if (seek("BLOCKMAP"))
	{
		WADBlockmap *w = (WADBlockmap *)ptr;
		blockmap_x = w->x;
		blockmap_y = w->y;
		blockmap.resize(w->numRows);
		for (int i = 0; i < w->numRows; i++) blockmap[i].resize(w->numCols);
		const uint16_t *u = (const uint16_t *)ptr;
		const uint16_t *maps = u + 4;
		for (int i = 0; i < w->numRows; i++)
			for (int j = 0; j < w->numCols; j++)
			{
				const uint16_t *block = u + (*maps++) + 1;
				while (*block != 0xffff) blockmap[i][j].push_back(&linedefs[*block++]);
			}
	}
	
	auto addLinedef = [&](const Linedef &l, const Sidedef *s) { if (s && s->sector) (const_cast<Sector*>(s->sector))->linedefs.push_back(&l); };
	for (int i = 0; i < linedefs.size(); i++)
	{
		addLinedef(linedefs[i], linedefs[i].rSidedef);
		addLinedef(linedefs[i], linedefs[i].lSidedef);
	}
	for (Sector &s : sectors)
	{
		if (!s.type) continue;	// Skip these.
		uint16_t minlight = s.lightlevel;
		for (const Linedef *l : s.linedefs)
		{
			if (l->lSidedef && l->lSidedef->sector && l->lSidedef->sector != &s) minlight = std::min(minlight, l->lSidedef->sector->lightlevel);
			if (l->rSidedef && l->rSidedef->sector && l->rSidedef->sector != &s) minlight = std::min(minlight, l->rSidedef->sector->lightlevel);
		}
		s.minlightlevel = (minlight == s.lightlevel) ? 0 : minlight;
	}
}
