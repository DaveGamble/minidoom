#include "ViewRenderer.hpp"

#include <algorithm>
#include <assert.h>
#include "Map.hpp"
#include "Texture.hpp"
#include "Flat.hpp"

ViewRenderer::ViewRenderer(int renderXSize, int renderYSize, const uint8_t (&l)[34][256])
: renderWidth(renderXSize)
, renderHeight(renderYSize)
, halfRenderWidth(renderXSize / 2)
, halfRenderHeight(renderYSize / 2)
, distancePlayerToScreen(halfRenderWidth)	// 90 here is FOV
, lights(l)
, ceilingClipHeight(renderWidth)
, floorClipHeight(renderWidth)
{
}


void ViewRenderer::render(uint8_t *pScreenBuffer, int iBufferPitch, const Viewpoint& view, Map &map)
{
	screenBuffer = pScreenBuffer;
	rowlen = iBufferPitch;
	renderLaters.clear();
	solidWallRanges.clear();
	solidWallRanges.push_back({INT_MIN, -1});
	solidWallRanges.push_back({renderWidth, INT_MAX});
	std::fill(ceilingClipHeight.begin(), ceilingClipHeight.end(), -1);
	std::fill(floorClipHeight.begin(), floorClipHeight.end(), renderHeight);
	map.render3DView(view, [&] (const Seg &seg){ addWallInFOV(seg, view); });

	for (int i = (int)renderLaters.size() - 1; i >= 0; i--)
	{
		renderLater& r = renderLaters[i];
		for (int y = r.from; y < r.to; y++)
		{
			uint16_t p = r.texture->pixel(r.u, r.dv * y + r.v);
			if (p != 256) screenBuffer[rowlen * y + r.x] = r.light[p];
		}
	}
}

void ViewRenderer::addWallInFOV(const Seg &seg, const Viewpoint &v)
{
	const int toV1x = seg.start.x - v.x, toV1y = seg.start.y - v.y, toV2x = seg.end.x - v.x, toV2y = seg.end.y - v.y;	// Vectors from origin to segment ends.
	if (toV1x * toV2y >= toV1y * toV2x) return;	// If sin(angle) between the two (computed as dot product of V1 and normal to V2) is +ve, wall is out of view. (It's behind us)

	const float ca = v.cosa, sa = v.sina;
	const float tov1z = toV1x * ca + toV1y * sa, tov2z = toV2x * ca + toV2y * sa;
	float tov1x = toV1x * sa - toV1y * ca, tov2x = toV2x * sa - toV2y * ca;	// Rotate vectors to be in front of us.
	// z = how far in front of us it is. -ve values are behind. +ve values are in front.
	// x = position left to right. left = -1, right = 1.

	if (tov1z < tov1x && tov2z < -tov2x) return;	// V1 is right of FOV and V2 is left of FOV
	if (tov1z < -tov1x && tov2z < -tov2x) return;	// Both points are to the left.
	if (tov1z < tov1x && tov1z > -tov1x) return;	// V1 is within 45 degrees of the X axis (it's on your right hand side, out of the FOV). Both points are to the right.

	if (tov1z < -tov1x) tov1x = -tov1z; // Clip hard left
	if (tov2z < tov2x) tov2x = tov2z;	// Clip hard right
	
	auto AngleToScreen = [&](float dz, float dx) {
		return distancePlayerToScreen + round(dx * halfRenderWidth / dz);
	};

	const int x1 = AngleToScreen(tov1z, tov1x), x2 = AngleToScreen(tov2z, tov2x); // Find Wall X Coordinates

    if (x1 == x2) return; // Skip same pixel wall
	bool solid =  (!seg.lSector || seg.lSector->ceilingHeight <= seg.rSector->floorHeight || seg.lSector->floorHeight >= seg.rSector->ceilingHeight); // Handle walls and closed door

	if (!solid && (seg.lSector->sky && seg.lSector->sky == seg.rSector->sky)
		&& seg.lSector->floortexture == seg.rSector->floortexture && seg.lSector->lightlevel == seg.rSector->lightlevel
//		&& seg.lSector->ceilingHeight == seg.rSector->ceilingHeight
		&& seg.lSector->floorHeight == seg.rSector->floorHeight
		&& !seg.linedef->rSidedef->middletexture) return;
	
	if (solid && solidWallRanges.size() < 2) return;

    auto f = solidWallRanges.begin(); while (x1 - 1 > f->end) ++f;

    if (x1 < f->start)
    {
        if (x2 < f->start - 1)
        {
            storeWallRange(seg, x1, x2, tov1z, tov2z, v); //All of the wall is visible, so insert it
            if (solid) solidWallRanges.insert(f, {x1, x2});
            return;
        }
        storeWallRange(seg, x1, f->start - 1, tov1z, tov2z, v); // The end is already included, just update start
        if (solid) f->start = x1;
    }
    
    if (x2 <= f->end) return; // This part is already occupied
    std::list<SolidSegmentRange>::iterator nextWall = f;
    while (x2 >= next(nextWall, 1)->start - 1)
    {
        storeWallRange(seg, nextWall->end + 1, next(nextWall, 1)->start - 1, tov1z, tov2z, v); // partialy clipped by other walls, store each fragment
		if (x2 > (++nextWall)->end) continue;
		if (!solid) return;
		f->end = nextWall->end;
		solidWallRanges.erase(++f, ++nextWall);
		return;
    }
    storeWallRange(seg, nextWall->end + 1, x2, tov1z, tov2z, v);
	if (!solid) return;
	f->end = x2;
	if (nextWall != f) solidWallRanges.erase(++f, ++nextWall);
}

