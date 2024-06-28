#pragma once
#include "DataTypes.hpp"

class Flat
{
public:
	Flat(const uint8_t *_data) { memcpy(data, _data, 4096); }
	uint8_t pixel(int u, int v) const { return data[(64 * v + (u & 63)) & 4095]; }
protected:
	uint8_t data[4096];
};
