#pragma once
#include <math.h>

#define PI 3.14159265358979f

class Angle
{
public:
	Angle() {}
	Angle(float angle) : a(angle) {Normalize360();}
	~Angle() {}

	Angle operator=(const float& rhs) { a = rhs; Normalize360(); return *this; }
	Angle operator+(const Angle& rhs) { return Angle(a + rhs.a); }
	Angle operator-(const Angle& rhs) { return Angle(a - rhs.a); }
	Angle operator-() { return Angle(360 - a); }
	Angle& operator+=(const float& rhs) { a += rhs; Normalize360(); return *this; }
	Angle& operator-=(const float& rhs) { a -= rhs; Normalize360(); return *this; }

	bool operator<(const Angle& rhs) { return (a < rhs.a); }
	bool operator<(const float& rhs) { return (a < rhs); }
	bool operator<=(const Angle& rhs) { return (a <= rhs.a); }
	bool operator<=(const float& rhs) { return (a <= rhs); }
	bool operator>(const Angle& rhs) { return (a > rhs.a); }
	bool operator>(const float& rhs) { return (a > rhs); }
	bool operator>=(const Angle& rhs) { return (a >= rhs.a); }
	bool operator>=(const float& rhs) { return (a >= rhs); }

	float GetValue() { return a; }
	float GetCosValue() { return cosf(a * PI / 180.0f); }
	float GetSinValue() { return sinf(a * PI / 180.0f); }
	float GetTanValue() { return tanf(a * PI / 180.0f); }
	float GetSignedValue() { return (a > 180) ? a - 360 : a; }

protected:
	float a {0};

    void Normalize360()
	{
		a = fmodf(a, 360);
		if (a < 0) a += 360;
	}
};
