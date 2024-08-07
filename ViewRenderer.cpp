#include "ViewRenderer.hpp"
#include <cassert>

const float light_depth = 0.025, sector_light_scale = -0.125, light_offset = 15;

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
, screenBuffer(new uint8_t[renderXSize * renderYSize])
, rowlen(renderXSize)
, wad(wadname)
, didload(false)
{
	if (!wad.didLoad()) return;
	std::vector<uint8_t> ll = wad.getLumpNamed("COLORMAP");
	for (int i = 0; i < 34; i++) memcpy(lights[i], ll.data() + 256 * i, 256);
	std::vector<uint8_t> pp = wad.getLumpNamed("PLAYPAL");
	for (int i = 0; i < 256; i++) pal[i] = (pp[i * 3 + 0] << 24) | (pp[i * 3 + 1] << 16) | (pp[i * 3 + 2] << 8) | 255;
	int li = wad.findLumpByName(mapName);
	std::vector<uint8_t> data; const uint8_t *ptr; size_t size;
	auto seek = [&](const char *name) { data = wad.getLumpNamed(name, li); ptr = data.data(); size = data.size(); return ptr; };
	
	std::vector<Vertex> vertices;
	if (seek("VERTEXES")) for (int i = 0; i < size; i += sizeof(Vertex)) vertices.push_back(*(Vertex*)(ptr + i));

	auto skyt = wad.getTexture("SKY1");
	const Texture *skytex = skyt.size() ? skyt[0] : nullptr;

	struct WADSector { int16_t fh, ch; char floorTexture[8], ceilingTexture[8]; uint16_t lightlevel, type, tag; };
	if (seek("SECTORS")) for (int i = 0; i < size; i += sizeof(WADSector))
	{
		WADSector *ws = (WADSector*)(ptr + i);
		const Texture * sky = (!strncasecmp(ws->ceilingTexture, "F_SKY", 5)) ? skytex : nullptr;
		sectors.push_back((Sector){ws->fh, ws->ch, wad.getFlat(ws->floorTexture), wad.getFlat(ws->ceilingTexture), ws->lightlevel, ws->lightlevel, ws->lightlevel, ws->type, ws->tag, sky});
	}

	struct WADSidedef { int16_t dx, dy; char upperTexture[8], lowerTexture[8], middleTexture[8]; uint16_t sector; };
	if (seek("SIDEDEFS")) for (int i = 0; i < size; i += sizeof(WADSidedef))
	{
		WADSidedef *ws = (WADSidedef*)(ptr + i);
		sidedefs.push_back((Sidedef){ws->dx, ws->dy, wad.getTexture(ws->upperTexture), wad.getTexture(ws->middleTexture), wad.getTexture(ws->lowerTexture), sectors.data() + ws->sector});
	}

	struct WADLinedef { uint16_t start, end, flags, type, sectorTag, rSidedef, lSidedef; }; // Sidedef 0xFFFF means there is no sidedef
	if (seek("LINEDEFS")) for (int i = 0; i < size; i += sizeof(WADLinedef))
	{
		WADLinedef *wl = (WADLinedef*)(ptr + i);
		linedefs.push_back((Linedef){vertices[wl->start], vertices[wl->end], wl->flags, wl->type, wl->sectorTag, {},
			(wl->rSidedef == 0xFFFF) ? nullptr : sidedefs.data() + wl->rSidedef, (wl->lSidedef == 0xFFFF) ? nullptr : sidedefs.data() + wl->lSidedef
		});
	}

	struct WADSeg { uint16_t start, end, slopeAngle, linedef, dir, offset; }; // Direction: 0 same as linedef, 1 opposite of linedef Offset: distance along linedef to start of seg
	if (seek("SEGS")) for (int i = 0; i < size; i += sizeof(WADSeg))
	{
		WADSeg *ws = (WADSeg*)(ptr + i);
		Linedef *pLinedef = &linedefs[ws->linedef];
		Sidedef *pRightSidedef = ws->dir ? pLinedef->lSidedef : pLinedef->rSidedef;
		Sidedef *pLeftSidedef = ws->dir ? pLinedef->rSidedef : pLinedef->lSidedef;
		segs.push_back((Seg){vertices[ws->start], vertices[ws->end], (float)(ws->slopeAngle * M_PI * 2 / 65536.f), pLinedef, pRightSidedef, ws->dir, (float)ws->offset,
			sqrtf((vertices[ws->start].x - vertices[ws->end].x) * (vertices[ws->start].x - vertices[ws->end].x) + (vertices[ws->start].y - vertices[ws->end].y) * (vertices[ws->start].y - vertices[ws->end].y)),
			(pRightSidedef) ? pRightSidedef->sector : nullptr, (pLeftSidedef) ? pLeftSidedef->sector : nullptr});
	}
	struct WADThing { int16_t x, y; uint16_t angle, type, flags; };
	if (seek("THINGS")) for (int i = 0; i < size; i += sizeof(WADThing)) {WADThing *wt = (WADThing*)(ptr + i); things.push_back((Thing){wt->x, wt->y, wt->angle, wt->type, wt->flags});}
	if (seek("NODES")) for (int i = 0; i < size; i += sizeof(Node)) nodes.push_back(*(Node*)(ptr + i));
	if (seek("SSECTORS")) for (int i = 0; i < size; i += sizeof(Subsector)) subsectors.push_back(*(Subsector*)(ptr + i));
	
	auto addLinedef = [&](Linedef &l, Sidedef *s) { if (s && s->sector) s->sector->linedefs.push_back(&l); };
	for (int i = 0; i < linedefs.size(); i++) { addLinedef(linedefs[i], linedefs[i].rSidedef); addLinedef(linedefs[i], linedefs[i].lSidedef); }
	for (Sector &s : sectors)
	{
		if (!s.type) continue;	// Skip these.
		uint16_t minlight = s.lightlevel;
		for (Linedef *l : s.linedefs)
		{
			if (l->tag == s.tag) l->targets.push_back(&s);
			if (l->lSidedef && l->lSidedef->sector && l->lSidedef->sector != &s) minlight = std::min(minlight, l->lSidedef->sector->lightlevel);
			if (l->rSidedef && l->rSidedef->sector && l->rSidedef->sector != &s) minlight = std::min(minlight, l->rSidedef->sector->lightlevel);
		}
		s.minlightlevel = (minlight == s.lightlevel) ? 0 : minlight;
	}
	struct thinginfo { int id; char name[5], anim[7]; int flags; };
	static std::vector<thinginfo> thinginfos = {	// UDS 1.666 list of things.
#include "thinginfo.inc"
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
				char buffer[9]; memset(buffer, 0, 9);
				snprintf(buffer, 9, "%s%c0", basename, toupper(*anim));
				t.imgs.push_back(wad.getPatch(buffer));
			}
		}
		Viewpoint v = (Viewpoint){t.x, t.y};
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
	weapon = wad.getPatch("PISGA0");
	didload = true;
}

