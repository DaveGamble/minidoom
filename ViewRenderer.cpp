#include "ViewRenderer.hpp"

#include "Map.hpp"
#include "Texture.hpp"
#include "Flat.hpp"

ViewRenderer::ViewRenderer(int renderXSize, int renderYSize, const uint8_t (&l)[34][256])
: renderWidth(renderXSize)
, invRenderWidth(1.f / renderXSize)
, renderHeight(renderYSize)
, invRenderHeight(1.f / renderYSize)
, halfRenderWidth(renderXSize / 2)
, halfRenderHeight(renderYSize / 2)
, distancePlayerToScreen(halfRenderWidth)	// 90 here is FOV
, lights(l)
, ceilingClipHeight(renderWidth)
, floorClipHeight(renderWidth)
{
}


void ViewRenderer::render(uint8_t *pScreenBuffer, int iBufferPitch, const Viewpoint& view, Map &map)
{
	frame++;
	texframe = frame / 20;
	screenBuffer = pScreenBuffer;
	rowlen = iBufferPitch;
	renderLaters.clear();
	solidWallRanges.clear();
	solidWallRanges.push_back({INT_MIN, -1});
	solidWallRanges.push_back({renderWidth, INT_MAX});
	std::fill(ceilingClipHeight.begin(), ceilingClipHeight.end(), -1);
	std::fill(floorClipHeight.begin(), floorClipHeight.end(), renderHeight);
	map.render3DView(view, [&] (const Seg &seg){ addWallInFOV(seg, view); }, frame);

	for (int i = (int)renderLaters.size() - 1; i >= 0; i--)
	{
		renderLater& r = renderLaters[i];
		float v = r.v;
		for (int y = r.from; y < r.to; y++, v += r.dv)
		{
			uint16_t p = ((v >= 0 && v < r.texture->getHeight())) ? r.texture->pixel(r.u, v) : 256;
			if (p != 256) screenBuffer[rowlen * y + r.x] = r.light[p];
		}
	}
}

void ViewRenderer::addWallInFOV(const Seg &seg, const Viewpoint &v)
{
	const int toV1x = seg.start.x - v.x, toV1y = seg.start.y - v.y, toV2x = seg.end.x - v.x, toV2y = seg.end.y - v.y;	// Vectors from origin to segment ends.
	if (toV1x * toV2y >= toV1y * toV2x) return;	// If sin(angle) between the two (computed as dot product of V1 and normal to V2) is +ve, wall is out of view. (It's behind us)

	const float ca = v.cosa, sa = v.sina;
	const float tov1z = toV1x * ca + toV1y * sa, tov2z = toV2x * ca + toV2y * sa;
	float tov1x = toV1x * sa - toV1y * ca, tov2x = toV2x * sa - toV2y * ca;	// Rotate vectors to be in front of us.
	// z = how far in front of us it is. -ve values are behind. +ve values are in front.
	// x = position left to right. left = -1, right = 1.

	if (tov1z < tov1x && tov2z < -tov2x) return;	// V1 is right of FOV and V2 is left of FOV
	if (tov1z < -tov1x && tov2z < -tov2x) return;	// Both points are to the left.
	if (tov1z < tov1x && tov1z > -tov1x) return;	// V1 is within 45 degrees of the X axis (it's on your right hand side, out of the FOV). Both points are to the right.

	const int x1 = (tov1z < -tov1x) ? 0 : distancePlayerToScreen + round(tov1x * halfRenderWidth / tov1z);
	const int x2 = (tov2z < tov2x) ? renderWidth : distancePlayerToScreen + round(tov2x * halfRenderWidth / tov2z);

    if (x1 == x2) return; // Skip same pixel wall
	bool solid =  (!seg.lSector || seg.lSector->ceilingHeight <= seg.rSector->floorHeight || seg.lSector->floorHeight >= seg.rSector->ceilingHeight); // Handle walls and closed door

	if (!solid && (seg.lSector->sky && seg.lSector->sky == seg.rSector->sky) && !seg.sidedef->middletexture.size()
		&& seg.rSector->floorHeight == seg.lSector->floorHeight
		&& seg.lSector->floortexture == seg.rSector->floortexture && seg.lSector->lightlevel == seg.rSector->lightlevel) return;
	
	if (solid && solidWallRanges.size() < 2) return;

    auto f = solidWallRanges.begin(); while (x1 - 1 > f->end) ++f;

    if (x1 < f->start)
    {
        if (x2 < f->start - 1)
        {
            storeWallRange(seg, x1, x2, tov1x, tov2x, tov1z, tov2z, v); //All of the wall is visible, so insert it
            if (solid) solidWallRanges.insert(f, {x1, x2});
            return;
        }
        storeWallRange(seg, x1, f->start - 1, tov1x, tov2x, tov1z, tov2z, v); // The end is already included, just update start
        if (solid) f->start = x1;
    }
    
    if (x2 <= f->end) return; // This part is already occupied
    std::list<SolidSegmentRange>::iterator nextWall = f;
    while (x2 >= next(nextWall, 1)->start - 1)
    {
        storeWallRange(seg, nextWall->end + 1, next(nextWall, 1)->start - 1, tov1x, tov2x, tov1z, tov2z, v); // partialy clipped by other walls, store each fragment
		if (x2 > (++nextWall)->end) continue;
		if (!solid) return;
		f->end = nextWall->end;
		solidWallRanges.erase(++f, ++nextWall);
		return;
    }
    storeWallRange(seg, nextWall->end + 1, x2, tov1x, tov2x, tov1z, tov2z, v);
	if (!solid) return;
	f->end = x2;
	if (nextWall != f) solidWallRanges.erase(++f, ++nextWall);
}