void ViewRenderer::storeWallRange(const Seg &seg, int x1, int x2, float z1, float z2, const Viewpoint &v)
{
	const int16_t flags = seg.linedef->flags;
	const float roomHeight = seg.rSector->ceilingHeight - seg.rSector->floorHeight;
	const float lHeight = seg.lSector ? seg.lSector->floorHeight - seg.lSector->ceilingHeight : 0;
	const float lrFloor = seg.lSector ? seg.lSector->floorHeight - seg.rSector->floorHeight : 0;
	const float rlCeiling = seg.lSector ? seg.rSector->ceilingHeight - seg.lSector->ceilingHeight : 0;
	const float tdX = seg.linedef->rSidedef->dx, tdY = seg.linedef->rSidedef->dy;
	const float sinv = v.sina, cosv = v.cosa;
	const float seglen = seg.linedef->len;

	// Fixme
	const int toV1x = seg.start.x - v.x, toV1y = seg.start.y - v.y;	// Vectors from origin to segment ends.
	const float idistanceToNormal = 1.0 / (toV1y * (seg.end.x - seg.start.x) - toV1x * (seg.end.y - seg.start.y));
	const float d2 = -(cosv * (seg.end.x - seg.start.x) + sinv * (seg.end.y - seg.start.y)) * idistanceToNormal;
	const float d1 = distancePlayerToScreen * (sinv * (seg.end.x - seg.start.x) - cosv * (seg.end.y - seg.start.y)) * idistanceToNormal - halfRenderWidth * d2;

    const float x1z = x1 * d2 + d1;
	const float dx = std::clamp(d2, -64.f, 64.f);

	const int sx = v.x - seg.linedef->start.x, sy = v.y - seg.linedef->start.y;
	const float distanceToNormal = sx * (seg.linedef->end.y - seg.linedef->start.y) - sy * (seg.linedef->end.x - seg.linedef->start.x);

	const float uB = (sinv * sy + sx * cosv) * seglen,
				uD = -d2 * distanceToNormal,
				uA = distancePlayerToScreen * (cosv * sy - sinv * sx) * seglen - halfRenderWidth * uB,
				uC = -d1 * distanceToNormal;
	// Calculations I am doing: ((uA + x * uB) / (uC + x * uD)), * dx, * x1z
	//

	const float vG = distancePlayerToScreen * (v.z -seg.rSector->floorHeight), vH = distancePlayerToScreen * (seg.rSector->ceilingHeight - v.z);
	const float vA = cosv - sinv, vB = 2 * sinv / renderWidth, vC = v.x, vD = cosv + sinv, vE = -2 * cosv / renderWidth, vF = v.y - 5;

	const float dyCeiling = -(seg.rSector->ceilingHeight - v.z) * dx;
	const float dyFloor = -(seg.rSector->floorHeight - v.z) * dx;
	const float dyUpper = seg.lSector ? -(seg.lSector->ceilingHeight - v.z) * dx : 0;
	const float dyLower = seg.lSector ? -((seg.lSector->floorHeight - v.z) * dx) : 0;
	const float dSkyAng = 1.0 / renderWidth;

	const float horizon = halfRenderHeight + v.pitch * halfRenderHeight;

	float yCeiling = horizon - (seg.rSector->ceilingHeight - v.z) * x1z;
	float yFloor = horizon - (seg.rSector->floorHeight - v.z) * x1z;
	float yUpper = seg.lSector ? horizon - ((seg.lSector->ceilingHeight - v.z) * x1z) : 0;
	float yLower = seg.lSector ? horizon - ((seg.lSector->floorHeight - v.z) * x1z) : 0;
	float skyAng = x1 * dSkyAng - 2 * v.angle / M_PI;
	
	const uint8_t *lut = lights[31 - (seg.rSector->lightlevel >> 3)];

	for (int x = x1; x <= x2; x++)
    {
		const float u = ((uA + x * uB) / (uC + x * uD)) + tdX;

		auto DrawTexture = [&](const Texture *texture, int from, int to, float a, float b, float dv, bool upper = false) {
			if (!texture) return;
			dv /= (b - a);
			float v = -a * dv;
			if (upper && !(flags & 8)) {v = -b * dv;}
			if (!upper && (flags & 16)) {v = -b * dv + roomHeight;}
			v += tdY;
			for (int y = from; y < to; y++) { screenBuffer[rowlen * y + x] = lut[texture->pixel(u, v + y * dv) & 255]; }
		};

		auto DrawSky = [&](const Patch *sky, int from, int to) {
			int tx = (skyAng - floor(skyAng)) * sky->getWidth();
			for (int i = from; i < to; i++)
			{
				float ty = std::clamp((i - horizon + halfRenderHeight) * sky->getHeight() / (float)renderHeight, -1.f, sky->getHeight() - 1.f);
				screenBuffer[rowlen * i + x] = lights[0][sky->pixel(sky->getColumnDataIndex((ty < 0) ? 0 : tx), std::max(ty, 0.f))];
			}
		};

		auto DrawFloor = [&](const Flat *flat, int from, int to) {
			for (int i = from; i < to; i++) { float z = vG / (i - horizon); screenBuffer[i * rowlen + x] = lut[flat->pixel(z * (vA + vB * x) + vC, z * (vD + vE * x) + vF)]; }
		};

		auto DrawCeiling = [&](const Flat *flat, int from, int to) {
			if (seg.rSector->sky)	DrawSky(seg.rSector->sky, from, to);
			else for (int i = from; i < to; i++) { float z = vH / (horizon - i); screenBuffer[i * rowlen + x] = lut[flat->pixel(z * (vA + vB * x) + vC, z * (vD + vE * x) + vF)]; }
		};
				
		int CurrentCeilingEnd = std::max(yCeiling, ceilingClipHeight[x] + 1.f), CurrentFloorStart = std::min(yFloor, floorClipHeight[x] - 1.f);
		int upper = std::min(floorClipHeight[x] - 1.f, yUpper), lower = std::max(yLower, ceilingClipHeight[x] + 1.f);
		int ceiltop = std::max(0, ceilingClipHeight[x]), ceilbot = std::min(CurrentCeilingEnd, CurrentFloorStart);
		int floortop = std::max(CurrentFloorStart, ceilingClipHeight[x]);
		int midtop = std::max(std::max(0, ceilbot), ceilingClipHeight[x]), midbot = std::min(floortop, renderHeight - 1);
		
		DrawFloor(seg.rSector->floortexture, floortop, floorClipHeight[x]);
		DrawCeiling(seg.rSector->ceilingtexture, ceiltop, ceilbot);

        if (seg.lSector)
        {
			if (seg.linedef->rSidedef->middletexture && midtop < midbot && yFloor > yCeiling)
			{
				float dv = lHeight / (yFloor - yCeiling);
				renderLaters.push_back({seg.linedef->rSidedef->middletexture, x, midtop, midbot, u, ((flags & 16) ? -yFloor * dv + roomHeight :  -yCeiling * dv) + tdY, dv, lut});
			}

			if (seg.lSector->sky)	DrawSky(seg.lSector->sky, ceilbot, upper);
			else					DrawTexture(seg.linedef->rSidedef->uppertexture, ceilbot, upper, yCeiling, yUpper, rlCeiling, true);
			DrawTexture(seg.linedef->rSidedef->lowertexture, lower, floortop, yLower, yFloor, lrFloor);
			ceilingClipHeight[x] = std::max(CurrentCeilingEnd - 1, upper);
			floorClipHeight[x] = std::min(CurrentFloorStart + 1, lower);
		}
        else
		{
			DrawTexture(seg.linedef->rSidedef->middletexture, midtop, midbot, yCeiling, yFloor, roomHeight);
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


