#include "ViewRenderer.h"

#include <math.h>
#include <iostream>

#include "Map.h"
#include "Texture.h"
#include "Player.hpp"
#include "AssetsManager.hpp"

constexpr float classicDoomScreenXtoView[] = {
  45.043945f, 44.824219f, 44.648437f, 44.472656f, 44.296875f, 44.121094f, 43.945312f, 43.725586f, 43.549805f, 43.374023f, 43.154297f, 42.978516f, 42.802734f, 42.583008f, 42.407227f, 42.187500f, 42.011719f, 41.791992f, 41.616211f, 41.396484f,
  41.220703f, 41.000977f, 40.781250f, 40.605469f, 40.385742f, 40.166016f, 39.946289f, 39.770508f, 39.550781f, 39.331055f, 39.111328f, 38.891602f, 38.671875f, 38.452148f, 38.232422f, 38.012695f, 37.792969f, 37.573242f, 37.353516f, 37.133789f,
  36.870117f, 36.650391f, 36.430664f, 36.210937f, 35.947266f, 35.727539f, 35.507812f, 35.244141f, 35.024414f, 34.760742f, 34.541016f, 34.277344f, 34.057617f, 33.793945f, 33.530273f, 33.310547f, 33.046875f, 32.783203f, 32.519531f, 32.299805f,
  32.036133f, 31.772461f, 31.508789f, 31.245117f, 30.981445f, 30.717773f, 30.454102f, 30.190430f, 29.926758f, 29.663086f, 29.355469f, 29.091797f, 28.828125f, 28.564453f, 28.256836f, 27.993164f, 27.729492f, 27.421875f, 27.158203f, 26.850586f,
  26.586914f, 26.279297f, 26.015625f, 25.708008f, 25.444336f, 25.136719f, 24.829102f, 24.521484f, 24.257812f, 23.950195f, 23.642578f, 23.334961f, 23.027344f, 22.719727f, 22.412109f, 22.104492f, 21.796875f, 21.489258f, 21.181641f, 20.874023f,
  20.566406f, 20.258789f, 19.951172f, 19.643555f, 19.291992f, 18.984375f, 18.676758f, 18.325195f, 18.017578f, 17.709961f, 17.358398f, 17.050781f, 16.699219f, 16.391602f, 16.040039f, 15.732422f, 15.380859f, 15.073242f, 14.721680f, 14.370117f,
  14.062500f, 13.710937f, 13.359375f, 13.051758f, 12.700195f, 12.348633f, 11.997070f, 11.645508f, 11.337891f, 10.986328f, 10.634766f, 10.283203f, 9.931641f, 9.580078f, 9.228516f, 8.876953f, 8.525391f, 8.173828f, 7.822266f, 7.470703f, 7.119141f,
  6.767578f, 6.416016f, 6.064453f, 5.712891f, 5.361328f, 5.009766f, 4.658203f, 4.306641f, 3.955078f, 3.559570f, 3.208008f, 2.856445f, 2.504883f, 2.153320f, 1.801758f, 1.450195f, 1.054687f, 0.703125f, 0.351562f, 0.000000f, 359.648437f, 359.296875f,
  358.945312f, 358.549805f, 358.198242f, 357.846680f, 357.495117f, 357.143555f, 356.791992f, 356.440430f, 356.044922f, 355.693359f, 355.341797f, 354.990234f, 354.638672f, 354.287109f, 353.935547f, 353.583984f, 353.232422f, 352.880859f, 352.529297f,
  352.177734f, 351.826172f, 351.474609f, 351.123047f, 350.771484f, 350.419922f, 350.068359f, 349.716797f, 349.365234f, 349.013672f, 348.662109f, 348.354492f, 348.002930f, 347.651367f, 347.299805f, 346.948242f, 346.640625f, 346.289062f, 345.937500f,
  345.629883f, 345.278320f, 344.926758f, 344.619141f, 344.267578f, 343.959961f, 343.608398f, 343.300781f, 342.949219f, 342.641601f, 342.290039f, 341.982422f, 341.674805f, 341.323242f, 341.015625f, 340.708008f, 340.356445f, 340.048828f, 339.741211f,
  339.433594f, 339.125977f, 338.818359f, 338.510742f, 338.203125f, 337.895508f, 337.587891f, 337.280273f, 336.972656f, 336.665039f, 336.357422f, 336.049805f, 335.742187f, 335.478516f, 335.170898f, 334.863281f, 334.555664f, 334.291992f, 333.984375f,
  333.720703f, 333.413086f, 333.149414f, 332.841797f, 332.578125f, 332.270508f, 332.006836f, 331.743164f, 331.435547f, 331.171875f, 330.908203f, 330.644531f, 330.336914f, 330.073242f, 329.809570f, 329.545898f, 329.282227f, 329.018555f, 328.754883f,
  328.491211f, 328.227539f, 327.963867f, 327.700195f, 327.480469f, 327.216797f, 326.953125f, 326.689453f, 326.469727f, 326.206055f, 325.942383f, 325.722656f, 325.458984f, 325.239258f, 324.975586f, 324.755859f, 324.492187f, 324.272461f, 324.052734f,
  323.789062f, 323.569336f, 323.349609f, 323.129883f, 322.866211f, 322.646484f, 322.426758f, 322.207031f, 321.987305f, 321.767578f, 321.547852f, 321.328125f, 321.108398f, 320.888672f, 320.668945f, 320.449219f, 320.229492f, 320.053711f, 319.833984f,
  319.614258f, 319.394531f, 319.218750f, 318.999023f, 318.779297f, 318.603516f, 318.383789f, 318.208008f, 317.988281f, 317.812500f, 317.592773f, 317.416992f, 317.197266f, 317.021484f, 316.845703f, 316.625977f, 316.450195f, 316.274414f, 316.054687f,
  315.878906f, 315.703125f, 315.527344f, 315.351562f, 315.175781f, 314.956055f };

