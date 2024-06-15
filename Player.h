#pragma once

#include "Angle.hpp"
#include "DataTypes.h"
#include "ViewRenderer.h"
#include "Patch.h"

class Player
{
public:
	Player(int iID = 1) : m_iPlayerID(iID) {}
	~Player() {}

    void Init(Thing *thing, class AssetsManager *assets);
    void MoveForward() { m_XPosition += m_Angle.GetCosValue() * m_iMoveSpeed; m_YPosition += m_Angle.GetSinValue() * m_iMoveSpeed; }
    void MoveLeftward() { m_XPosition -= m_Angle.GetCosValue() * m_iMoveSpeed; m_YPosition -= m_Angle.GetSinValue() * m_iMoveSpeed; }
	void RotateLeft() { m_Angle += (0.1875f * m_iRotationSpeed); }
	void RotateRight() { m_Angle -= (0.1875f * m_iRotationSpeed); }
	void Fly() { m_ZPosition += 1; }
	void Sink() { m_ZPosition -= 1; }
	void Think(int iSubSectorHieght) { m_ZPosition = iSubSectorHieght + m_EyeLevel; }
	void Render(uint8_t *pScreenBuffer, int iBufferPitch) { m_pWeapon->Render(pScreenBuffer, iBufferPitch, -m_pWeapon->GetXOffset(), -m_pWeapon->GetYOffset()); }

	int GetID() const { return m_iPlayerID; }
	int GetXPosition() const { return m_XPosition; }
	int GetYPosition() const { return m_YPosition; }
	int GetZPosition() const { return m_ZPosition; }
	int GetFOV() { return m_FOV; }

    bool ClipVertexesInFOV(Vertex &V1, Vertex &V2, Angle &V1Angle, Angle &V2Angle, Angle &V1AngleFromPlayer, Angle &V2AngleFromPlayer);

    // Calulate the distance between the player an the vertex.
	float DistanceToPoint(Vertex &V) { return sqrt((m_XPosition - V.XPosition) * (m_XPosition - V.XPosition) + (m_YPosition - V.YPosition) * (m_YPosition - V.YPosition)); }

	Angle AngleToVertex(Vertex &vertex) { return Angle(atan2f(vertex.YPosition - m_YPosition, vertex.XPosition - m_XPosition) * 180.0f / PI); }
	Angle GetAngle() const { return m_Angle; }

protected:
	int m_iPlayerID, m_XPosition, m_YPosition, m_ZPosition {41}, m_EyeLevel {41}, m_FOV {90}, m_iRotationSpeed {4}, m_iMoveSpeed {4};
	Angle m_Angle, m_HalfFOV {45};
	Patch *m_pWeapon;
};
