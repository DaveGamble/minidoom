This is a repo for a fork of DIYDoom [https://github.com/amroibrahim/DIYDoom/tree/master].

It's in a very rough state, and includes SDL2 directly.

Tidying is for later. Although maybe it'll end up as an stb style library. Which would rock.

TODO:
- Clip detection
- Lighting
- Optimise floor/ceiling drawing
- Render sprites
- View bob?
- Skybox
- SFX
- Music
- Maybe actually build some game code around it?
- Automap?
- A nice interface for selecting weapons and drawing firing animations?
- Be really cool if we could do, like, doors and floor movements and stuff?
- Event triggering?


Build is for Xcode on Apple Silicon macs (but if you replaced the precompiled SDL binary you could do whatever)

Requires a WAD file next to the binary or crashy times. See DIYDoom.cpp