ViewRenderer::~ViewRenderer() {delete[] screenBuffer;}

static bool doesLineSegmentIntersect(const Linedef *l, int x1, int y1, int x2, int y2, int *wherex = nullptr, int *wherey = nullptr)
{
	const int Ax = x2 - x1, Ay = y2 - y1, Bx = l->start.x - l->end.x, By = l->start.y - l->end.y, Cx = x1 - l->start.x, Cy = y1 - l->start.y;
	
	const int x1lo = (Ax < 0) ? x2 : x1, x1hi = (Ax < 0) ? x1 : x2, y1lo = (Ay < 0) ? y2 : y1, y1hi = (Ay < 0) ? y1 : y2;
	if (Bx > 0) { if( x1hi < l->end.x || l->start.x < x1lo) return false; } else if (x1hi < l->start.x || l->end.x < x1lo) return false;
	if (By > 0) { if (y1hi < l->end.y || l->start.y < y1lo) return false; } else if (y1hi < l->start.y || l->end.y < y1lo) return false;

	int den = Ay * Bx - Ax * By, tn = By * Cx - Bx * Cy;
	if (!den) return false;
	if (den > 0) { if (tn < 0 || tn > den) return false; } else if (tn > 0 || tn < den) return false;
	int un = Ax * Cy - Ay * Cx;
	if (den > 0) { if (un < 0 || un > den) return false; } else if (un > 0 || un < den) return false;

	if (wherex) *wherex = x1 + (tn * Ax + (((tn * Ax >= 0) == (den >= 0)) ? den/2 : -den/2)) / den;
	if (wherey) *wherey = y1 + (un * Ay + (((un * Ay >= 0) == (den >= 0)) ? den/2 : -den/2)) / den;
	return true;
}