ViewRenderer::ViewRenderer() : m_UseClassicDoomScreenToAngle(true)
{
}

ViewRenderer::~ViewRenderer()
{
}

void  ViewRenderer::Init(Map *pMap, Player *pPlayer)
{
    m_pMap = pMap;
    m_pPlayer = pPlayer;

    m_iRenderXSize = 320;
    m_iRenderYSize = 200;

    m_HalfScreenWidth = m_iRenderXSize / 2;
    m_HalfScreenHeight = m_iRenderYSize / 2;
	Angle HalfFOV = 45;
    m_iDistancePlayerToScreen = m_HalfScreenWidth / HalfFOV.GetTanValue();

    for (int i = 0; i <= m_iRenderXSize; ++i)
    {
        if (m_UseClassicDoomScreenToAngle)
        {
            m_ScreenXToAngle[i] = classicDoomScreenXtoView[i];
        }
        else
        {
            m_ScreenXToAngle[i] = atan((m_HalfScreenWidth - i) / (float)m_iDistancePlayerToScreen) * 180 / PI;
        }
    }

    m_CeilingClipHeight.resize(m_iRenderXSize);
    m_FloorClipHeight.resize(m_iRenderXSize);
}

void ViewRenderer::Render(uint8_t *pScreenBuffer, int iBufferPitch)
{
	m_pScreenBuffer = pScreenBuffer;
	m_iBufferPitch = iBufferPitch;

    InitFrame();
	m_pMap->Render3DView();
}

void ViewRenderer::InitFrame()
{
    m_SolidWallRanges.clear();

    SolidSegmentRange WallLeftSide;
    SolidSegmentRange WallRightSide;

    WallLeftSide.XStart = INT_MIN;
    WallLeftSide.XEnd = -1;
    m_SolidWallRanges.push_back(WallLeftSide);

    WallRightSide.XStart = m_iRenderXSize;
    WallRightSide.XEnd = INT_MAX;
    m_SolidWallRanges.push_back(WallRightSide);

    std::fill(m_CeilingClipHeight.begin(), m_CeilingClipHeight.end(), -1);
    std::fill(m_FloorClipHeight.begin(), m_FloorClipHeight.end(), m_iRenderYSize);
}

