This is a repo for a fork of DIYDoom [https://github.com/amroibrahim/DIYDoom/tree/master].

It's in a very rough state, and includes SDL2 directly. It may make your eyes bleed. Tidying is for later.

TODO:
Wishlist:
- View bob?
- Nicer collision detection, with a rectilinear bounding box
- Wall slide movement if you run at a wall?
- Be really cool if we could do, like, doors and floor movements and stuff?
- SFX / Music
- A nice interface for selecting weapons and drawing firing animations?
- Lighting effects more like the original
- Optimising performance? Prerender textures?
- Remove <map>, <list>, <vector> and <string>? Go full c89?


Build is for Xcode on Apple Silicon macs (but if you replaced the precompiled SDL binary you could do whatever)

Requires a WAD file next to the binary or crashy times. See DIYDoom.cpp