void ViewRenderer::moveBy(float fwd, float side)
{
	float dx = fwd * view.cosa + side * view.sina, dy = fwd * view.sina - side * view.cosa;
	int size = 16 / 2;


	std::vector<const Linedef *> out;
	findIntersectingNodes((int)nodes.size() - 1, view.x, view.y, view.x + dx, view.y + dy, out, size);
	
	std::vector<const Linedef *> hits, triggers;
	for (const Linedef *l : out) if (!l->lSidedef || (l->flags & 1)) hits.push_back(l); else triggers.push_back(l);	// split 'em

	if (hits.size())
	{
		for (const Linedef *l : hits)
		{
			int lx = l->end.x - l->start.x, ly = l->end.y - l->start.y;
			float dot = lx * view.cosa + ly * view.sina;
			if (dot < 0) {lx *= -1; ly *= -1;}
			if (dot * dot < 0.5 * (lx * lx + ly * ly)) continue;	// angle too far off, can't do it.
			float ldet = 1.f / sqrt(lx * lx + ly * ly);
			dx = (fwd * lx + side * ly) * ldet;
			dy = (fwd * ly - side * lx) * ldet;
			break;
		}
		out.clear();
		findIntersectingNodes((int)nodes.size() - 1, view.x, view.y, view.x + dx, view.y + dy, out, size);
		if (out.size()) return;
	}

	// Process triggers!
	view.x += dx;
	view.y += dy;
}

void ViewRenderer::rotateBy(float dt)
{
	view.angle += dt;
	view.angle -= M_PI * 2 * floorf(0.5 * view.angle * M_1_PI);
	view.cosa = cos(view.angle);
	view.sina = sin(view.angle);
}

void ViewRenderer::updatePitch(float dp) { view.pitch = clamp(view.pitch - dp, -1.f, 1.f); }

