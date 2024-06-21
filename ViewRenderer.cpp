#include "ViewRenderer.hpp"

#include <algorithm>
#include <assert.h>
#include "Map.hpp"
#include "Texture.hpp"
#include "Flat.hpp"

ViewRenderer::ViewRenderer(int renderXSize, int renderYSize)
: renderWidth(renderXSize)
, renderHeight(renderYSize)
, halfRenderWidth(renderXSize / 2)
, halfRenderHeight(renderYSize / 2)
, distancePlayerToScreen(halfRenderWidth)	// 90 here is FOV
{
	ceilingClipHeight.resize(renderWidth);
	floorClipHeight.resize(renderWidth);
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
		float scale = (r.fl - r.cl) / (float)r.texture->getHeight();
		r.texture->renderColumn(screenBuffer + rowlen * r.from + r.x, rowlen, r.texture->getWidth() * r.u, scale, (r.from - r.cl) / scale, (r.to - r.from));
	}
}

void ViewRenderer::addWallInFOV(const Seg &seg, const Viewpoint &v)
{
	const int toV1x = seg.start.x - v.x, toV1y = seg.start.y - v.y, toV2x = seg.end.x - v.x, toV2y = seg.end.y - v.y;	// Vectors from origin to segment ends.
	if (toV1x * toV2y >= toV1y * toV2x) return;	// If sin(angle) between the two (computed as dot product of V1 and normal to V2) is +ve, wall is out of view. (It's behind us)

	const float ca = v.cosa, sa = v.sina;
	float tov1z = toV1x * ca + toV1y * sa, tov1x = toV1x * sa - toV1y * ca, tov2z = toV2x * ca + toV2y * sa, tov2x = toV2x * sa - toV2y * ca;	// Rotate vectors to be in front of us.
	// z = how far in front of us it is. -ve values are behind. +ve values are in front.
	// x = position left to right. left = -1, right = 1.

	if (tov1z < -tov1x && tov2z < -tov2x) return;	// Both points are to the left.
	if (tov1z < tov1x && tov1z > -tov1x) return;	// V1 is within 45 degrees of the X axis (it's on your right hand side, out of the FOV). Both points are to the right.

	if (tov1z < -tov1x) {tov1z = 1; tov1x = -1;} // Clip hard left
	if (tov2z < tov2x) {tov2z = tov2x = 1;}	// Clip hard right
	
	auto AngleToScreen = [&](float dz, float dx) {
		return distancePlayerToScreen + round(dx * halfRenderWidth / dz);
	};

	const int x1 = AngleToScreen(tov1z, tov1x), x2 = AngleToScreen(tov2z, tov2x); // Find Wall X Coordinates

    if (x1 == x2) return; // Skip same pixel wall
	bool solid =  (!seg.lSector || seg.lSector->ceilingHeight <= seg.rSector->floorHeight || seg.lSector->floorHeight >= seg.rSector->ceilingHeight); // Handle walls and closed door

	if (solid && solidWallRanges.size() < 2) return;

	const float idistanceToNormal = 1.0 / (toV1y * (seg.end.x - seg.start.x) - toV1x * (seg.end.y - seg.start.y));
	const float d2 = -(ca * (seg.end.x - seg.start.x) + sa * (seg.end.y - seg.start.y)) * idistanceToNormal;
	const float d1 = distancePlayerToScreen * (sa * (seg.end.x - seg.start.x) - ca * (seg.end.y - seg.start.y)) * idistanceToNormal - halfRenderWidth * d2;

    auto f = solidWallRanges.begin(); while (x1 - 1 > f->end) ++f;

    if (x1 < f->start)
    {
        if (x2 < f->start - 1)
        {
            storeWallRange(seg, x1, x2, d1, d2, v); //All of the wall is visible, so insert it
            if (solid) solidWallRanges.insert(f, {x1, x2});
            return;
        }
        storeWallRange(seg, x1, f->start - 1, d1, d2, v); // The end is already included, just update start
        if (solid) f->start = x1;
    }
    
    if (x2 <= f->end) return; // This part is already occupied
    std::list<SolidSegmentRange>::iterator nextWall = f;
    while (x2 >= next(nextWall, 1)->start - 1)
    {
        storeWallRange(seg, nextWall->end + 1, next(nextWall, 1)->start - 1, d1, d2, v); // partialy clipped by other walls, store each fragment
		if (x2 > (++nextWall)->end) continue;
		if (solid)
		{
			f->end = nextWall->end;
			solidWallRanges.erase(++f, ++nextWall);
		}
		return;
    }
    storeWallRange(seg, nextWall->end + 1, x2, d1, d2, v);
	if (solid)
	{
		f->end = x2;
		if (nextWall != f) solidWallRanges.erase(++f, ++nextWall);
	}
}

