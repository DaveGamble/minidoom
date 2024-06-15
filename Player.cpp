#include "Player.h"
#include "AssetsManager.h"
#include <math.h>

void Player::Init(Thing* thing, AssetsManager *assets)
{
	m_pWeapon = assets->GetPatch("PISGA0");
	if (thing)
	{
		m_XPosition = thing->XPosition;
		m_YPosition = thing->YPosition;
		m_Angle = thing->Angle;
	}
}

bool Player::ClipVertexesInFOV(Vertex &V1, Vertex &V2, Angle &V1Angle, Angle &V2Angle, Angle &V1AngleFromPlayer, Angle &V2AngleFromPlayer)
{
    V1Angle = AngleToVertex(V1);
    V2Angle = AngleToVertex(V2);

    Angle V1ToV2Span = V1Angle - V2Angle;

    if (V1ToV2Span >= 180) return false;
    V1AngleFromPlayer = V1Angle - m_Angle; // Rotate every thing.
    V2AngleFromPlayer = V2Angle - m_Angle;
    
    Angle V1Moved = V1AngleFromPlayer + m_HalfFOV; // Validate and Clip V1. shift angles to be between 0 and 90 (now virtualy we shifted FOV to be in that range)

    if (V1Moved > m_FOV)
    {
		if (V1Moved - m_FOV >= V1ToV2Span) return false; // now we know that V1, is outside the left side of the FOV But we need to check is Also V2 is outside. Lets find out what is the size of the angle outside the FOV // Are both V1 and V2 outside?
        V1AngleFromPlayer = m_HalfFOV; // At this point V2 or part of the line should be in the FOV. We need to clip the V1
    }
    if (m_HalfFOV - V2AngleFromPlayer > m_FOV) V2AngleFromPlayer = -m_HalfFOV; // Validate and Clip V2 // Is V2 outside the FOV?

    V1AngleFromPlayer += 90;
    V2AngleFromPlayer += 90;

    return true;
}
