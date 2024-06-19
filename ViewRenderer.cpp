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

	const float ca = cos(v.angle), sa = sin(v.angle);
	float tov1z = toV1x * ca + toV1y * sa, tov1x = toV1x * sa - toV1y * ca, tov2z = toV2x * ca + toV2y * sa, tov2x = toV2x * sa - toV2y * ca;	// Rotate vectors to be in front of us.
	// z = how far in front of us it is. -ve values are behind. +ve values are in front.
	// x = position left to right. left = -1, right = 1.

	auto amod = [](float a) {
		return a - M_PI * 2 * floor(a * 0.5 * M_1_PI);
	};

	if (tov1z < abs(tov1x))
	{
		// ok, this is complicated, but we want to know if the line intersects either FOV marker, and if not, we reject.
		// so the segment is [z = tov1z + t * (tov2z - tov1z), x = tov1x + t * (tov2x - tov1x)], and the fov markers are  z + x = 0 and z - x = 0, for z +ve
		// ok, so test case 1 is
		// tov1z + t * (tov2z - tov1z) + tov1x + t * (tov2x - tov1x) = 0, solve for t, establish that it's in the range 0<1, and that the z value is +ve.
		// test case 2 is
		// tov1z + t * (tov2z - tov1z) - tov1x + t * (tov2x - tov1x) = 0, solve for t, establish that it's in the range 0<1, and that the z value is +ve.
		//
		// case 1: tov1z + tov1x + t * (tov2z - tov1z + tov2x - tov1x) = 0, so t = -(tov1z + tov1x) / (tov2z - tov1z + tov2x - tov1x)
		// case 2: tov1z - tov1x + t * (tov2z - tov1z - tov2x + tov1x) = 0, so t = (tov1z - tov1x) / (tov2z - tov1z - tov2x + tov1x)
		float t1n = -tov1z - tov1x, t1d = tov2z - tov1z + tov2x - tov1x, t2n = -tov1z + tov1x, t2d = tov2z - tov1z - tov2x + tov1x;
		if (t1d < 0) {t1n = -t1n; t1d = -t1d;}
		if (t2d < 0) {t2n = -t2n; t2d = -t2d;}
		if ((t1n < 0 || t1n > t1d || (tov1z * t1d + t1n * (tov2z - tov1z) < 0)) && (t2n < 0 || t2n > t2d || (tov1z * t2d + t2n * (tov2z - tov1z) < 0))) return;

//		if (amod(atan2f(-tov1x, tov1z) - M_PI_4) >= amod(atan2f(-tov1x, tov1z) - atan2f(-tov2x, tov2z))) return; // now we know that V1, is outside the left side of the FOV But we need to check is Also V2 is outside. Lets find out what is the size of the angle outside the FOV // Are both V1 and V2 outside?
//		if (atan2f(tov1y - tov1x, tov1x + tov1y) + 2 * M_PI  + atan2f(tov2y, tov2x) >= atan2f(tov1y, tov1x)) return; // now we know that V1, is outside the left side of the FOV But we need to check is Also V2 is outside. Lets find out what is the size of the angle outside the FOV // Are both V1 and V2 outside?
		tov1z = 1; tov1x = -1;
	}
	if (tov2z < tov2x) {tov2z = tov2x = 1;}	// Is V2 outside the FOV?
	
	auto AngleToScreen = [&](float dz, float dx) {
		return distancePlayerToScreen + round(dx * halfRenderWidth / dz);
	};

	const int V1XScreen = AngleToScreen(tov1z, tov1x), V2XScreen = AngleToScreen(tov2z, tov2x); // Find Wall X Coordinates
	assert(V1XScreen >= 0 && V1XScreen <= renderWidth);
	assert(V2XScreen >= 0 && V2XScreen <= renderWidth);

    if (V1XScreen == V2XScreen) return; // Skip same pixel wall
	bool solid = false;
    if (!seg.lSector) solid = true; // Handle solid walls
    else if (seg.lSector->ceilingHeight <= seg.rSector->floorHeight || seg.lSector->floorHeight >= seg.rSector->ceilingHeight) // Handle closed door
        solid = true;
