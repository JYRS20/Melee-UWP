# Melee UWP

Experimental UWP adaptation of [Melee-PC](https://github.com/999sian/melee-pc), a native port of Super Smash Bros. Melee, for Xbox Developer Mode.

The adaptation keeps the original PC launcher and menus, reads the user-supplied disc directly from USB, and stores saves, settings and caches under `Melee-UWP` on that drive. Supported starting point: USA 1.02 (GALE01, revision 2).

Xbox gameplay and an online Direct Connect session have been reported working by the tester. Windows UWP launch and direct USB disc access have also been verified locally. This remains experimental: four-player local play, Android crossplay and long-session stability have not been validated. Version 0.0.3 also removes the white rectangle from the startup splash.

## Source and credits

This repository contains the UWP source overlay for Melee-PC v0.4.2, commit `0fb0af3304d3469308b5574369a6ec86f145632f`. Apply its files to that upstream source tree, preserving its dependencies. The CMake targets require a UWP dependency depot and verified GCC/UCRT COFF objects; complete build integration remains in development.

No disc images, extracted game data, decompiled game source, compiled game objects, private certificates or credentials are included. Unlike a runtime recompiler, Melee-PC builds its native executable in advance and reads assets from your own disc image.

Port code is GPL-3.0-or-later; Aurora modifications retain the MIT license. See [credits](CREDITS.md), [licensing](licenses/LICENSE.md) and [development status](platforms/uwp/STATUS.md).