void ViewRenderer::AddWallInFOV(Seg &seg, Angle V1Angle, Angle V2Angle, Angle V1AngleFromPlayer, Angle V2AngleFromPlayer)
{
    int V1XScreen = AngleToScreen(V1AngleFromPlayer), V2XScreen = AngleToScreen(V2AngleFromPlayer); // Find Wall X Coordinates
    if (V1XScreen == V2XScreen) return; // Skip same pixel wall
    if (!seg.pLeftSector) ClipSolidWalls(seg, V1XScreen, V2XScreen, V1Angle, V2Angle); // Handle solid walls
    else if (seg.pLeftSector->CeilingHeight <= seg.pRightSector->FloorHeight || seg.pLeftSector->FloorHeight >= seg.pRightSector->CeilingHeight) // Handle closed door
        ClipSolidWalls(seg, V1XScreen, V2XScreen, V1Angle, V2Angle);
    else if (seg.pRightSector->CeilingHeight != seg.pLeftSector->CeilingHeight || seg.pRightSector->FloorHeight != seg.pLeftSector->FloorHeight) // Windowed walls
        ClipPassWalls(seg, V1XScreen, V2XScreen, V1Angle, V2Angle);
}

void ViewRenderer::ClipSolidWalls(Seg &seg, int V1XScreen, int V2XScreen, Angle V1Angle, Angle V2Angle)
{
    if (m_SolidWallRanges.size() < 2)
    {
        return;
    }
    // Find clip window 
    SolidSegmentRange CurrentWall = { V1XScreen, V2XScreen };

    std::list<SolidSegmentRange>::iterator FoundClipWall = m_SolidWallRanges.begin();
    while (FoundClipWall != m_SolidWallRanges.end() && FoundClipWall->XEnd < CurrentWall.XStart - 1)
    {
        ++FoundClipWall;
    }

    if (CurrentWall.XStart < FoundClipWall->XStart)
    {
        if (CurrentWall.XEnd < FoundClipWall->XStart - 1)
        {
            //All of the wall is visible, so insert it
            StoreWallRange(seg, CurrentWall.XStart, CurrentWall.XEnd, V1Angle, V2Angle);
            m_SolidWallRanges.insert(FoundClipWall, CurrentWall);
            return;
        }

        // The end is already included, just update start
        StoreWallRange(seg, CurrentWall.XStart, FoundClipWall->XStart - 1, V1Angle, V2Angle);
        FoundClipWall->XStart = CurrentWall.XStart;
    }

    // This part is already occupied
    if (CurrentWall.XEnd <= FoundClipWall->XEnd)
        return;

    std::list<SolidSegmentRange>::iterator NextWall = FoundClipWall;

    while (CurrentWall.XEnd >= next(NextWall, 1)->XStart - 1)
    {
        // partialy clipped by other walls, store each fragment
        StoreWallRange(seg, NextWall->XEnd + 1, next(NextWall, 1)->XStart - 1, V1Angle, V2Angle);
        ++NextWall;

        if (CurrentWall.XEnd <= NextWall->XEnd)
        {
            FoundClipWall->XEnd = NextWall->XEnd;
            if (NextWall != FoundClipWall)
            {
                FoundClipWall++;
                NextWall++;
                m_SolidWallRanges.erase(FoundClipWall, NextWall);
            }
            return;
        }
    }

    StoreWallRange(seg, NextWall->XEnd + 1, CurrentWall.XEnd, V1Angle, V2Angle);
    FoundClipWall->XEnd = CurrentWall.XEnd;

    if (NextWall != FoundClipWall)
    {
        FoundClipWall++;
        NextWall++;
        m_SolidWallRanges.erase(FoundClipWall, NextWall);
    }
}

