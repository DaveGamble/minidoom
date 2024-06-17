#include "ViewRenderer.h"

#include <algorithm>
#include "Map.h"
#include "Player.hpp"
#include "Texture.h"

ViewRenderer::ViewRenderer(Map *pMap, Player *pPlayer, int renderXSize, int renderYSize)
: map(pMap)
, player(pPlayer)
, renderWidth(renderXSize)
, renderHeight(renderYSize)
, halfRenderWidth(renderXSize / 2)
, halfRenderHeight(renderYSize / 2)
, distancePlayerToScreen(halfRenderWidth / tan(90 * 0.5 * M_PI / 180))	// 90 here is FOV
{
	screenXToAngle.resize(renderWidth + 1);
	for (int i = 0; i <= renderWidth; ++i)
		screenXToAngle[i] = atan((halfRenderWidth - i) / (float)distancePlayerToScreen) * 180 / M_PI;

	ceilingClipHeight.resize(renderWidth);
	floorClipHeight.resize(renderWidth);
}


void ViewRenderer::render(uint8_t *pScreenBuffer, int iBufferPitch)
{
	screenBuffer = pScreenBuffer;
	rowlen = iBufferPitch;
	renderLaters.clear();
	solidWallRanges.clear();
	solidWallRanges.push_back({INT_MIN, -1});
	solidWallRanges.push_back({renderWidth, INT_MAX});
	std::fill(ceilingClipHeight.begin(), ceilingClipHeight.end(), -1);
	std::fill(floorClipHeight.begin(), floorClipHeight.end(), renderHeight);

	map->render3DView();
	for (auto& r : renderLaters) {
		float scale = (r.fl - r.cl) / (float)r.texture->getHeight();
		r.texture->renderColumn(screenBuffer + rowlen * r.from + r.x, rowlen, r.texture->getWidth() * r.u, scale, (r.from - r.cl) / scale, (r.to - r.from));
	}
}

void ViewRenderer::addWallInFOV(Seg &seg)
{
	const float m_Angle = player->getAngle();
	const int px = player->getX(), py = player->getY();

	auto amod = [](float a) {
		return a - 360 * floor(a / 360);
	};

	int toV1x = seg.start.x - px, toV1y = seg.start.y - py, toV2x = seg.end.x - px, toV2y = seg.end.y - py;
	float V1Angle = atan2f(toV1y, toV1x) * 180.0f * M_1_PI;
	float V2Angle = atan2f(toV2y, toV2x) * 180.0f * M_1_PI;
	float V1ToV2Span = amod(V1Angle - V2Angle);

	if (V1ToV2Span >= 180) return;
	float V1AngleFromPlayer = amod(V1Angle - m_Angle); // Rotate every thing.
	float V2AngleFromPlayer = amod(V2Angle - m_Angle);
	if (amod(V1AngleFromPlayer + 45) > 90)
	{
		if (amod(V1AngleFromPlayer - 45) >= V1ToV2Span) return; // now we know that V1, is outside the left side of the FOV But we need to check is Also V2 is outside. Lets find out what is the size of the angle outside the FOV // Are both V1 and V2 outside?
		V1AngleFromPlayer = 45; // At this point V2 or part of the line should be in the FOV. We need to clip the V1
	}
	if (amod(45 - V2AngleFromPlayer) > 90) V2AngleFromPlayer = -45; // Validate and Clip V2 // Is V2 outside the FOV?
	
	auto AngleToScreen = [&](float angle) {
		return distancePlayerToScreen - round(tanf((angle) * M_PI / 180.0f) * halfRenderWidth);
	};

    const int V1XScreen = AngleToScreen(V1AngleFromPlayer), V2XScreen = AngleToScreen(V2AngleFromPlayer); // Find Wall X Coordinates
    if (V1XScreen == V2XScreen) return; // Skip same pixel wall
	bool solid = false;
    if (!seg.lSector) solid = true; // Handle solid walls
    else if (seg.lSector->ceilingHeight <= seg.rSector->floorHeight || seg.lSector->floorHeight >= seg.rSector->ceilingHeight) // Handle closed door
        solid = true;
//    else if (seg.rSector->ceilingHeight != seg.lSector->ceilingHeight || seg.rSector->floorHeight != seg.lSector->floorHeight) // Windowed walls
//        solid = false;
//	else return;

	if (solid && solidWallRanges.size() < 2) return;
    const SolidSegmentRange CurrentWall = { V1XScreen, V2XScreen }; // Find clip window
    std::list<SolidSegmentRange>::iterator FoundClipWall = solidWallRanges.begin();
    while (FoundClipWall != solidWallRanges.end() && CurrentWall.XStart - 1 > FoundClipWall->XEnd) ++FoundClipWall;

    if (CurrentWall.XStart < FoundClipWall->XStart)
    {
        if (CurrentWall.XEnd < FoundClipWall->XStart - 1)
        {
            storeWallRange(seg, CurrentWall.XStart, CurrentWall.XEnd, V1Angle, V2Angle); //All of the wall is visible, so insert it
            if (solid) solidWallRanges.insert(FoundClipWall, CurrentWall);
            return;
        }
        storeWallRange(seg, CurrentWall.XStart, FoundClipWall->XStart - 1, V1Angle, V2Angle); // The end is already included, just update start
        if (solid) FoundClipWall->XStart = CurrentWall.XStart;
    }
    
    if (CurrentWall.XEnd <= FoundClipWall->XEnd) return; // This part is already occupied
    std::list<SolidSegmentRange>::iterator NextWall = FoundClipWall;
    while (CurrentWall.XEnd >= next(NextWall, 1)->XStart - 1)
    {
        storeWallRange(seg, NextWall->XEnd + 1, next(NextWall, 1)->XStart - 1, V1Angle, V2Angle); // partialy clipped by other walls, store each fragment
        ++NextWall;
        if (CurrentWall.XEnd <= NextWall->XEnd)
        {
			if (solid)
			{
				FoundClipWall->XEnd = NextWall->XEnd;
				if (NextWall != FoundClipWall) solidWallRanges.erase(++FoundClipWall, ++NextWall);
			}
            return;
        }
    }
    storeWallRange(seg, NextWall->XEnd + 1, CurrentWall.XEnd, V1Angle, V2Angle);
	if (solid)
	{
		FoundClipWall->XEnd = CurrentWall.XEnd;
		if (NextWall != FoundClipWall) solidWallRanges.erase(++FoundClipWall, ++NextWall);
	}
}

