# Melee UWP — first full test build

Based on Melee-PC v0.4.2 (`0fb0af3304d3469308b5574369a6ec86f145632f`). Original PC launcher, in-game menus and simulation remain the base. The UWP disc picker browses removable storage; disc contents are read directly without importing the full image.

Verified locally in a Windows UWP AppContainer:

- 995 original game C units and 54 host C units compile with GCC into UCRT-compatible x64 COFF. The original big-endian and floating-point rules remain enabled.
- The full host, original RmlUi launcher and current vendored Aurora link with the UWP dependency depot.
- Original rollback-range checks pass: required game globals are captured, engine/audio globals excluded. This is not a netplay acceptance test.
- The normal launcher presents at least 120 frames. A private direct-boot test reads the supplied USA revision 2 RVZ from USB and reaches the upstream 120-frame exit limit.
- USB atomic creation, replacement, preserved-content updates, byte-exact readback, metadata and directory enumeration pass. Shader databases persist on USB and produce cache hits on subsequent launches.
- UWP input initialization and the original 1000 Hz polling thread start. Controller behavior, audible sound and actual save-game creation still require console testing.
- User data stays under `USB:\Melee-UWP`, with no automatic internal fallback. OS installation data and access tokens remain system-managed.

The USA 1.02 image is available and its container/header checks passed; full decoded-disc SHA-1 validation has not been run. The standalone probes also verify endian access, GCC/Store ABI, varargs, stack probing and 30,000 original RNG steps.

The desktop self-updater cannot install a UWP package; UWP updates use Device Portal. Original online functionality is retained as an adaptation target, but Xbox/PC/Android compatibility is unverified.

This remains an experimental personal test build. Xbox rendering, performance, controls, audio, USB browser navigation, texture packs and save-game persistence are not accepted yet. Private compiler outputs, diagnostics, local game images and toolchains stay ignored under `.local/`, `image/` and other local exclusions. This repository publishes the UWP source adaptation only; no binary release is provided.
