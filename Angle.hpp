#pragma once
#include <math.h>

class Angle
{
public:
	Angle() {}
	Angle(float angle) : a(angle) {norm();}
	~Angle() {}

	Angle operator=(const float& rhs) { a = rhs; norm(); return *this; }
	Angle operator+(const Angle& rhs) { return Angle(a + rhs.a); }
	Angle operator-(const Angle& rhs) { return Angle(a - rhs.a); }
	Angle operator-() { return Angle(360 - a); }
	Angle& operator+=(const float& rhs) { a += rhs; norm(); return *this; }
	Angle& operator-=(const float& rhs) { a -= rhs; norm(); return *this; }

	bool operator<(const Angle& rhs) { return (a < rhs.a); }
	bool operator<(const float& rhs) { return (a < rhs); }
	bool operator<=(const Angle& rhs) { return (a <= rhs.a); }
	bool operator<=(const float& rhs) { return (a <= rhs); }
	bool operator>(const Angle& rhs) { return (a > rhs.a); }
	bool operator>(const float& rhs) { return (a > rhs); }
	bool operator>=(const Angle& rhs) { return (a >= rhs.a); }
	bool operator>=(const float& rhs) { return (a >= rhs); }

	float get() { return a; }
	float cos() { return cosf(a * M_PI / 180.0f); }
	float sin() { return sinf(a * M_PI / 180.0f); }
protected:
	float a {0};

    void norm()
	{
		a -= 360 * floor(a / 360);
	}
};