void ViewRenderer::storeWallRange(Seg &seg, int V1XScreen, int V2XScreen, float V1Angle, float V2Angle)
{
	auto amod = [](float a) {
		return a - 360 * floor(a / 360);
	};

	bool bDrawUpperSection = false, bDrawLowerSection = false, UpdateFloor = false, UpdateCeiling = false;;
	float UpperHeightStep = 0, iUpperHeight = 0, LowerHeightStep = 0, iLowerHeight = 0;

	float SegToNormalAngle = amod(seg.slopeAngle + 90);

	float px = player->getX(), py = player->getY(), pa = player->getAngle();     // Calculate the distance between the player an the vertex.
	float DistanceToNormal = sin(M_PI * (V1Angle - seg.slopeAngle) / 180) * sqrt((px - seg.start.x) * (px - seg.start.x) + (py - seg.start.y) * (py - seg.start.y));

	auto GetScaleFactor = [&](int VXScreen) {
		float SkewAngle = amod(screenXToAngle[VXScreen] + pa - SegToNormalAngle);
		return std::clamp((distancePlayerToScreen * cosf(M_PI * SkewAngle / 180)) / (DistanceToNormal * cosf(M_PI * screenXToAngle[VXScreen] / 180)), 0.00390625f, 64.0f);
	};

    float V1ScaleFactor = GetScaleFactor(V1XScreen);
    float Steps = (GetScaleFactor(V2XScreen) - V1ScaleFactor) / (V2XScreen - V1XScreen);

    float RightSectorCeiling = seg.rSector->ceilingHeight - player->getZ();
    float RightSectorFloor = seg.rSector->floorHeight - player->getZ();

    float CeilingStep = -(RightSectorCeiling * Steps), CeilingEnd = round(halfRenderHeight - RightSectorCeiling * V1ScaleFactor);
    float FloorStep = -(RightSectorFloor * Steps), FloorStart = round(halfRenderHeight - RightSectorFloor * V1ScaleFactor);

    if (seg.lSector)
    {
        float LeftSectorCeiling = seg.lSector->ceilingHeight - player->getZ();
        float LeftSectorFloor = seg.lSector->floorHeight - player->getZ();

		UpdateCeiling = (LeftSectorCeiling != RightSectorCeiling);
		UpdateFloor = (LeftSectorFloor != RightSectorFloor);

		if (seg.lSector->ceilingHeight <= seg.rSector->floorHeight || seg.lSector->floorHeight >= seg.rSector->ceilingHeight) // closed door
			UpdateCeiling = UpdateFloor = true;
		if (seg.rSector->ceilingHeight <= player->getZ()) // below view plane
			UpdateCeiling = false;
		if (seg.rSector->floorHeight >= player->getZ()) // above view plane
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

	const float uA = py - seg.linedef->start.y, uB = seg.linedef->start.x - px, uC = seg.linedef->end.y - seg.linedef->start.y, uD = seg.linedef->start.x - seg.linedef->end.x;
	
	for (int x = V1XScreen; x <= V2XScreen; x++)
    {
		const float K = tan((screenXToAngle[x] + pa) * M_PI / 180);
		const float u = std::clamp((uA + K * uB) / (uC + K * uD), 0.f, 0.99999f);
				
        int CurrentCeilingEnd = std::max(CeilingEnd, ceilingClipHeight[x] + 1.f);
        int CurrentFloorStart = std::min(FloorStart, floorClipHeight[x] - 1.f);

		auto DrawTexture = [&](const Texture *texture, int x, int from, int to, float u, int cl, int fl) {
			if (!texture || to < from || fl <= cl) return;
			float scale = (fl - cl) / (float)texture->getHeight();
			texture->renderColumn(screenBuffer + rowlen * from + x, rowlen, texture->getWidth() * u, scale, (from - cl) / scale, (to - from));
		};
		
		
		if (CurrentCeilingEnd > CurrentFloorStart)
		{
			CeilingEnd += CeilingStep;
			FloorStart += FloorStep;
			continue;
		}
		
        if (seg.lSector)
        {
			for (int i = std::max(0, ceilingClipHeight[x]); i < CurrentCeilingEnd; i++)
				seg.rSector->ceilingtexture->renderSpan(screenBuffer + i * rowlen + x, 1, 0, 0, 0, 0);

			int upper = std::min(floorClipHeight[x] - 1.f, iUpperHeight);
			int lower = std::max(iLowerHeight, ceilingClipHeight[x] + 1.f);
			if (seg.linedef->rSidedef->middletexture && lower > upper && iLowerHeight > iUpperHeight)
				renderLaters.push_back({seg.linedef->rSidedef->middletexture, x, std::max(upper, 0), std::min(lower, renderHeight - 1), u, (int)iUpperHeight, (int)iLowerHeight});

			iUpperHeight += UpperHeightStep;
			iLowerHeight += LowerHeightStep;

			if (bDrawUpperSection)
			{
				DrawTexture(seg.linedef->rSidedef->uppertexture, x, CurrentCeilingEnd, upper, u, CeilingEnd, iUpperHeight);
				ceilingClipHeight[x] = std::max(CurrentCeilingEnd - 1, upper);
			}
			else if (UpdateCeiling)
				ceilingClipHeight[x] = CurrentCeilingEnd - 1;

			for (int i = CurrentFloorStart; i < floorClipHeight[x]; i++)
				seg.rSector->floortexture->renderSpan(screenBuffer + i * rowlen + x, 1, 0, 0, 0, 0);
			
			if (bDrawLowerSection)
			{
				DrawTexture(seg.linedef->rSidedef->lowertexture, x, lower, CurrentFloorStart, u, iLowerHeight, FloorStart);
				floorClipHeight[x] = std::min(CurrentFloorStart + 1, lower);
			}
			else if (UpdateFloor)
				floorClipHeight[x] = CurrentFloorStart + 1;
		}
        else
		{
			DrawTexture(seg.linedef->rSidedef->middletexture, x, CurrentCeilingEnd, CurrentFloorStart, u, CeilingEnd, FloorStart);
			for (int i = CurrentFloorStart; i < floorClipHeight[x]; i++)
				seg.rSector->floortexture->renderSpan(screenBuffer + i * rowlen + x, 1, 0, 0, 0, 0);
			for (int i = std::max(ceilingClipHeight[x], 0); i < CurrentCeilingEnd; i++)
				seg.rSector->ceilingtexture->renderSpan(screenBuffer + i * rowlen + x, 1, 0, 0, 0, 0);
			ceilingClipHeight[x] = renderHeight;
			floorClipHeight[x] = -1;
		}
        CeilingEnd += CeilingStep;
        FloorStart += FloorStep;
    }
}


