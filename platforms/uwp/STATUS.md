# Melee UWP - development status

Based on Melee-PC v0.4.2 (`0fb0af3304d3469308b5574369a6ec86f145632f`). Original PC launcher, menus and simulation remain the base. The UWP disc picker browses removable storage and reads the disc directly without importing the full image. Application-managed saves, settings and caches use `USB:\Melee-UWP`; OS installation data and access tokens remain system-managed.

## Tester reports

- Initial Xbox gameplay test reported working very well.
- Online Direct Connect reported working after a Windows/Xbox test was requested. This is one reported session, not a general compatibility guarantee or a Slippi compatibility claim.
- Four-player local play, Android crossplay, texture packs, full save/reload coverage and long-session stability remain unverified.
- No measured Xbox frame-rate benchmark is claimed.

## Local verification

- 995 game C units and 54 host C units compile into UCRT-compatible x64 COFF, preserving the upstream endian and floating-point rules.
- Original rollback-range checks pass: required game globals are captured and engine/audio globals excluded.
- Windows AppContainer tests presented the original launcher and reached a 120-frame direct USB game-boot smoke-test limit.
- USB atomic creation/replacement, byte-exact readback, metadata and directory enumeration pass. Shader databases persist on USB and produce subsequent cache hits.
- Isolated ABI, endian, varargs, stack and 30,000-step original RNG checks pass.

## Version 0.0.3

The startup symbol is white on transparency over the dark background. The application executable is byte-identical to 0.0.2; only splash artwork, package version and signing metadata changed. Packaging and signing passed; the changed splash has been inspected in a 1080p preview, not revalidated on Xbox.

The repository is a source overlay; original game source, game images, private build outputs and personal automation are excluded. Complete reproducible build integration remains in development. Desktop self-updates are unavailable in UWP; application updates use Device Portal.