void ViewRenderer::ClipPassWalls(Seg &seg, int V1XScreen, int V2XScreen, Angle V1Angle, Angle V2Angle)
{
    // Find clip window 
    SolidSegmentRange CurrentWall = { V1XScreen, V2XScreen };

    std::list<SolidSegmentRange>::iterator FoundClipWall = m_SolidWallRanges.begin();
    while (FoundClipWall != m_SolidWallRanges.end() && FoundClipWall->XEnd < CurrentWall.XStart - 1)
    {
        ++FoundClipWall;
    }

    if (CurrentWall.XStart < FoundClipWall->XStart)
    {
        if (CurrentWall.XEnd < FoundClipWall->XStart - 1)
        {
            //All of the wall is visible, so insert it
            StoreWallRange(seg, CurrentWall.XStart, CurrentWall.XEnd, V1Angle, V2Angle);
            return;
        }

        // The end is already included, just update start
        StoreWallRange(seg, CurrentWall.XStart, FoundClipWall->XStart - 1, V1Angle, V2Angle);
    }

    // This part is already occupied
    if (CurrentWall.XEnd <= FoundClipWall->XEnd)
        return;

    std::list<SolidSegmentRange>::iterator NextWall = FoundClipWall;

    while (CurrentWall.XEnd >= next(NextWall, 1)->XStart - 1)
    {
        // partialy clipped by other walls, store each fragment
        StoreWallRange(seg, NextWall->XEnd + 1, next(NextWall, 1)->XStart - 1, V1Angle, V2Angle);
        ++NextWall;

        if (CurrentWall.XEnd <= NextWall->XEnd)
        {
            return;
        }
    }

    StoreWallRange(seg, NextWall->XEnd + 1, CurrentWall.XEnd, V1Angle, V2Angle);
}

void ViewRenderer::StoreWallRange(Seg &seg, int V1XScreen, int V2XScreen, Angle V1Angle, Angle V2Angle)
{
    // For now just lets sotre the range 
    CalculateWallHeight(seg, V1XScreen, V2XScreen, V1Angle, V2Angle);
}

void ViewRenderer::CalculateWallHeight(Seg &seg, int V1XScreen, int V2XScreen, Angle V1Angle, Angle V2Angle)
{
    // Calculate the distance to the first edge of the wall
    Angle Angle90(90);
    SegmentRenderData RenderData { 0 };
    Angle SegToNormalAngle = seg.SlopeAngle + Angle90;

    //Angle NomalToV1Angle = abs(SegToNormalAngle.GetSignedValue() - V1Angle.GetSignedValue());
    Angle NomalToV1Angle = SegToNormalAngle.GetValue() - V1Angle.GetValue();

    // Normal angle is 90 degree to wall
    Angle SegToPlayerAngle = Angle90 - NomalToV1Angle;

	RenderData.V1XScreen = V1XScreen;
	RenderData.V2XScreen = V2XScreen;
	RenderData.V1Angle = V1Angle;
	RenderData.V2Angle = V2Angle;

    RenderData.DistanceToV1 = m_pPlayer->DistanceToPoint(seg.pStartVertex);
    RenderData.DistanceToNormal = SegToPlayerAngle.GetSinValue() * RenderData.DistanceToV1;

    RenderData.V1ScaleFactor = GetScaleFactor(V1XScreen, SegToNormalAngle, RenderData.DistanceToNormal);
    RenderData.V2ScaleFactor = GetScaleFactor(V2XScreen, SegToNormalAngle, RenderData.DistanceToNormal);

    RenderData.Steps = (RenderData.V2ScaleFactor - RenderData.V1ScaleFactor) / (V2XScreen - V1XScreen);

    RenderData.RightSectorCeiling = seg.pRightSector->CeilingHeight - m_pPlayer->GetZPosition();
    RenderData.RightSectorFloor = seg.pRightSector->FloorHeight - m_pPlayer->GetZPosition();

    RenderData.CeilingStep = -(RenderData.RightSectorCeiling * RenderData.Steps);
    RenderData.CeilingEnd = round(m_HalfScreenHeight - (RenderData.RightSectorCeiling * RenderData.V1ScaleFactor));

    RenderData.FloorStep = -(RenderData.RightSectorFloor * RenderData.Steps);
    RenderData.FloorStart = round(m_HalfScreenHeight - (RenderData.RightSectorFloor * RenderData.V1ScaleFactor));

	RenderData.pSeg = &seg;

    if (seg.pLeftSector)
    {
        RenderData.LeftSectorCeiling = seg.pLeftSector->CeilingHeight - m_pPlayer->GetZPosition();
        RenderData.LeftSectorFloor = seg.pLeftSector->FloorHeight - m_pPlayer->GetZPosition();

        CeilingFloorUpdate(RenderData);

        if (RenderData.LeftSectorCeiling < RenderData.RightSectorCeiling)
        {
            RenderData.bDrawUpperSection = true;
            RenderData.UpperHeightStep = -(RenderData.LeftSectorCeiling * RenderData.Steps);
            RenderData.iUpperHeight = round(m_HalfScreenHeight - (RenderData.LeftSectorCeiling * RenderData.V1ScaleFactor));
        }

        if (RenderData.LeftSectorFloor > RenderData.RightSectorFloor)
        {
            RenderData.bDrawLowerSection = true;
            RenderData.LowerHeightStep = -(RenderData.LeftSectorFloor * RenderData.Steps);
            RenderData.iLowerHeight = round(m_HalfScreenHeight - (RenderData.LeftSectorFloor * RenderData.V1ScaleFactor));
        }
    }

    RenderSegment(RenderData);
}

