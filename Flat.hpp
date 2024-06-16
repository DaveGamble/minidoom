#pragma once
#include "DataTypes.hpp"

class Flat
{
public:
	Flat(const uint8_t *_data) { memcpy(data, _data, 4096); }
	
	void renderSpan(uint8_t *buffer, int len, float u1, float v1, float du, float dv) const
	{
		for (int i = 0; i < len; i++) buffer[i] = data[(int)(64*floor(64*(v1 + i * dv)) + floor(64*(u1 + i * du)))];
	}
protected:
	uint8_t data[4096];
};