//    else if (seg.rSector->ceilingHeight != seg.lSector->ceilingHeight || seg.rSector->floorHeight != seg.lSector->floorHeight) // Windowed walls
//        solid = false;
//	else return;

	if (solid && solidWallRanges.size() < 2) return;

	float DistanceToNormal = -toV1x * sin(seg.slopeAngle) + toV1y * cos(seg.slopeAngle);

	
	const SolidSegmentRange CurrentWall = { V1XScreen, V2XScreen }; // Find clip window
    std::list<SolidSegmentRange>::iterator FoundClipWall = solidWallRanges.begin();
    while (FoundClipWall != solidWallRanges.end() && CurrentWall.start - 1 > FoundClipWall->end) ++FoundClipWall;

    if (CurrentWall.start < FoundClipWall->start)
    {
        if (CurrentWall.end < FoundClipWall->start - 1)
        {
            storeWallRange(seg, CurrentWall.start, CurrentWall.end, DistanceToNormal, v); //All of the wall is visible, so insert it
            if (solid) solidWallRanges.insert(FoundClipWall, CurrentWall);
            return;
        }
        storeWallRange(seg, CurrentWall.start, FoundClipWall->start - 1, DistanceToNormal, v); // The end is already included, just update start
        if (solid) FoundClipWall->start = CurrentWall.start;
    }
    
    if (CurrentWall.end <= FoundClipWall->end) return; // This part is already occupied
    std::list<SolidSegmentRange>::iterator NextWall = FoundClipWall;
    while (CurrentWall.end >= next(NextWall, 1)->start - 1)
    {
        storeWallRange(seg, NextWall->end + 1, next(NextWall, 1)->start - 1, DistanceToNormal, v); // partialy clipped by other walls, store each fragment
        ++NextWall;
        if (CurrentWall.end <= NextWall->end)
        {
			if (solid)
			{
				FoundClipWall->end = NextWall->end;
				if (NextWall != FoundClipWall) solidWallRanges.erase(++FoundClipWall, ++NextWall);
			}
            return;
        }
    }
    storeWallRange(seg, NextWall->end + 1, CurrentWall.end, DistanceToNormal, v);
	if (solid)
	{
		FoundClipWall->end = CurrentWall.end;
		if (NextWall != FoundClipWall) solidWallRanges.erase(++FoundClipWall, ++NextWall);
	}
}

void ViewRenderer::storeWallRange(const Seg &seg, int V1XScreen, int V2XScreen, float DistanceToNormal, const Viewpoint &v)
{
	bool bDrawUpperSection = false, bDrawLowerSection = false, UpdateFloor = false, UpdateCeiling = false;;
	float UpperHeightStep = 0, iUpperHeight = 0, LowerHeightStep = 0, iLowerHeight = 0;

	auto GetScaleFactor = [&](int VXScreen) {
		float screenAng = atan((halfRenderWidth - VXScreen) / (float)distancePlayerToScreen);
		float SkewAngle = screenAng + v.angle - seg.slopeAngle;
		return std::clamp((distancePlayerToScreen * sinf(SkewAngle)) / (DistanceToNormal * cosf(screenAng)), 0.00390625f, 64.0f);
	};

    float V1ScaleFactor = GetScaleFactor(V1XScreen);
    float Steps = (GetScaleFactor(V2XScreen) - V1ScaleFactor) / (V2XScreen - V1XScreen);

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
	const float PT = tan(v.angle);
	const float uA = halfRenderWidth * ((1 - PT) * sy - (1 + PT) * sx),
				uB = PT * sy + sx,
				uC = halfRenderWidth * ((1 - PT) * dy - (1 + PT) * dx),
				uD = PT * dy + dx;
					  
//	const float uA = py - seg.linedef->start.y, uB = seg.linedef->start.x - px, uC = seg.linedef->end.y - seg.linedef->start.y, uD = seg.linedef->start.x - seg.linedef->end.x;
	const float pc = cos(v.angle) / 64, ps = sin(v.angle) / 64;
	const float vG = distancePlayerToScreen * (v.z -seg.rSector->floorHeight), vH = distancePlayerToScreen * (seg.rSector->ceilingHeight - v.z);
	const float vA = pc - ps, vB = 2 * ps / renderWidth, vC = v.x / 64.f, vD = pc + ps, vE = -2 * pc / renderWidth, vF = v.y / 64.f;

	for (int x = V1XScreen; x <= V2XScreen; x++)
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