void ViewRenderer::CeilingFloorUpdate(SegmentRenderData &RenderData)
{
    if (!RenderData.pSeg->pLeftSector)
    {
        RenderData.UpdateFloor = true;
        RenderData.UpdateCeiling = true;
        return;
    }

    if (RenderData.LeftSectorCeiling != RenderData.RightSectorCeiling)
    {
        RenderData.UpdateCeiling = true;
    }
    else
    {
        RenderData.UpdateCeiling = false;
    }

    if (RenderData.LeftSectorFloor != RenderData.RightSectorFloor)
    {
        RenderData.UpdateFloor = true;
    }
    else
    {
        RenderData.UpdateFloor = false;
    }

    if (RenderData.pSeg->pLeftSector->CeilingHeight <= RenderData.pSeg->pRightSector->FloorHeight || RenderData.pSeg->pLeftSector->FloorHeight >= RenderData.pSeg->pRightSector->CeilingHeight)
    {
        // closed door
        RenderData.UpdateCeiling = RenderData.UpdateFloor = true;
    }

    if (RenderData.pSeg->pRightSector->CeilingHeight <= m_pPlayer->GetZPosition())
    {
        // below view plane
        RenderData.UpdateCeiling = false;
    }

    if (RenderData.pSeg->pRightSector->FloorHeight >= m_pPlayer->GetZPosition())
    {
        // above view plane
        RenderData.UpdateFloor = false;
    }
}

float ViewRenderer::GetScaleFactor(int VXScreen, Angle SegToNormalAngle, float DistanceToNormal)
{
    static float MAX_SCALEFACTOR = 64.0f;
    static float MIN_SCALEFACTOR = 0.00390625f;

    Angle Angle90(90);

    Angle ScreenXAngle = m_ScreenXToAngle[VXScreen];
    Angle SkewAngle = m_ScreenXToAngle[VXScreen] + m_pPlayer->GetAngle() - SegToNormalAngle;

    float ScreenXAngleCos = ScreenXAngle.GetCosValue();
    float SkewAngleCos = SkewAngle.GetCosValue();
    float ScaleFactor = (m_iDistancePlayerToScreen * SkewAngleCos) / (DistanceToNormal * ScreenXAngleCos);

    if (ScaleFactor > MAX_SCALEFACTOR)
    {
        ScaleFactor = MAX_SCALEFACTOR;
    }
    else if (MIN_SCALEFACTOR > ScaleFactor)
    {
        ScaleFactor = MIN_SCALEFACTOR;
    }

    return ScaleFactor;
}