void ViewRenderer::render(uint8_t *pScreenBuffer, int iBufferPitch)
{
	updatePlayerSubSectorHeight();

	frame++;
	texframe = frame / 20;
	solidWallRanges.clear();
	solidWallRanges.push_back((SolidSegmentRange){INT_MIN, -1});
	solidWallRanges.push_back((SolidSegmentRange){renderWidth, INT_MAX});
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
	renderBSPNodes((int)nodes.size() - 1);
	
	for (int x = 0; x < renderWidth; x++)
	{
		if (!renderLaters[x].size())  {renderMarks[x].clear(); continue;}
		std::sort(renderMarks[x].begin(), renderMarks[x].end(), [] (const renderMark &a, const renderMark &b) {return a.z < b.z; });
		std::sort(renderLaters[x].begin(), renderLaters[x].end(), [] (const renderLater &a, const renderLater &b) {return a.z < b.z; });
		for (int i = (int)renderLaters[x].size() - 1; i >= 0; i--)
		{
			renderLater& r = renderLaters[x][i];
			int from = std::max(0, r.from), to = std::min(r.to, renderHeight);
			for (int c = 0; to > from && c < renderMarks[x].size() && renderMarks[x][c].z < r.z; c++)
			{
				const renderMark &m = renderMarks[x][c];
				if (m.to <= from || m.from > to) continue;
				if (m.from <= from) from = std::max(from, m.to); else to = std::min(to, m.from);
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
	
//	weapon->render(screenBuffer, rowlen, -weapon->xoffset * 3, -weapon->yoffset * 3, lights[0], 3);

	const uint8_t *from = screenBuffer;
	for (int y = 0; y < renderHeight; y++)
	{
		uint32_t *to = (uint32_t*)(pScreenBuffer + iBufferPitch * y);
		for (int x = 0; x < renderWidth; x++) *to++ = pal[*from++];
	}
}

void ViewRenderer::addThing(const Thing &thing, const Seg &seg)
{
	const Patch *patch = thing.imgs[texframe % thing.imgs.size()];
	if (!patch) return;
	const int toV1x = thing.x - view.x, toV1y = thing.y - view.y;	// Vectors from origin to segment ends.
	const float ca = view.cosa, sa = view.sina, tz = toV1x * ca + toV1y * sa, tx = toV1x * sa - toV1y * ca;	// Rotate vectors to be in front of us.

	if (tz <= 0) return;

	int light = clamp(light_offset + seg.rSector->lightlevel * sector_light_scale + tz * light_depth, 0.f, 31.f);

	const float horizon = halfRenderHeight + view.pitch * halfRenderHeight, height = patch->height, scaling = distancePlayerToScreen / tz;

	const float top = (thing.attr & thing_hangs) ? seg.rSector->ceilingHeight + height - patch->yoffset : seg.rSector->floorHeight + patch->yoffset;
	float y1 = horizon + scaling * (view.z - top), y2 = horizon + scaling * (view.z - top + height);
	float dv = tz * invRenderWidth * 2;

	const float xc = distancePlayerToScreen + tx * scaling, scale = patch->width * scaling, xoff = (patch->width - patch->xoffset) * scaling;
	int x1 = std::max(xc - xoff, 0.f), x2 = std::min(xc + scale - xoff, (float)renderWidth);
	float u = dv * (x1 - xc + xoff);

	if (x2 < 0 || x1 >= renderWidth) return;

	for ( ; x1 < x2; x1++, u += dv)
		if (u >= 0 && u < patch->width && y2 > y1)
			renderLaters[x1].push_back((renderLater){patch, (int)u, (int)y1, (int)y2, 0, dv, tz, lights[light]});
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
            if (solid) solidWallRanges.insert(f, (SolidSegmentRange){x1, x2});
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
		const uint8_t *lut = lights[clamp(light, 0, 31)];

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
		};

		auto DrawSky = [&](const Texture *sky, int from, int to) {
			int tx = (skyAng - floor(skyAng)) * sky->width;
			if (tx == sky->width) tx = 0;
			for (int i = std::max(0, from); i < std::min(to, renderHeight); i++)
			{
				float ty = clamp((i - horizon + halfRenderHeight) * sky->height * invRenderHeight, -1.f, sky->height - 1.f);
				screenBuffer[rowlen * i + x] = lights[0][sky->pixel((ty < 0) ? sky->width / 2 : tx, std::max(ty, 0.f))];
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
				screenBuffer[i * rowlen + x] = lights[clamp(light, 0, 31)][flat->pixel(z * (vA + vB * x) + vC, z * (vD + vE * x) + vF)];
			}
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
					screenBuffer[i * rowlen + x] = lights[clamp(light, 0, 31)][flat->pixel(z * (vA + vB * x) + vC, z * (vD + vE * x) + vF)];
				}
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
				if (flags & kLowerTextureUnpeg)  v = tex->height -std::min(yLower, yFloor) * dv;
				int col, yoffset, texu = ((int)u) % tex->width; if (texu < 0) texu += tex->width;
				const Patch *p;
				if ((p = tex->getPatchForColumn(texu, col, yoffset)))
					renderLaters[x].push_back((renderLater){p, col, top, bot,  v + top * dv + tdY - yoffset, dv, z, lut});
			}

			if (seg.lSector->sky) DrawSky(seg.lSector->sky, ceilbot, upper);
			else if (seg.sidedef->uppertexture.size()) DrawTexture(seg.sidedef->uppertexture, ceilbot, upper, yCeiling, yUpper, rlCeiling, 0);
			if (seg.sidedef->lowertexture.size()) DrawTexture(seg.sidedef->lowertexture, lower, floortop, yLower, yFloor, lrFloor, 2);
			mark(x, 0, std::max(yCeiling, yUpper), z);
			mark(x, std::min(yFloor, yLower), renderHeight, z);
			ceilingClipHeight[x] = std::max(CurrentCeilingEnd - 1, upper);
			floorClipHeight[x] = std::min(CurrentFloorStart + 1, lower);
		}
        else
		{
			DrawTexture(seg.sidedef->middletexture, midtop, midbot, yCeiling, yFloor, roomHeight, 1);
			mark(x, 0, renderHeight, z);
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


void ViewRenderer::updatePlayerSubSectorHeight()
{
	int subsector = (int)(nodes.size() - 1);
	while (!(subsector & kSubsectorIdentifier)) subsector = isPointOnLeftSide(view, subsector) ? nodes[subsector].lChild : nodes[subsector].rChild;
	view.z = 41 + segs[subsectors[subsector & (~kSubsectorIdentifier)].firstSeg].rSector->floorHeight;
}


void ViewRenderer::renderBSPNodes(int iNodeID)
{
	if (iNodeID & kSubsectorIdentifier) // Masking all the bits exipt the last one to check if this is a subsector
	{
		iNodeID &= ~kSubsectorIdentifier;
		for (int i = 0; i < subsectors[iNodeID].numSegs; i++) addWallInFOV(segs[subsectors[iNodeID].firstSeg + i]);
		return;
	}
	const bool left = isPointOnLeftSide(view, iNodeID);
	renderBSPNodes(left ? nodes[iNodeID].lChild : nodes[iNodeID].rChild);
	renderBSPNodes(left ? nodes[iNodeID].rChild : nodes[iNodeID].lChild);
}

void ViewRenderer::findIntersectingNodes(int n, int x1, int y1, int x2, int y2, std::vector<const Linedef*>& out, int size) const
{
	auto sideForBox = [nodes = this->nodes, size](int x, int y, int node)
	{
		const int x1 = (x - size - nodes[node].x) * nodes[node].dy, x2 = (x + size - nodes[node].x) * nodes[node].dy;
		const int y1 = (y - size - nodes[node].y) * nodes[node].dx, y2 = (y + size - nodes[node].y) * nodes[node].dx;
		const int x1y1 = x1 - y1, x1y2 = x1 - y2, x2y1 = x2 - y1, x2y2 = x2 - y2;
		if (x1y1 > 0 && x1y2 > 0 && x2y1 > 0 && x2y2 > 0) return -1;	// right side
		if (x1y1 < 0 && x1y2 < 0 && x2y1 < 0 && x2y2 < 0) return 1;		// left side
		return 0; // there's an intersection.
	};
	
	if (n & kSubsectorIdentifier)	// subsector.
	{
		const Subsector &sub = subsectors[n & ~kSubsectorIdentifier];
		for (int i = 0; i < sub.numSegs; i++)
		{
			const Linedef *l = segs[sub.firstSeg + i].linedef;
			bool hit = false;
			hit |= doesLineSegmentIntersect(l, x2 - size, y2 + size, x2 + size, y2 + size);
			hit |= doesLineSegmentIntersect(l, x2 - size, y2 - size, x2 + size, y2 - size);
			hit |= doesLineSegmentIntersect(l, x2 + size, y2 - size, x2 + size, y2 + size);
			hit |= doesLineSegmentIntersect(l, x2 - size, y2 - size, x2 - size, y2 + size);
			if (hit) out.push_back(l);
		}
	}
	else
	{
		int side1 = sideForBox(x1, y1, n), side2 = sideForBox(x2, y2, n);
		if (side1 == side2 && side1)	// both on same side
			findIntersectingNodes((side1 == 1) ? nodes[n].lChild : nodes[n].rChild, x1, y1, x2, y2, out, size);	// pass it down
		else
		{
			findIntersectingNodes(nodes[n].lChild, x1, y1, x2, y2, out, size);
			findIntersectingNodes(nodes[n].rChild, x1, y1, x2, y2, out, size);	// it might intersect on either side.
		}
	}
}

bool ViewRenderer::isPointOnLeftSide(const Viewpoint &v, int node) const { return (v.x - nodes[node].x) * nodes[node].dy <= (v.y - nodes[node].y) * nodes[node].dx; }

struct WADPatchHeader { uint16_t width, height; int16_t leftOffset, topOffset; };
Patch::Patch(const char *_name, const uint8_t *ptr) : name(_name), cols(((WADPatchHeader*)ptr)->width)
, width(((WADPatchHeader*)ptr)->width), height(((WADPatchHeader*)ptr)->height), xoffset(((WADPatchHeader*)ptr)->leftOffset), yoffset(((WADPatchHeader*)ptr)->topOffset)
{
	for (int i = 0; i < width; ++i) for (int off = ((uint32_t*)(ptr))[i + 2]; ptr[off] != 0xff; off += ptr[off + 1] + 4) cols[i].push_back((colData){ptr[off], ptr[off + 1], ptr + off + 3});
}

void Patch::render(uint8_t *buf, int rowlen, int screenx, int screeny, const uint8_t *lut, float scale) const
{
	buf += rowlen * screeny + screenx;
	for (int x = 0, tox = 0; x < width; x++)
		while (tox < (x + 1) * scale)
		{
			for (const colData &c : cols[x])
			{
				int y = (c.top < 0) ? -c.top : 0, sl = floor(scale * (c.top + y)), el = floor((c.length - y) * scale) + sl;
				for (int i = 0, to = 0; to < std::max(el - sl, 0) && i < c.length; i++) while (to < (i + 1) * scale && to < std::max(el - sl, 0)) buf[tox + (sl + to++) * rowlen] = lut[c.data[i + y]];
			}
			tox++;
		}
}

void Patch::composeColumn(uint8_t *buf, int iHeight, int x, int yOffset) const {
	for (const colData &c : cols[x])
	{
		int y = std::max(yOffset + c.top, 0), iMaxRun = c.length + std::min(yOffset + c.top, 0);
		iMaxRun = std::min(iHeight - y, iMaxRun); if (iMaxRun > 0) memcpy(buf + y, c.data, iMaxRun);
	}
}

Texture::Texture(const char *_name, const uint8_t *ptr, WADLoader *wad) : name(_name)
{
	struct WADTextureData { char textureName[8]; uint32_t alwayszero; uint16_t width, height; uint32_t alwayszero2; uint16_t patchCount; };
	WADTextureData *textureData = (WADTextureData*)ptr;
	struct WADTexturePatch { int16_t dx, dy; uint16_t pnameIndex, alwaysone, alwayszero; };
	WADTexturePatch *texturePatch = (WADTexturePatch*)(ptr + 22);

	width = textureData->width;
	height = textureData->height;
	cols.resize(width);

	for (int i = 0; i < textureData->patchCount; ++i)
	{
		const Patch *patch = wad->getPatch(texturePatch[i].pnameIndex);	// Get the patch
		for (int x = std::max(texturePatch[i].dx, (int16_t)0); x < std::min(width, texturePatch[i].dx + patch->width); x++)
		{
			if (cols[x].patch)	// This column already has something in
			{
				if (!cols[x].overlap.size())	// Need to render off what's in there!
				{
					cols[x].overlap.resize(height);
					cols[x].patch->composeColumn(cols[x].overlap.data(), height, cols[x].x, cols[x].dy);
				}
				patch->composeColumn(cols[x].overlap.data(), height, x - texturePatch[i].dx, texturePatch[i].dy);	// Render your goodies on top.
			}
			else cols[x] = (colData){ x - texturePatch[i].dx, texturePatch[i].dy, patch, std::vector<uint8_t>()};	// Save this as the handler for this column.
		}
	}
}

WADLoader::WADLoader(const char *filename)
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
   const char *pn = (const char *)(data + dirs[npnames].lumpOffset);
   int32_t count = *(int32_t*)pn;
   for (int i = 0; i < count; ++i) pnames.push_back(pn + 4 + 8 * i);
   
   int cycle = -1;
   const char *toload[2] = {"TEXTURE1", "TEXTURE2"};
   static const char *specialtextures[kNumTextureCycles][2] = { {"BLODGR1", "BLODGR4"}, {"BLODRIP1", "BLODRIP4"}, {"FIREBLU1", "FIREBLU2"}, {"FIRELAV3", "FIRELAVA"},
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
		   textures.push_back(new Texture(name, lump + asint[j + 1], this));
		   if (cycle != -1) texturecycles[cycle].push_back(textures[textures.size() - 1]);
		   for (int k = 0; cycle != -1 && k < kNumTextureCycles; k++) if (!strncasecmp(name, specialtextures[k][1], 8)) cycle = -1;
	   }
   }
   
   static const char *specialflats[kNumFlatCycles][2] = {{"NUKAGE1", "NUKAGE3"}, {"FWATER1", "FWATER4"}, {"SWATER1", "SWATER4"}, {"LAVA1", "LAVA4"},
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

WADLoader::~WADLoader() { delete[] data; for (Patch *p : patches) delete p; for (Texture *t : textures) delete t; for (Flat *f : flats) delete f; }

std::vector<uint8_t> WADLoader::getLumpNamed(const char *name, size_t after) const {
	int id = findLumpByName(name, after); return (id == -1) ? std::vector<uint8_t>() : std::vector<uint8_t>(data + dirs[id].lumpOffset, data + dirs[id].lumpOffset + dirs[id].lumpSize);
}
int WADLoader::findLumpByName(const char *lumpName, size_t after) const {
	for (size_t i = after; i < numLumps; ++i) if (!strncasecmp(dirs[i].lumpName, lumpName, 8)) return (int)i; return -1;
}
const std::vector<const char*> WADLoader::getPatchesStartingWith(const char *name) {
	std::vector<const char*> all; for (int i = 0; i < numLumps; i++) if (!strncasecmp(name, dirs[i].lumpName, 4)) all.push_back(dirs[i].lumpName); return all;
}
const Patch *WADLoader::getPatch(const char *name) {
	for (const Patch *p : patches) if (!strncasecmp(name, p->name, 8)) return p;
	int n = findLumpByName(name); if (n == -1 || !dirs[n].lumpSize) {assert(0); return nullptr;}
	patches.push_back(new Patch(dirs[n].lumpName, data + dirs[n].lumpOffset)); return patches[patches.size() - 1];
}
std::vector<const Texture *> WADLoader::getTexture(const char *name) const {
	if (!strncasecmp(name, "-", 2)) return {};
	for (const Texture *t : textures) if (!strncasecmp(name, t->name, 8)) {for (int i = 0; i < kNumTextureCycles; i++) { for (const Texture *tt : texturecycles[i]) if (tt == t) return texturecycles[i]; } return {t};}
	assert(0); return {};
}
std::vector<const Flat *> WADLoader::getFlat(const char *name) const {
	for (const Flat *f : flats) if (!strncasecmp(name, f->name, 8)) { for (int i = 0; i < kNumFlatCycles; i++) {for (const Flat *ff : flatcycles[i]) if (ff == f) return flatcycles[i];} return {f}; }
	assert(0); return {};
}