void ViewRenderer::storeWallRange(const Seg &seg, int x1, int x2, float d1, float d2, const Viewpoint &v)
{
	auto GetScaleFactor = [&](int x) { return std::clamp(d1 + x * d2, 0.00390625f, 64.0f); };

	
	bool bDrawUpperSection = false, bDrawLowerSection = false, UpdateFloor = false, UpdateCeiling = false;;
	float UpperHeightStep = 0, iUpperHeight = 0, LowerHeightStep = 0, iLowerHeight = 0;


    float V1ScaleFactor = GetScaleFactor(x1);
    float Steps = (GetScaleFactor(x2) - V1ScaleFactor) / (x2 - x1);

    float RightSectorCeiling = seg.rSector->ceilingHeight - v.z;
	float RightSectorFloor = seg.rSector->floorHeight - v.z;

    float CeilingStep = -(RightSectorCeiling * Steps), CeilingEnd = round(halfRenderHeight - RightSectorCeiling * V1ScaleFactor);
    float FloorStep = -(RightSectorFloor * Steps), FloorStart = round(halfRenderHeight - RightSectorFloor * V1ScaleFactor);

    if (seg.lSector)
    {
		float LeftSectorCeiling = seg.lSector->ceilingHeight - v.z;
		float LeftSectorFloor = seg.lSector->floorHeight - v.z;

		UpdateCeiling = (LeftSectorCeiling != RightSectorCeiling);
		UpdateFloor = (LeftSectorFloor != RightSectorFloor);

		if (seg.lSector->ceilingHeight <= seg.rSector->floorHeight || seg.lSector->floorHeight >= seg.rSector->ceilingHeight) // closed door
			UpdateCeiling = UpdateFloor = true;
		if (seg.rSector->ceilingHeight <= v.z) // below view plane
			UpdateCeiling = false;
		if (seg.rSector->floorHeight >= v.z) // above view plane
			UpdateFloor = false;

        if (LeftSectorCeiling < RightSectorCeiling)
            bDrawUpperSection = true;

		UpperHeightStep = -(LeftSectorCeiling * Steps);
		iUpperHeight = round(halfRenderHeight - (LeftSectorCeiling * V1ScaleFactor));

        if (LeftSectorFloor > RightSectorFloor)
            bDrawLowerSection = true;

		LowerHeightStep = -(LeftSectorFloor * Steps);
		iLowerHeight = round(halfRenderHeight - (LeftSectorFloor * V1ScaleFactor));
    }

	const int dx = seg.linedef->end.x - seg.linedef->start.x, dy = seg.linedef->end.y - seg.linedef->start.y;
	const int sx = v.x - seg.linedef->start.x, sy = v.y - seg.linedef->start.y;
	const float sinv = v.sina, cosv = v.cosa;
	const float uA = halfRenderWidth * ((cosv - sinv) * sy - (cosv + sinv) * sx),
				uB = sinv * sy + sx * cosv,
				uC = halfRenderWidth * ((cosv - sinv) * dy - (cosv + sinv) * dx),
				uD = sinv * dy + dx * cosv;
					  
//	const float uA = py - seg.linedef->start.y, uB = seg.linedef->start.x - px, uC = seg.linedef->end.y - seg.linedef->start.y, uD = seg.linedef->start.x - seg.linedef->end.x;
	const float pc = cosv / 64, ps = sinv / 64;
	const float vG = distancePlayerToScreen * (v.z -seg.rSector->floorHeight), vH = distancePlayerToScreen * (seg.rSector->ceilingHeight - v.z);
	const float vA = pc - ps, vB = 2 * ps / renderWidth, vC = v.x / 64.f, vD = pc + ps, vE = -2 * pc / renderWidth, vF = v.y / 64.f;

	for (int x = x1; x <= x2; x++)
    {
		const float u = std::clamp((uA + x * uB) / (uC + x * uD), 0.f, 1.f);

        int CurrentCeilingEnd = std::max(CeilingEnd, ceilingClipHeight[x] + 1.f);
        int CurrentFloorStart = std::min(FloorStart, floorClipHeight[x] - 1.f);

		auto DrawTexture = [&](const Texture *texture, int x, int from, int to, float u, int cl, int fl) {
			if (!texture || to < from || fl <= cl) return;
			float scale = (fl - cl) / (float)texture->getHeight();
			texture->renderColumn(screenBuffer + rowlen * from + x, rowlen, (texture->getWidth() - 1) * u, scale, std::max((from - cl), 0) / scale, (to - from));
		};


		auto DrawFloor = [&](const Flat *flat, int x, int from, int to) {
			for (int i = from; i < to; i++)
			{
				float z = vG / (i - halfRenderHeight);
				flat->renderSpan(screenBuffer + i * rowlen + x, 1, z * (vA + vB * x) + vC, z * (vD + vE * x) + vF, 0, 0);
			}
		};

		auto DrawCeiling = [&](const Flat *flat, int x, int from, int to) {
			for (int i = from; i < to; i++)
			{
				float z = vH / (halfRenderHeight - i);
				flat->renderSpan(screenBuffer + i * rowlen + x, 1, z * (vA + vB * x) + vC, z * (vD + vE * x) + vF, 0, 0);
			}
		};
		
		if (CurrentCeilingEnd > CurrentFloorStart)
		{
			CeilingEnd += CeilingStep;
			FloorStart += FloorStep;
			continue;
		}
		
        if (seg.lSector)
        {

			int upper = std::min(floorClipHeight[x] - 1.f, iUpperHeight);
			int lower = std::max(iLowerHeight, ceilingClipHeight[x] + 1.f);
			if (seg.linedef->rSidedef->middletexture && CurrentFloorStart > CurrentCeilingEnd && FloorStart > CeilingEnd)
				renderLaters.push_back({seg.linedef->rSidedef->middletexture, x, std::max(CurrentCeilingEnd, 0), std::min(CurrentFloorStart, renderHeight - 1), u, (int)CeilingEnd, (int)FloorStart});

			iUpperHeight += UpperHeightStep;
			iLowerHeight += LowerHeightStep;

			DrawCeiling(seg.rSector->ceilingtexture, x, std::max(0, ceilingClipHeight[x]), CurrentCeilingEnd);

			if (bDrawUpperSection)
			{
				DrawTexture(seg.linedef->rSidedef->uppertexture, x, CurrentCeilingEnd, upper, u, CeilingEnd, iUpperHeight);
				ceilingClipHeight[x] = std::max(CurrentCeilingEnd - 1, upper);
			}
			else if (UpdateCeiling) ceilingClipHeight[x] = CurrentCeilingEnd - 1;

			DrawFloor(seg.rSector->floortexture, x, CurrentFloorStart, floorClipHeight[x]);

			if (bDrawLowerSection)
			{
				DrawTexture(seg.linedef->rSidedef->lowertexture, x, lower, CurrentFloorStart, u, iLowerHeight, FloorStart);
				floorClipHeight[x] = std::min(CurrentFloorStart + 1, lower);
			}
			else if (UpdateFloor) floorClipHeight[x] = CurrentFloorStart + 1;
		}
        else
		{
			DrawTexture(seg.linedef->rSidedef->middletexture, x, CurrentCeilingEnd, CurrentFloorStart, u, CeilingEnd, FloorStart);
			DrawFloor(seg.rSector->floortexture, x, CurrentFloorStart, floorClipHeight[x]);
			DrawCeiling(seg.rSector->ceilingtexture, x, std::max(0, ceilingClipHeight[x]), CurrentCeilingEnd);
			ceilingClipHeight[x] = renderHeight;
			floorClipHeight[x] = -1;
		}
        CeilingEnd += CeilingStep;
        FloorStart += FloorStep;
    }
}