void ViewRenderer::CalculateCeilingFloorHeight(Seg &seg, int &VXScreen, float &DistanceToV, float &CeilingVOnScreen, float &FloorVOnScreen)
{
    float Ceiling = seg.pRightSector->CeilingHeight - m_pPlayer->GetZPosition();
    float Floor = seg.pRightSector->FloorHeight - m_pPlayer->GetZPosition();

    Angle VScreenAngle = m_ScreenXToAngle[VXScreen];

    float DistanceToVScreen = m_iDistancePlayerToScreen / VScreenAngle.GetCosValue();

    CeilingVOnScreen = (abs(Ceiling) * DistanceToVScreen) / DistanceToV;
    FloorVOnScreen = (abs(Floor) * DistanceToVScreen) / DistanceToV;

    if (Ceiling > 0)
    {
        CeilingVOnScreen = m_HalfScreenHeight - CeilingVOnScreen;
    }
    else
    {
        CeilingVOnScreen += m_HalfScreenHeight;
    }

    if (Floor > 0)
    {
        FloorVOnScreen = m_HalfScreenHeight - FloorVOnScreen;
    }
    else
    {
        FloorVOnScreen += m_HalfScreenHeight;
    }
}

void ViewRenderer::PartialSeg(Seg &seg, Angle &V1Angle, Angle &V2Angle, float &DistanceToV, bool IsLeftSide)
{
    float SideC = sqrt(pow(seg.pStartVertex.XPosition - seg.pEndVertex.XPosition, 2) + pow(seg.pStartVertex.YPosition - seg.pEndVertex.YPosition, 2));
    Angle V1toV2Span = V1Angle - V2Angle;
    float SINEAngleB = DistanceToV * V1toV2Span.GetSinValue() / SideC;
    Angle AngleB(asinf(SINEAngleB) * 180.0 / PI);
    Angle AngleA(180 - V1toV2Span.GetValue() - AngleB.GetValue());

    Angle AngleVToFOV;
    if (IsLeftSide)
    {
        AngleVToFOV = V1Angle - (m_pPlayer->GetAngle() + 45);
    }
    else
    {
        AngleVToFOV = (m_pPlayer->GetAngle() - 45) - V2Angle;
    }

    Angle NewAngleB(180 - AngleVToFOV.GetValue() - AngleA.GetValue());
    DistanceToV = DistanceToV * AngleA.GetSinValue() / NewAngleB.GetSinValue();
}

void ViewRenderer::RenderSegment(SegmentRenderData &RenderData)
{
    int iXCurrent = RenderData.V1XScreen;

    while (iXCurrent <= RenderData.V2XScreen)
    {
        int CurrentCeilingEnd = RenderData.CeilingEnd;
        int CurrentFloorStart = RenderData.FloorStart;

        if (!ValidateRange(RenderData, iXCurrent, CurrentCeilingEnd, CurrentFloorStart))
        {
            continue;
        }

        if (RenderData.pSeg->pLeftSector)
        {
            DrawUpperSection(RenderData, iXCurrent, CurrentCeilingEnd);
            DrawLowerSection(RenderData, iXCurrent, CurrentFloorStart);
        }
        else
        {
            DrawMiddleSection(RenderData, iXCurrent, CurrentCeilingEnd, CurrentFloorStart);
        }

        RenderData.CeilingEnd += RenderData.CeilingStep;
        RenderData.FloorStart += RenderData.FloorStep;
        ++iXCurrent;
    }
}

void ViewRenderer::DrawMiddleSection(ViewRenderer::SegmentRenderData &RenderData, int iXCurrent, int CurrentCeilingEnd, int CurrentFloorStart)
{
	uint8_t color = GetSectionColor(RenderData.pSeg->pLinedef->pRightSidedef->MiddleTexture);
	DrawVerticalLine(iXCurrent, CurrentCeilingEnd, CurrentFloorStart, color);
    m_CeilingClipHeight[iXCurrent] = m_iRenderYSize;
    m_FloorClipHeight[iXCurrent] = -1;
}

