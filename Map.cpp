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
	struct WADThing { int16_t x, y; uint16_t angle, type, flags; };
	if (seek("THINGS")) for (int i = 0; i < size; i += sizeof(WADThing)) {WADThing *wt = (WADThing*)(ptr + i); things.push_back({wt->x, wt->y, wt->angle, wt->type, wt->flags});}
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
	
	
	
	struct thinginfo { int id; char name[5], anim[7]; int flags; };
	enum { thing_collectible = 1, thing_obstructs = 2, thing_hangs = 4, thing_artefact = 8 };
	static std::vector<thinginfo> thinginfos = {	// UDS 1.666 list of things.
	{    5, "BKEY", "ab    ", thing_collectible },
	{    6, "YKEY", "ab    ", thing_collectible },
	{    7, "SPID", "+     ", thing_obstructs },
	{    8, "BPAK", "a     ", thing_collectible },
	{    9, "SPOS", "+     ", thing_obstructs },
	{   10, "PLAY", "w     ", 0 },
	{   12, "PLAY", "w     ", 0 },
	{   13, "RKEY", "ab    ", thing_collectible },
	{   15, "PLAY", "n     ", 0 },
	{   16, "CYBR", "+     ", thing_obstructs },
	{   17, "CELP", "a     ", thing_collectible },
	{   18, "POSS", "l     ", 0 },
	{   19, "SPOS", "l     ", 0 },
	{   20, "TROO", "m     ", 0 },
	{   21, "SARG", "n     ", 0 },
	{   22, "HEAD", "l     ", 0 },
	{   23, "SKUL", "k     ", 0 },
	{   24, "POL5", "a     ", 0 },
	{   25, "POL1", "a     ", thing_obstructs },
	{   26, "POL6", "ab    ", thing_obstructs },
	{   27, "POL4", "a     ", thing_obstructs },
	{   28, "POL2", "a     ", thing_obstructs },
	{   29, "POL3", "ab    ", thing_obstructs },
	{   30, "COL1", "a     ", thing_obstructs },
	{   31, "COL2", "a     ", thing_obstructs },
	{   32, "COL3", "a     ", thing_obstructs },
	{   33, "COL4", "a     ", thing_obstructs },
	{   34, "CAND", "a     ", 0 },
	{   35, "CBRA", "a     ", thing_obstructs },
	{   36, "COL5", "ab    ", thing_obstructs },
	{   37, "COL6", "a     ", thing_obstructs },
	{   38, "RSKU", "ab    ", thing_collectible },
	{   39, "YSKU", "ab    ", thing_collectible },
	{   40, "BSKU", "ab    ", thing_collectible },
	{   41, "CEYE", "abcb  ", thing_obstructs },
	{   42, "FSKU", "abc   ", thing_obstructs },
	{   43, "TRE1", "a     ", thing_obstructs },
	{   44, "TBLU", "abcd  ", thing_obstructs },
	{   45, "TGRE", "abcd  ", thing_obstructs },
	{   46, "TRED", "abcd  ", thing_obstructs },
	{   47, "SMIT", "a     ", thing_obstructs },
	{   48, "ELEC", "a     ", thing_obstructs },
	{   49, "GOR1", "abcb  ", thing_hangs | thing_obstructs },
	{   50, "GOR2", "a     ", thing_hangs | thing_obstructs},
	{   51, "GOR3", "a     ", thing_hangs | thing_obstructs},
	{   52, "GOR4", "a     ", thing_hangs | thing_obstructs},
	{   53, "GOR5", "a     ", thing_hangs | thing_obstructs},
	{   54, "TRE2", "a     ", thing_obstructs },
	{   55, "SMBT", "abcd  ", thing_obstructs },
	{   56, "SMGT", "abcd  ", thing_obstructs },
	{   57, "SMRT", "abcd  ", thing_obstructs },
	{   58, "SARG", "+     ", thing_obstructs },
	{   59, "GOR2", "a     ", thing_hangs },
	{   60, "GOR4", "a     ", thing_hangs },
	{   61, "GOR3", "a     ", thing_hangs },
	{   62, "GOR5", "a     ", thing_hangs },
	{   63, "GOR1", "abcb  ", thing_hangs },
	{   64, "VILE", "+     ", thing_obstructs },
	{   65, "CPOS", "+     ", thing_obstructs },
	{   66, "SKEL", "+     ", thing_obstructs },
	{   67, "FATT", "+     ", thing_obstructs },
	{   68, "BSPI", "+     ", thing_obstructs },
	{   69, "BOS2", "+     ", thing_obstructs },
	{   70, "FCAN", "abc   ", thing_obstructs },
	{   71, "PAIN", "+     ", thing_hangs | thing_obstructs},
	{   72, "KEEN", "a     ", thing_obstructs },	// taken out + here
	{   73, "HDB1", "a     ", thing_hangs | thing_obstructs},
	{   74, "HDB2", "a     ", thing_hangs | thing_obstructs},
	{   75, "HDB3", "a     ", thing_hangs | thing_obstructs},
	{   76, "HDB4", "a     ", thing_hangs | thing_obstructs},
	{   77, "HDB5", "a     ", thing_hangs | thing_obstructs},
	{   78, "HDB6", "a     ", thing_hangs | thing_obstructs},
	{   79, "POB1", "a     ", 0 },
	{   80, "POB2", "a     ", 0 },
	{   81, "BRS1", "a     ", 0 },
	{   82, "SGN2", "a     ", thing_collectible },
	{   83, "MEGA", "abcd  ", thing_artefact },
	{   84, "SSWV", "+     ", thing_obstructs },
	{   85, "TLMP", "abcd  ", thing_obstructs },
	{   86, "TLP2", "abcd  ", thing_obstructs },
	{   88, "BBRN", "+     ", thing_obstructs },
	{ 2001, "SHOT", "a     ", thing_collectible },
	{ 2002, "MGUN", "a     ", thing_collectible },
	{ 2003, "LAUN", "a     ", thing_collectible },
	{ 2004, "PLAS", "a     ", thing_collectible },
	{ 2005, "CSAW", "a     ", thing_collectible },
	{ 2006, "BFUG", "a     ", thing_collectible },
	{ 2007, "CLIP", "a     ", thing_collectible },
	{ 2008, "SHEL", "a     ", thing_collectible },
	{ 2010, "ROCK", "a     ", thing_collectible },
	{ 2011, "STIM", "a     ", thing_collectible },
	{ 2012, "MEDI", "a     ", thing_collectible },
	{ 2013, "SOUL", "abcdcb", thing_artefact },
	{ 2014, "BON1", "abcdcb", thing_artefact },
	{ 2015, "BON2", "abcdcb", thing_artefact },
	{ 2018, "ARM1", "ab    ", thing_collectible },
	{ 2019, "ARM2", "ab    ", thing_collectible },
	{ 2022, "PINV", "abcd  ", thing_artefact },
	{ 2023, "PSTR", "a     ", thing_artefact },
	{ 2024, "PINS", "abcd  ", thing_artefact },
	{ 2025, "SUIT", "a     ", thing_artefact },
	{ 2026, "PMAP", "abcdcb", thing_artefact },
	{ 2028, "COLU", "a     ", thing_obstructs },
	{ 2035, "BAR1", "ab   ", thing_obstructs },	// taken out the + here
	{ 2045, "PVIS", "ab    ", thing_artefact },
	{ 2046, "BROK", "a     ", thing_collectible },
	{ 2047, "CELL", "a     ", thing_collectible },
	{ 2048, "AMMO", "a     ", thing_collectible },
	{ 2049, "SBOX", "a     ", thing_collectible },
	{ 3001, "TROO", "+     ", thing_obstructs },
	{ 3002, "SARG", "+     ", thing_obstructs },
	{ 3003, "BOSS", "+     ", thing_obstructs },
	{ 3004, "POSS", "+     ", thing_obstructs },
	{ 3005, "HEAD", "+     ", thing_hangs | thing_obstructs},
	{ 3006, "SKUL", "+     ", thing_hangs | thing_obstructs}};
	
	for (Thing& t : things)
	{
		auto info = std::lower_bound(thinginfos.begin(), thinginfos.end(), t.type, [] (const thinginfo &a, const int &b) { return a.id < b; });
		if (info == thinginfos.end()) {printf("Unknown thing, id %d", t.type); continue;}
		const char *basename = info->name;
		t.attr = info->flags;
		
		for (const char *anim = info->anim; *anim && *anim != ' '; anim++)
		{
			if (*anim == '+') t.imgs = wad.getPatchesStartingWith(basename);
			else
			{
				char buffer[9] {};
				snprintf(buffer, 9, "%s%c", basename, toupper(*anim));				
				t.imgs.push_back(wad.getPatch(buffer));
			}
		}
	}
}
