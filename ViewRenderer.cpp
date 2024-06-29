#include "ViewRenderer.hpp"

constexpr float light_depth = 0.025, sector_light_scale = -0.125, light_offset = 15;

ViewRenderer::ViewRenderer(int renderXSize, int renderYSize, const char *wadname, const char *mapName)
: renderWidth(renderXSize)
, invRenderWidth(1.f / renderXSize)
, renderHeight(renderYSize)
, invRenderHeight(1.f / renderYSize)
, halfRenderWidth(renderXSize / 2)
, halfRenderHeight(renderYSize / 2)
, distancePlayerToScreen(halfRenderWidth)	// 90 here is FOV
, ceilingClipHeight(renderWidth)
, floorClipHeight(renderWidth)
, renderLaters(renderWidth)
, renderMarks(renderWidth)
, wad(wadname)
{
	std::vector<uint8_t> ll = wad.getLumpNamed("COLORMAP");
	for (int i = 0; i < 34; i++) memcpy(lights[i], ll.data() + 256 * i, 256);

	int li = wad.findLumpByName(mapName);
	std::vector<uint8_t> data;
	const uint8_t *ptr;
	size_t size;
	auto seek = [&](const char *name) {
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
		Linedef *pLinedef = &linedefs[ws->linedef];
		Sidedef *pRightSidedef = ws->dir ? pLinedef->lSidedef : pLinedef->rSidedef;
		Sidedef *pLeftSidedef = ws->dir ? pLinedef->rSidedef : pLinedef->lSidedef;
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
	static std::vector<thinginfo> thinginfos = {	// UDS 1.666 list of things.
	{    5, "BKEY", "ab    ", thing_collectible },
	{    6, "YKEY", "ab    ", thing_collectible },
	{    7, "SPID", "+     ", thing_obstructs },
	{    8, "BPAK", "a     ", thing_collectible },
	{    9, "SPOS", "+     ", thing_obstructs },
	//	{   10, "PLAY", "w     ", 0 },
	//	{   12, "PLAY", "w     ", 0 },
	{   13, "RKEY", "ab    ", thing_collectible },
	//	{   15, "PLAY", "n     ", 0 },
	{   16, "CYBR", "+     ", thing_obstructs },
	{   17, "CELP", "a     ", thing_collectible },
	//	{   18, "POSS", "l     ", 0 },
	//	{   19, "SPOS", "l     ", 0 },
	//	{   20, "TROO", "m     ", 0 },
	//	{   21, "SARG", "n     ", 0 },
	//	{   22, "HEAD", "l     ", 0 },
	//	{   23, "SKUL", "k     ", 0 },
	//	{   24, "POL5", "a     ", 0 },
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
	//	{   79, "POB1", "a     ", 0 },
	//	{   80, "POB2", "a     ", 0 },
	//	{   81, "BRS1", "a     ", 0 },
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
	{ 2035, "BAR1", "ab    ", thing_obstructs },	// taken out the + here
	{ 2045, "PVIS", "ab    ", thing_artefact },
	{ 2046, "BROK", "a     ", thing_collectible },
	{ 2047, "CELL", "a     ", thing_collectible },
	{ 2048, "AMMO", "a     ", thing_collectible },
	{ 2049, "SBOX", "a     ", thing_collectible },
	//	{ 3001, "TROO", "+     ", thing_obstructs },
	//	{ 3002, "SARG", "+     ", thing_obstructs },
	//	{ 3003, "BOSS", "+     ", thing_obstructs },
	//	{ 3004, "POSS", "+     ", thing_obstructs },
	//	{ 3005, "HEAD", "+     ", thing_hangs | thing_obstructs},
	//	{ 3006, "SKUL", "+     ", thing_hangs | thing_obstructs}
};
	
	things.erase(std::remove_if(things.begin(), things.end(), [] (const Thing& t) { return !(t.flags & 2); }), things.end());
	things.erase(std::remove_if(things.begin(), things.end(), [] (const Thing& t) { return (t.flags & 16); }), things.end());

	for (Thing& t : things)
	{
		if (t.type == 11 || (t.type >= 1 && t.type <=4)) continue;// Player start positions.
		auto info = std::lower_bound(thinginfos.begin(), thinginfos.end(), t.type, [] (const thinginfo &a, const int &b) { return a.id < b; });
		
		if (info == thinginfos.end() || info->id != t.type) {printf("Unknown thing, id %d\n", t.type); continue;}
		const char *basename = info->name;
		t.attr = info->flags;
		
		for (const char *anim = info->anim; *anim && *anim != ' '; anim++)
		{
			if (*anim == '+')
			{
				std::vector<const char*> all = wad.getPatchesStartingWith(basename);
				for (const char *s : all) t.imgs.push_back(wad.getPatch(s));
			}
			else
			{
				char buffer[9] {};
				snprintf(buffer, 9, "%s%c0", basename, toupper(*anim));
				t.imgs.push_back(wad.getPatch(buffer));
			}
		}
		Viewpoint v {t.x, t.y};
		int subsector = (int)(nodes.size() - 1);
		while (!(subsector & kSubsectorIdentifier)) subsector = isPointOnLeftSide(v, subsector) ? nodes[subsector].lChild : nodes[subsector].rChild;
		segs[subsectors[subsector & (~kSubsectorIdentifier)].firstSeg].rSector->things.push_back(&t);
	}
	
	const Thing* t = getThing(1);
	if (t)
	{
		view.x = t->x;
		view.y = t->y;
		view.angle = t->angle * M_PI / 180;
		view.cosa = cos(view.angle);
		view.sina = sin(view.angle);
	}
	view.z = 41;
	view.pitch = 0;
}


void ViewRenderer::render(uint8_t *pScreenBuffer, int iBufferPitch)
{
	frame++;
	texframe = frame / 20;
	screenBuffer = pScreenBuffer;
	rowlen = iBufferPitch;
	solidWallRanges.clear();
	solidWallRanges.push_back({INT_MIN, -1});
	solidWallRanges.push_back({renderWidth, INT_MAX});
	std::fill(ceilingClipHeight.begin(), ceilingClipHeight.end(), -1);
	std::fill(floorClipHeight.begin(), floorClipHeight.end(), renderHeight);

	for (Sector &sec : sectors)
	{
		sec.thingsThisFrame = false;
		if (!sec.type) continue;
		if (sec.type == 2 || sec.type == 4 || sec.type == 12) sec.lightlevel = ((frame % 60) < 30) ? sec.maxlightlevel : sec.minlightlevel;
		if (sec.type == 3 || sec.type == 13) sec.lightlevel = ((frame % 120) < 60) ? sec.maxlightlevel : sec.minlightlevel;
		if (sec.type == 1) sec.lightlevel = (rand() & 1) ? sec.maxlightlevel : sec.minlightlevel;
	}
	renderBSPNodes((int)nodes.size() - 1, view);
	
	const float horizon = halfRenderHeight + view.pitch * halfRenderHeight;
	for (int x = 0; x < renderWidth; x++)
	{
		if (!renderLaters[x].size())  {renderMarks[x].clear(); continue;}
		std::sort(renderMarks[x].begin(), renderMarks[x].end(), [] (const renderMark &a, const renderMark &b) {return a.zfrom + a.zto < b.zfrom + b.zto; });
		std::sort(renderLaters[x].begin(), renderLaters[x].end(), [] (const renderLater &a, const renderLater &b) {return a.z < b.z; });
		for (int i = (int)renderLaters[x].size() - 1; i >= 0; i--)
		{
			renderLater& r = renderLaters[x][i];
			int from = std::max(0, r.from), to = std::min(r.to, renderHeight);
			for (int c = 0; to > from && c < renderMarks[x].size() && std::min(renderMarks[x][c].zfrom, renderMarks[x][c].zto) < r.z; c++)
			{
				const renderMark &m = renderMarks[x][c];
				if (m.to <= from || m.from > to) continue;
				if (m.zfrom == m.zto || std::max(m.zfrom, m.zto) < r.z)	// column clip or completely before.
				{
					if (m.from <= from) from = std::max(from, m.to);
					else to = std::min(to, m.from);
				}
				else
				{
					const float intersection =  horizon + m.zfrom * (m.from - horizon) / r.z;
					if (m.zfrom < r.z) from = std::max(from, (int)intersection);
					else  to = std::min(to, (int)intersection);
				}
			}
			float v = r.v + (from - r.from) * r.dv;
			for (int y = from; y < to; y++, v += r.dv)
			{
				uint16_t p = r.patch->pixel(r.column, v);
				if (p != 256) screenBuffer[rowlen * y + x] = r.light[p];
			}
		}
		renderLaters[x].clear();
		renderMarks[x].clear();
	}
}

void ViewRenderer::addThing(const Thing &thing, const Seg &seg)
{
//	printf("Add thing %d at %d %d\n", thing.type, thing.x, thing.y);
	const int toV1x = thing.x - view.x, toV1y = thing.y - view.y;	// Vectors from origin to segment ends.
	const float ca = view.cosa, sa = view.sina, tz = toV1x * ca + toV1y * sa, tx = toV1x * sa - toV1y * ca;	// Rotate vectors to be in front of us.

	if (tz < 0) return;

	int light = std::clamp(light_offset + seg.rSector->lightlevel * sector_light_scale + tz * light_depth, 0.f, 31.f);

	const int xc = distancePlayerToScreen + round(tx * halfRenderWidth / tz);
	const float horizon = halfRenderHeight + view.pitch * halfRenderHeight;
	const float vG = distancePlayerToScreen * (view.z - seg.rSector->floorHeight), vH = distancePlayerToScreen * (seg.rSector->ceilingHeight - view.z);
	
	const Patch *patch = thing.imgs[texframe % thing.imgs.size()];
	if (!patch) return;

	float y1, y2, height = patch->getHeight();
	if (thing.attr & thing_hangs) {y1 = horizon - vH / tz; y2 = horizon - distancePlayerToScreen * (seg.rSector->ceilingHeight + height - view.z) / tz;}
	else {y2 = vG / tz + horizon; y1 = horizon + distancePlayerToScreen * (view.z - seg.rSector->floorHeight - height) / tz;}
	float dv = height / (y2 - y1);
	float py1 = y1, py2 = y2;
	const float scale = 0.5 * patch->getWidth() * (y2 - y1) / height; // 16 / z;

	if (xc + scale < 0 || xc - scale >= renderWidth) return;

	for (int x1 = xc - scale; x1 < xc + scale; x1++)
	{
		if (x1 < 0 || x1 >= renderWidth) continue;
		float vx = patch->getWidth() * (x1 - xc + scale) / (scale * 2);
		if (vx < 0 || vx >= patch->getWidth()) continue;
		renderLaters[x1].push_back({patch, patch->getColumnDataIndex(vx), (int)py1, (int)py2, 0, dv, tz, lights[light]});
	}
}

void ViewRenderer::addWallInFOV(const Seg &seg)
{
	if (seg.rSector && !seg.rSector->thingsThisFrame)
	{
		(const_cast<Seg&>(seg)).rSector->thingsThisFrame = true;	// Mark it done.
		
		for (int i = 0; i < seg.rSector->things.size(); i++) addThing(*seg.rSector->things[i], seg);
	}
	
	const int toV1x = seg.start.x - view.x, toV1y = seg.start.y - view.y, toV2x = seg.end.x - view.x, toV2y = seg.end.y - view.y;	// Vectors from origin to segment ends.
	if (toV1x * toV2y >= toV1y * toV2x) return;	// If sin(angle) between the two (computed as dot product of V1 and normal to V2) is +ve, wall is out of view. (It's behind us)

	const float ca = view.cosa, sa = view.sina;
	const float tov1z = toV1x * ca + toV1y * sa, tov2z = toV2x * ca + toV2y * sa;
	const float tov1x = toV1x * sa - toV1y * ca, tov2x = toV2x * sa - toV2y * ca;	// Rotate vectors to be in front of us.
	// z = how far in front of us it is. -ve values are behind. +ve values are in front.
	// x = position left to right. left = -1, right = 1.

	if (tov1z < tov1x && tov2z < -tov2x) return;	// V1 is right of FOV and V2 is left of FOV
	if (tov1z < -tov1x && tov2z < -tov2x) return;	// Both points are to the left.
	if (tov1z < tov1x && tov1z > -tov1x) return;	// V1 is within 45 degrees of the X axis (it's on your right hand side, out of the FOV). Both points are to the right.

	const int x1 = (tov1z < -tov1x) ? 0 : distancePlayerToScreen + round(tov1x * halfRenderWidth / tov1z);
	const int x2 = (tov2z < tov2x) ? renderWidth : distancePlayerToScreen + round(tov2x * halfRenderWidth / tov2z);

    if (x1 == x2) return; // Skip same pixel wall
	bool solid =  (!seg.lSector || seg.lSector->ceilingHeight <= seg.rSector->floorHeight || seg.lSector->floorHeight >= seg.rSector->ceilingHeight); // Handle walls and closed door

	if (!solid && (seg.lSector->sky && seg.lSector->sky == seg.rSector->sky) && !seg.sidedef->middletexture.size()
		&& seg.rSector->floorHeight == seg.lSector->floorHeight
		&& seg.lSector->floortexture == seg.rSector->floortexture && seg.lSector->lightlevel == seg.rSector->lightlevel) return;
	
	if (solid && solidWallRanges.size() < 2) return;

    auto f = solidWallRanges.begin(); while (x1 - 1 > f->end) ++f;

    if (x1 < f->start)
    {
        if (x2 < f->start - 1)
        {
            storeWallRange(seg, x1, x2, tov1x, tov2x, tov1z, tov2z); //All of the wall is visible, so insert it
            if (solid) solidWallRanges.insert(f, {x1, x2});
            return;
        }
        storeWallRange(seg, x1, f->start - 1, tov1x, tov2x, tov1z, tov2z); // The end is already included, just update start
        if (solid) f->start = x1;
    }
    
    if (x2 <= f->end) return; // This part is already occupied
    std::list<SolidSegmentRange>::iterator nextWall = f;
    while (x2 >= next(nextWall, 1)->start - 1)
    {
        storeWallRange(seg, nextWall->end + 1, next(nextWall, 1)->start - 1, tov1x, tov2x, tov1z, tov2z); // partialy clipped by other walls, store each fragment
		if (x2 > (++nextWall)->end) continue;
		if (!solid) return;
		f->end = nextWall->end;
		solidWallRanges.erase(++f, ++nextWall);
		return;
    }
    storeWallRange(seg, nextWall->end + 1, x2, tov1x, tov2x, tov1z, tov2z);
	if (!solid) return;
	f->end = x2;
	if (nextWall != f) solidWallRanges.erase(++f, ++nextWall);
}

void ViewRenderer::storeWallRange(const Seg &seg, int x1, int x2, float ux1, float ux2, float z1, float z2)
{
	const int16_t flags = seg.linedef->flags;
	const float roomHeight = seg.rSector->ceilingHeight - seg.rSector->floorHeight;
	const float lrFloor = seg.lSector ? seg.lSector->floorHeight - seg.rSector->floorHeight : 0;
	const float rlCeiling = seg.lSector ? seg.rSector->ceilingHeight - seg.lSector->ceilingHeight : 0;
	const float tdX = seg.sidedef->dx + seg.offset, tdY = seg.sidedef->dy;
	const float sinv = view.sina, cosv = view.cosa;
	const float seglen = seg.len;
	const float distanceToNormal = (z1 * ux2 - ux1 * z2);
	const float idistanceToNormal = 1.0 / distanceToNormal;	// Distance from origin to rotated wall segment
	const float zscalar = -distancePlayerToScreen * distanceToNormal;
	const float uA = distancePlayerToScreen * (ux1 + z1) * seglen, uB = -z1 * seglen, uC = -distancePlayerToScreen * (ux2 - ux1 + z2 - z1), uD = z2 - z1;
	const float dx = uD * idistanceToNormal, x1z = uC * idistanceToNormal + x1 * dx;

	const float vG = distancePlayerToScreen * (view.z - seg.rSector->floorHeight), vH = distancePlayerToScreen * (seg.rSector->ceilingHeight - view.z);
	const float vA = cosv - sinv, vB = 2 * sinv * invRenderWidth, vC = view.x - 1, vD = -cosv - sinv, vE = 2 * cosv * invRenderWidth, vF = -view.y;

	const float dyCeiling = (seg.rSector->ceilingHeight - view.z) * dx;
	const float dyFloor = (seg.rSector->floorHeight - view.z) * dx;
	const float dyUpper = seg.lSector ? (seg.lSector->ceilingHeight - view.z) * dx : 0;
	const float dyLower = seg.lSector ? (seg.lSector->floorHeight - view.z) * dx : 0;
	const float dSkyAng = invRenderWidth;

	const float horizon = halfRenderHeight + view.pitch * halfRenderHeight;

	float yCeiling = horizon + (seg.rSector->ceilingHeight - view.z) * x1z;
	float yFloor = horizon + (seg.rSector->floorHeight - view.z) * x1z;
	float yUpper = seg.lSector ? horizon + (seg.lSector->ceilingHeight - view.z) * x1z : 0;
	float yLower = seg.lSector ? horizon + (seg.lSector->floorHeight - view.z) * x1z : 0;
	float skyAng = x1 * dSkyAng - 2 * view.angle / M_PI;
	
	for (int x = x1; x <= x2; x++)
    {
		const float iz = 1.f / (uC + x * uD);
		const float z = zscalar * iz;
		const float u = (uA + x * uB) * iz + tdX;
		int light = light_offset + seg.rSector->lightlevel * sector_light_scale +  z * light_depth;
		const uint8_t *lut = lights[std::clamp(light, 0, 31)];

		auto DrawTexture = [&](const std::vector<const Texture *> &textures, int from, int to, float a, float b, float dv, int stage) {
			if (!textures.size()) return;
			const Texture *texture = textures[texframe % textures.size()];
			if (!texture || to <= from) return;
			dv /= (b - a);
			float v = -a * dv;
			if (stage == 0 && !(flags & kUpperTextureUnpeg)) v = -b * dv;// top
			if (stage == 1 && (flags & kLowerTextureUnpeg))  v = -yFloor * dv; // middle
			if (stage == 2 && (flags & kLowerTextureUnpeg)) v = -yCeiling * dv; // bottom
			v += tdY;
			for (int y = from; y < to; y++) { screenBuffer[rowlen * y + x] = lut[texture->pixel(u, v + y * dv) & 255]; }
			mark(x, from, to, z, z);
		};

		auto DrawSky = [&](const Patch *sky, int from, int to) {
			int tx = (skyAng - floor(skyAng)) * sky->getWidth();
			if (tx == sky->getWidth()) tx = 0;
			for (int i = std::max(0, from); i < std::min(to, renderHeight); i++)
			{
				float ty = std::clamp((i - horizon + halfRenderHeight) * sky->getHeight() * invRenderHeight, -1.f, sky->getHeight() - 1.f);
				screenBuffer[rowlen * i + x] = lights[0][sky->pixel(sky->getColumnDataIndex((ty < 0) ? 0 : tx), std::max(ty, 0.f))];
			}
		};

		auto DrawFloor = [&](const std::vector<const Flat *> &flats, int from, int to) {
			if (vG < 0) return;
			const Flat *flat = flats[texframe % flats.size()];
			from = std::max((int)horizon + 1, from);
			to = std::min(to, renderHeight);
			for (int i = from; i < to; i++)
			{
				float z = vG / (i - horizon);
				int light = light_offset + seg.rSector->lightlevel * sector_light_scale + z * light_depth;
				screenBuffer[i * rowlen + x] = lights[std::clamp(light, 0, 31)][flat->pixel(z * (vA + vB * x) + vC, z * (vD + vE * x) + vF)];
			}
			mark(x, from, to, vG / (from - horizon), vG / (to - horizon));
		};

		auto DrawCeiling = [&](const std::vector<const Flat *> &flats, int from, int to) {
			if (seg.rSector->sky)	DrawSky(seg.rSector->sky, from, to);
			else
			{
				if (vH < 0) return;
				const Flat *flat = flats[texframe % flats.size()];
				from = std::max(0, from);
				to = std::min(to, (int)horizon - 1);
				for (int i = from; i < to; i++)
				{
					float z = vH / (horizon - i);
					int light = light_offset + seg.rSector->lightlevel * sector_light_scale + z * light_depth;
					screenBuffer[i * rowlen + x] = lights[std::clamp(light, 0, 31)][flat->pixel(z * (vA + vB * x) + vC, z * (vD + vE * x) + vF)];
				}
				mark(x, from, to, vH / (horizon - from), vH / (horizon - to));
			}
		};
				
		int CurrentCeilingEnd = std::max(yCeiling, ceilingClipHeight[x] + 1.f), CurrentFloorStart = std::min(yFloor, floorClipHeight[x] - 1.f);
		int ceilbot = std::min(CurrentCeilingEnd, CurrentFloorStart), floortop = std::max(CurrentFloorStart, ceilingClipHeight[x]);
		int midtop = std::max(std::max(0, ceilbot), ceilingClipHeight[x]), midbot = std::min(floortop, renderHeight - 1);
		
		DrawFloor(seg.rSector->floortexture, floortop, floorClipHeight[x]);
		DrawCeiling(seg.rSector->ceilingtexture, std::max(0, ceilingClipHeight[x]), ceilbot);

        if (seg.lSector)
        {
			int upper = std::min((float)CurrentFloorStart, yUpper), lower = std::max(yLower, ceilingClipHeight[x] + 1.f);
			if (seg.sidedef->middletexture.size() && midtop < midbot && yFloor > yCeiling)
			{
				int top = std::max(std::max(upper, ceilbot), 0), bot = std::min(std::min(lower, floortop), renderHeight);
				const Texture *tex = seg.sidedef->middletexture[texframe % seg.sidedef->middletexture.size()];
				float dv = z * invRenderWidth * 2;
				float v = -std::max(yUpper, yCeiling) * dv;
				if (flags & kLowerTextureUnpeg)  v = tex->getHeight() -std::min(yLower, yFloor) * dv;
				int col, yoffset, texu = ((int)u) % tex->getWidth(); if (texu < 0) texu += tex->getWidth();
				const Patch *p;
				if ((p = tex->getPatchForColumn(texu, col, yoffset)))
					renderLaters[x].push_back({p, col, top, bot,  v + top * dv + tdY - yoffset, dv, z, lut});
			}

			if (seg.lSector->sky) DrawSky(seg.lSector->sky, ceilbot, upper);
			else if (seg.sidedef->uppertexture.size()) DrawTexture(seg.sidedef->uppertexture, ceilbot, upper, yCeiling, yUpper, rlCeiling, 0);
			if (seg.sidedef->lowertexture.size()) DrawTexture(seg.sidedef->lowertexture, lower, floortop, yLower, yFloor, lrFloor, 2);
			ceilingClipHeight[x] = std::max(CurrentCeilingEnd - 1, upper);
			floorClipHeight[x] = std::min(CurrentFloorStart + 1, lower);
		}
        else
		{
			DrawTexture(seg.sidedef->middletexture, midtop, midbot, yCeiling, yFloor, roomHeight, 1);
			ceilingClipHeight[x] = renderHeight;
			floorClipHeight[x] = -1;
		}
		yUpper += dyUpper;
		yLower += dyLower;
        yCeiling += dyCeiling;
        yFloor += dyFloor;
		skyAng += dSkyAng;
    }
}