void ViewRenderer::DrawLowerSection(ViewRenderer::SegmentRenderData &RenderData, int iXCurrent, int CurrentFloorStart)
{
    if (RenderData.bDrawLowerSection)
    {
		int iLowerHeight = RenderData.iLowerHeight;
        RenderData.iLowerHeight += RenderData.LowerHeightStep;

        if (iLowerHeight <= m_CeilingClipHeight[iXCurrent])
        {
            iLowerHeight = m_CeilingClipHeight[iXCurrent] + 1;
        }

        if (iLowerHeight <= CurrentFloorStart)
        {
			uint8_t color = GetSectionColor(RenderData.pSeg->pLinedef->pRightSidedef->LowerTexture);
			DrawVerticalLine(iXCurrent, iLowerHeight, CurrentFloorStart, color);
            m_FloorClipHeight[iXCurrent] = iLowerHeight;
        }
        else
            m_FloorClipHeight[iXCurrent] = CurrentFloorStart + 1;
    }
    else if (RenderData.UpdateFloor)
    {
        m_FloorClipHeight[iXCurrent] = CurrentFloorStart + 1;
    }
}

void ViewRenderer::DrawUpperSection(ViewRenderer::SegmentRenderData &RenderData, int iXCurrent, int CurrentCeilingEnd)
{
    if (RenderData.bDrawUpperSection)
    {
        int iUpperHeight = RenderData.iUpperHeight;
        RenderData.iUpperHeight += RenderData.UpperHeightStep;

        if (iUpperHeight >= m_FloorClipHeight[iXCurrent])
        {
            iUpperHeight = m_FloorClipHeight[iXCurrent] - 1;
        }

        if (iUpperHeight >= CurrentCeilingEnd)
        {
			uint8_t color = GetSectionColor(RenderData.pSeg->pLinedef->pRightSidedef->UpperTexture);
			DrawVerticalLine(iXCurrent, CurrentCeilingEnd, iUpperHeight, color);
            m_CeilingClipHeight[iXCurrent] = iUpperHeight;
        }
        else
        {
            m_CeilingClipHeight[iXCurrent] = CurrentCeilingEnd - 1;
        }
    }
    else if (RenderData.UpdateCeiling)
    {
        m_CeilingClipHeight[iXCurrent] = CurrentCeilingEnd - 1;
    }
}

bool ViewRenderer::ValidateRange(ViewRenderer::SegmentRenderData & RenderData, int &iXCurrent, int &CurrentCeilingEnd, int &CurrentFloorStart)
{
    if (CurrentCeilingEnd < m_CeilingClipHeight[iXCurrent] + 1)
    {
        CurrentCeilingEnd = m_CeilingClipHeight[iXCurrent] + 1;
    }

    if (CurrentFloorStart >= m_FloorClipHeight[iXCurrent])
    {
        CurrentFloorStart = m_FloorClipHeight[iXCurrent] - 1;
    }

    if (CurrentCeilingEnd > CurrentFloorStart)
    {
        RenderData.CeilingEnd += RenderData.CeilingStep;
        RenderData.FloorStart += RenderData.FloorStep;
        ++iXCurrent;
        return false;
    }
    return true;
}

int ViewRenderer::AngleToScreen(Angle angle)
{
    int iX = 0;

    if (angle > 90)
    {
        angle -= 90;
        iX = m_iDistancePlayerToScreen - round(angle.GetTanValue() * m_HalfScreenWidth);
    }
    else
    {
        angle = 90 - angle.GetValue();
        iX = round(angle.GetTanValue() * m_HalfScreenWidth);
        iX += m_iDistancePlayerToScreen;
    }

    return iX;
}

uint8_t ViewRenderer::GetSectionColor(const std::string &TextureName)
{
    uint8_t color;
    if (m_WallColor.count(TextureName))
    {
        color = m_WallColor[TextureName];
    }
    else
    {
        color = rand() % 256;
        m_WallColor[TextureName] = color;
    };

    return color;
}

void ViewRenderer::DrawVerticalLine(int iX, int iStartY, int iEndY, uint8_t color)
{
	//int iStartY = line->y1;
	while (iStartY < iEndY)
	{
		m_pScreenBuffer[m_iBufferPitch * iStartY + iX] = color;
		++iStartY;
	}
}
