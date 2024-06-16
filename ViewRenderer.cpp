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
	solidWallRanges.clear();
	solidWallRanges.push_back({INT_MIN, -1});
	solidWallRanges.push_back({renderWidth, INT_MAX});
	std::fill(ceilingClipHeight.begin(), ceilingClipHeight.end(), -1);
	std::fill(floorClipHeight.begin(), floorClipHeight.end(), renderHeight);

	map->render3DView();
}

void ViewRenderer::addWallInFOV(Seg &seg)
{
	const float m_Angle = player->getAngle().get();
	const int px = player->getX(), py = player->getY();

	auto amod = [](float a) {
		return a - 360 * floor(a / 360);
	};

	int toV1x = seg.start.x - px, toV1y = seg.start.y - py, toV2x = seg.end.x - px, toV2y = seg.end.y - py;
	float V1Angle = atan2f(toV1y, toV1x) * 180.0f / M_PI;
	float V2Angle = atan2f(toV2y, toV2x) * 180.0f / M_PI;
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
    else if (seg.rSector->ceilingHeight != seg.lSector->ceilingHeight || seg.rSector->floorHeight != seg.lSector->floorHeight) // Windowed walls
        solid = false;
	else return;

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

void ViewRenderer::storeWallRange(Seg &seg, int V1XScreen, int V2XScreen, Angle V1Angle, Angle V2Angle)
{
	auto GetScaleFactor = [&](int VXScreen, Angle SegToNormalAngle, float DistanceToNormal) {
		Angle SkewAngle = screenXToAngle[VXScreen] + player->getAngle() - SegToNormalAngle;
		return std::clamp((distancePlayerToScreen * SkewAngle.cos()) / (DistanceToNormal * screenXToAngle[VXScreen].cos()), 0.00390625f, 64.0f);
	};

    // Calculate the distance to the first edge of the wall
    Angle Angle90(90);
	
	struct SegmentRenderData
	{
		Angle V1Angle;
		Angle V2Angle;
		float DistanceToV1;
		float DistanceToNormal;
		float V1ScaleFactor;
		float V2ScaleFactor;
		float Steps;

		float RightSectorCeiling;
		float RightSectorFloor;
		float CeilingStep;
		float CeilingEnd;
		float FloorStep;
		float FloorStart;

		float LeftSectorCeiling;
		float LeftSectorFloor;

		bool bDrawUpperSection;
		bool bDrawLowerSection;

		float UpperHeightStep;
		float iUpperHeight;
		float LowerHeightStep;
		float iLowerHeight;

		bool UpdateFloor;
		bool UpdateCeiling;
	};
	
    SegmentRenderData RenderData { 0 };
    Angle SegToNormalAngle = Angle(seg.slopeAngle) + Angle90;

    Angle NomalToV1Angle = SegToNormalAngle.get() - V1Angle.get();

    // Normal angle is 90 degree to wall
    Angle SegToPlayerAngle = Angle90 - NomalToV1Angle;

	RenderData.V1Angle = V1Angle;
	RenderData.V2Angle = V2Angle;

	float px = player->getX(), py = player->getY();     // Calculate the distance between the player an the vertex.
	RenderData.DistanceToV1 = sqrt((px - seg.start.x) * (px - seg.start.x) + (py - seg.start.y) * (py - seg.start.y));
    RenderData.DistanceToNormal = SegToPlayerAngle.sin() * RenderData.DistanceToV1;

    RenderData.V1ScaleFactor = GetScaleFactor(V1XScreen, SegToNormalAngle, RenderData.DistanceToNormal);
    RenderData.V2ScaleFactor = GetScaleFactor(V2XScreen, SegToNormalAngle, RenderData.DistanceToNormal);

    RenderData.Steps = (RenderData.V2ScaleFactor - RenderData.V1ScaleFactor) / (V2XScreen - V1XScreen);

    RenderData.RightSectorCeiling = seg.rSector->ceilingHeight - player->getZ();
    RenderData.RightSectorFloor = seg.rSector->floorHeight - player->getZ();

    RenderData.CeilingStep = -(RenderData.RightSectorCeiling * RenderData.Steps);
    RenderData.CeilingEnd = round(halfRenderHeight - (RenderData.RightSectorCeiling * RenderData.V1ScaleFactor));

    RenderData.FloorStep = -(RenderData.RightSectorFloor * RenderData.Steps);
    RenderData.FloorStart = round(halfRenderHeight - (RenderData.RightSectorFloor * RenderData.V1ScaleFactor));

    if (seg.lSector)
    {
        RenderData.LeftSectorCeiling = seg.lSector->ceilingHeight - player->getZ();
        RenderData.LeftSectorFloor = seg.lSector->floorHeight - player->getZ();

		if (!seg.lSector)
		{
			RenderData.UpdateFloor = true;
			RenderData.UpdateCeiling = true;
			return;
		}

		RenderData.UpdateCeiling = (RenderData.LeftSectorCeiling != RenderData.RightSectorCeiling);
		RenderData.UpdateFloor = (RenderData.LeftSectorFloor != RenderData.RightSectorFloor);

		if (seg.lSector->ceilingHeight <= seg.rSector->floorHeight || seg.lSector->floorHeight >= seg.rSector->ceilingHeight) // closed door
			RenderData.UpdateCeiling = RenderData.UpdateFloor = true;
		if (seg.rSector->ceilingHeight <= player->getZ()) // below view plane
			RenderData.UpdateCeiling = false;
		if (seg.rSector->floorHeight >= player->getZ()) // above view plane
			RenderData.UpdateFloor = false;

        if (RenderData.LeftSectorCeiling < RenderData.RightSectorCeiling)
        {
            RenderData.bDrawUpperSection = true;
            RenderData.UpperHeightStep = -(RenderData.LeftSectorCeiling * RenderData.Steps);
            RenderData.iUpperHeight = round(halfRenderHeight - (RenderData.LeftSectorCeiling * RenderData.V1ScaleFactor));
        }

        if (RenderData.LeftSectorFloor > RenderData.RightSectorFloor)
        {
            RenderData.bDrawLowerSection = true;
            RenderData.LowerHeightStep = -(RenderData.LeftSectorFloor * RenderData.Steps);
            RenderData.iLowerHeight = round(halfRenderHeight - (RenderData.LeftSectorFloor * RenderData.V1ScaleFactor));
        }
    }

	auto DrawTexture = [&](const Texture *texture, int x, int from, int to) {
		if (!texture) return;

		int width = texture->getWidth();
		float scale = (to - from) / (float)texture->getHeight();
		texture->renderColumn(screenBuffer + rowlen * from + x, rowlen, 0, scale);
	};
	
	for (int x = V1XScreen; x <= V2XScreen; x++)
    {
        int CurrentCeilingEnd = std::max(RenderData.CeilingEnd, ceilingClipHeight[x] + 1.f);
        int CurrentFloorStart = std::min(RenderData.FloorStart, floorClipHeight[x] - 1.f);

		if (CurrentCeilingEnd > CurrentFloorStart)
		{
			RenderData.CeilingEnd += RenderData.CeilingStep;
			RenderData.FloorStart += RenderData.FloorStep;
			continue;
		}

        if (seg.lSector)
        {
			if (RenderData.bDrawUpperSection)
			{
				int iUpperHeight = std::min(floorClipHeight[x] - 1.f, RenderData.iUpperHeight);
				RenderData.iUpperHeight += RenderData.UpperHeightStep;
				if (iUpperHeight >= CurrentCeilingEnd)
					DrawTexture(seg.linedef->rSidedef->uppertexture, x, CurrentCeilingEnd, iUpperHeight);
				ceilingClipHeight[x] = std::max(CurrentCeilingEnd - 1, iUpperHeight);
			}
			else if (RenderData.UpdateCeiling)
				ceilingClipHeight[x] = CurrentCeilingEnd - 1;

			if (RenderData.bDrawLowerSection)
			{
				int iLowerHeight = std::max(RenderData.iLowerHeight, ceilingClipHeight[x] + 1.f);
				RenderData.iLowerHeight += RenderData.LowerHeightStep;
				if (iLowerHeight <= CurrentFloorStart)
					DrawTexture(seg.linedef->rSidedef->lowertexture, x, iLowerHeight, CurrentFloorStart);
				floorClipHeight[x] = std::min(CurrentFloorStart + 1, iLowerHeight);
			}
			else if (RenderData.UpdateFloor)
				floorClipHeight[x] = CurrentFloorStart + 1;
		}
        else
		{
			DrawTexture(seg.linedef->rSidedef->middletexture, x, CurrentCeilingEnd, CurrentFloorStart);
			ceilingClipHeight[x] = renderHeight;
			floorClipHeight[x] = -1;

		}
        RenderData.CeilingEnd += RenderData.CeilingStep;
        RenderData.FloorStart += RenderData.FloorStep;
    }
}