void ViewRenderer::storeWallRange(const Seg &seg, int x1, int x2, float ux1, float ux2, float z1, float z2, const Viewpoint &p)
{
	constexpr float light_depth = 0.05;
	
	const int16_t flags = seg.linedef->flags;
	const float roomHeight = seg.rSector->ceilingHeight - seg.rSector->floorHeight;
	const float lHeight = seg.lSector ? seg.lSector->floorHeight - seg.lSector->ceilingHeight : 0;
	const float lrFloor = seg.lSector ? seg.lSector->floorHeight - seg.rSector->floorHeight : 0;
	const float rlCeiling = seg.lSector ? seg.rSector->ceilingHeight - seg.lSector->ceilingHeight : 0;
	const float tdX = seg.sidedef->dx + seg.offset, tdY = seg.sidedef->dy;
	const float sinv = p.sina, cosv = p.cosa;
	const float seglen = seg.len;
	const float distanceToNormal = (z1 * ux2 - ux1 * z2);
	const float idistanceToNormal = 1.0 / distanceToNormal;	// Distance from origin to rotated wall segment
	const float zscalar = -distancePlayerToScreen * distanceToNormal;
	const float uA = distancePlayerToScreen * (ux1 + z1) * seglen, uB = -z1 * seglen, uC = -distancePlayerToScreen * (ux2 - ux1 + z2 - z1), uD = z2 - z1;
	const float dx = uD * idistanceToNormal, x1z = uC * idistanceToNormal + x1 * dx;

	const float vG = distancePlayerToScreen * (p.z - seg.rSector->floorHeight), vH = distancePlayerToScreen * (seg.rSector->ceilingHeight - p.z);
	const float vA = cosv - sinv, vB = 2 * sinv * invRenderWidth, vC = p.x - 1, vD = -cosv - sinv, vE = 2 * cosv * invRenderWidth, vF = -p.y;

	const float dyCeiling = (seg.rSector->ceilingHeight - p.z) * dx;
	const float dyFloor = (seg.rSector->floorHeight - p.z) * dx;
	const float dyUpper = seg.lSector ? (seg.lSector->ceilingHeight - p.z) * dx : 0;
	const float dyLower = seg.lSector ? (seg.lSector->floorHeight - p.z) * dx : 0;
	const float dSkyAng = invRenderWidth;

	const float horizon = halfRenderHeight + p.pitch * halfRenderHeight;

	float yCeiling = horizon + (seg.rSector->ceilingHeight - p.z) * x1z;
	float yFloor = horizon + (seg.rSector->floorHeight - p.z) * x1z;
	float yUpper = seg.lSector ? horizon + (seg.lSector->ceilingHeight - p.z) * x1z : 0;
	float yLower = seg.lSector ? horizon + (seg.lSector->floorHeight - p.z) * x1z : 0;
	float skyAng = x1 * dSkyAng - 2 * p.angle / M_PI;
	
	for (int x = x1; x <= x2; x++)
    {
		const float iz = 1.f / (uC + x * uD);
		const float z = zscalar * iz;
		const float u = (uA + x * uB) * iz + tdX;
		int light = std::min(32 - (seg.rSector->lightlevel >> 3), (int)((z - 5) * light_depth));
		
		const uint8_t *lut = lights[std::clamp(light, 0, 31)];

		auto DrawTexture = [&](const std::vector<const Texture *> &textures, int from, int to, float a, float b, float dv, int stage) {
			if (!textures.size()) return;
			const Texture *texture = textures[texframe % textures.size()];
			if (!texture || to <= from) return;
			dv /= (b - a);
			float v = -a * dv;
			if (stage == 0 && !(flags & kUpperTextureUnpeg)) v = -b * dv;// top
			if (stage == 1 && (flags & kLowerTextureUnpeg))  v = -yFloor * dv; // middle
			if (stage == 2 && (flags & kLowerTextureUnpeg)) v = -yCeiling * dv; // bottom
			v += tdY;
			for (int y = from; y < to; y++) { screenBuffer[rowlen * y + x] = lut[texture->pixel(u, v + y * dv) & 255]; }
		};

		auto DrawSky = [&](const Patch *sky, int from, int to) {
			int tx = (skyAng - floor(skyAng)) * sky->getWidth();
			if (tx == sky->getWidth()) tx = 0;
			for (int i = std::max(0, from); i < std::min(to, renderHeight); i++)
			{
				float ty = std::clamp((i - horizon + halfRenderHeight) * sky->getHeight() * invRenderHeight, -1.f, sky->getHeight() - 1.f);
				screenBuffer[rowlen * i + x] = lights[0][sky->pixel(sky->getColumnDataIndex((ty < 0) ? 0 : tx), std::max(ty, 0.f))];
			}
		};

		auto DrawFloor = [&](const std::vector<const Flat *> &flats, int from, int to) {
			const Flat *flat = flats[texframe % flats.size()];
			for (int i = std::max(0, from); i < std::min(to, renderHeight); i++)
			{
				float z = vG / (i - horizon);
				int light = std::min(32 - (seg.rSector->lightlevel >> 3), (int)((z - 5) * light_depth));
				screenBuffer[i * rowlen + x] = lights[std::clamp(light, 0, 31)][flat->pixel(z * (vA + vB * x) + vC, z * (vD + vE * x) + vF)];
			}
		};

		auto DrawCeiling = [&](const std::vector<const Flat *> &flats, int from, int to) {
			if (seg.rSector->sky)	DrawSky(seg.rSector->sky, from, to);
			else
			{
				const Flat *flat = flats[texframe % flats.size()];
				for (int i = std::max(0, from); i < std::min(to, renderHeight); i++)
				{
					float z = vH / (horizon - i);
					int light = std::min(32 - (seg.rSector->lightlevel >> 3), (int)(z - 5) / 20);
					screenBuffer[i * rowlen + x] = lights[std::clamp(light, 0, 31)][flat->pixel(z * (vA + vB * x) + vC, z * (vD + vE * x) + vF)];
				}
			}
		};
				
		int CurrentCeilingEnd = std::max(yCeiling, ceilingClipHeight[x] + 1.f), CurrentFloorStart = std::min(yFloor, floorClipHeight[x] - 1.f);
		int ceilbot = std::min(CurrentCeilingEnd, CurrentFloorStart), floortop = std::max(CurrentFloorStart, ceilingClipHeight[x]);
		int midtop = std::max(std::max(0, ceilbot), ceilingClipHeight[x]), midbot = std::min(floortop, renderHeight - 1);
		
		DrawFloor(seg.rSector->floortexture, floortop, floorClipHeight[x]);
		DrawCeiling(seg.rSector->ceilingtexture, std::max(0, ceilingClipHeight[x]), ceilbot);

        if (seg.lSector)
        {
			int upper = std::min((float)CurrentFloorStart, yUpper), lower = std::max(yLower, ceilingClipHeight[x] + 1.f);
			if (seg.sidedef->middletexture.size() && midtop < midbot && yFloor > yCeiling)
			{
				int top = std::max(std::max(upper, ceilbot), 0), bot = std::min(std::min(lower, floortop), renderHeight);
				const Texture *tex = seg.sidedef->middletexture[texframe % seg.sidedef->middletexture.size()];
				float dv = z * invRenderWidth * 2;
				float v =  -std::max(yUpper, yCeiling) * dv;
				if (flags & kLowerTextureUnpeg)  v = tex->getHeight() -std::min(yLower, yFloor) * dv;
				renderLaters.push_back({tex, x, top, bot, u,  v + top * dv + tdY, dv, lut});
			}

			if (seg.lSector->sky) DrawSky(seg.lSector->sky, ceilbot, upper);
			else if (seg.sidedef->uppertexture.size()) DrawTexture(seg.sidedef->uppertexture, ceilbot, upper, yCeiling, yUpper, rlCeiling, 0);
			if (seg.sidedef->lowertexture.size()) DrawTexture(seg.sidedef->lowertexture, lower, floortop, yLower, yFloor, lrFloor, 2);
			ceilingClipHeight[x] = std::max(CurrentCeilingEnd - 1, upper);
			floorClipHeight[x] = std::min(CurrentFloorStart + 1, lower);
		}
        else
		{
			DrawTexture(seg.sidedef->middletexture, midtop, midbot, yCeiling, yFloor, roomHeight, 1);
			ceilingClipHeight[x] = renderHeight;
			floorClipHeight[x] = -1;
		}
		yUpper += dyUpper;
		yLower += dyLower;
        yCeiling += dyCeiling;
        yFloor += dyFloor;
		skyAng += dSkyAng;
    }
}


