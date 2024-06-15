#pragma once
#include <math.h>

#define PI 3.14159265358979f

class Angle
{
public:
	Angle() {}
	Angle(float angle) : m_Angle(angle) {Normalize360();}
	~Angle() {}

	Angle operator=(const float& rhs) { m_Angle = rhs; Normalize360(); return *this; }
	Angle operator+(const Angle& rhs) { return Angle(m_Angle + rhs.m_Angle); }
	Angle operator-(const Angle& rhs) { return Angle(m_Angle - rhs.m_Angle); }
	Angle operator-() { return Angle(360 - m_Angle); }
	Angle& operator+=(const float& rhs) { m_Angle += rhs; Normalize360(); return *this; }
	Angle& operator-=(const float& rhs) { m_Angle -= rhs; Normalize360(); return *this; }

	bool operator<(const Angle& rhs) { return (m_Angle < rhs.m_Angle); }
	bool operator<(const float& rhs) { return (m_Angle < rhs); }
	bool operator<=(const Angle& rhs) { return (m_Angle <= rhs.m_Angle); }
	bool operator<=(const float& rhs) { return (m_Angle <= rhs); }
	bool operator>(const Angle& rhs) { return (m_Angle > rhs.m_Angle); }
	bool operator>(const float& rhs) { return (m_Angle > rhs); }
	bool operator>=(const Angle& rhs) { return (m_Angle >= rhs.m_Angle); }
	bool operator>=(const float& rhs) { return (m_Angle >= rhs); }

	float GetValue() { return m_Angle; }
	float GetCosValue() { return cosf(m_Angle * PI / 180.0f); }
	float GetSinValue() { return sinf(m_Angle * PI / 180.0f); }
	float GetTanValue() { return tanf(m_Angle * PI / 180.0f); }
	float GetSignedValue() { return (m_Angle > 180) ? m_Angle - 360 : m_Angle; }

protected:
	float m_Angle {0};

    void Normalize360()
	{
		m_Angle = fmodf(m_Angle, 360);
		if (m_Angle < 0) m_Angle += 360;
	}
};
