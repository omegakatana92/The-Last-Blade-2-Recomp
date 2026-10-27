# Third-party notices

This project contains original code and vendored third-party components. The
original code is licensed under PolyForm Noncommercial 1.0.0; the components
below keep their own licenses. Nothing below is relicensed under PolyForm.

## neogeorecomp

- Copyright (c) 2026 sp00nznet
- MIT License
- Upstream: <https://github.com/sp00nznet/neogeorecomp>
- Vendored source: `neogeorecomp/`, pinned to upstream release v0.2.0
  (`af96b2b4b39b42b392b17bcaa6a74a5f27088202`).
- License: `LICENSE-neogeorecomp`; notice: `NOTICE-neogeorecomp`.

The vendored copy carries the following **generic** changes from this project
line. They contain no game-specific addresses or patches:

1. **Watchdog modelling** (`src/bus.c`, `src/io.c`, `include/neogeorecomp/bus.h`,
   `src/neogeo.c`): an A0-set write in `$300000-$3BFFFF` strobes a 1,622,015
   68000-clock watchdog; expiry resets the board while preserving work and
   backup RAM. Needed by the post-1998 MVS slot check.
2. **P-ROM bank guarding** (`src/exec.c`, `include/neogeorecomp/bus.h`): while
   a bank other than bank 0 is selected, the `$200000-$2FFFFF` window is
   interpreted rather than dispatched to bank-0 native code.
3. **Odd-PC guarding** (`src/exec.c`): an odd 68000 PC is left to Musashi to
   raise an address error instead of being aliased to the even entry below it.
4. **Coverage report** (`src/exec.c`): prints native and interpreted
   instruction totals as a percentage.
5. **Sprite bounds** (`src/video.c`): rejects decoded tile numbers past the
   fitted C-ROM data, required for 24/48 MB carts whose tile count is not a
   power of two.
6. **Window presentation** (`src/window_sdl.c`, `src/platform.c`,
   `src/neogeo.c`, `src/ng_internal.h`): `--fullscreen`, `--windowed`,
   F11/Alt+Enter, nearest-neighbour scaling, and a centred 10:7
   letterbox/pillarbox fit instead of stretching.
7. **Timing/diagnostic flag** (`src/neogeo.c`): `NG_TIMING=1` reports simulated
   68000 clocks.
8. **Audio output robustness** (`src/window_sdl.c`): the SDL device may open at
   44.1/48 kHz instead of the YM2610's 55,555 Hz; an `SDL_AudioStream` converts
   the rate so pitch and speed stay correct. Queue errors are reported once,
   a 35/300 ms queue window absorbs PulseAudio/PipeWire jitter, and a gentle
   audio-clock PLL trims frame pacing by at most 0.5 ms so the queue does not
   drift. `NG_AUDIO_DEBUG=1` reports the queue level and drops.

The upstream `docs/` and `README.md` in `neogeorecomp/` are retained as
shipped, except `docs/screenshots/mslug-tank.png`, which was removed because
it contains Metal Slug game artwork; the upstream README's image link is
therefore dangling. No game art is distributed. The modified files remain
under the upstream MIT license.

## Musashi

- Copyright (c) 1998-2001 Karl Stenerud
- MIT License
- Upstream: <https://github.com/kstenerud/Musashi>, commit `313ebf1`
- Vendored: `neogeorecomp/ext/musashi/`
- License: `LICENSES/Musashi.txt`
- Used as the Musashi fallback interpreter, the `--verify` oracle, the 68000
  cycle table, and the conformance corpus.

## superzazu/z80

- Copyright (c) 2019 Nicolas Allemand
- MIT License
- Upstream: <https://github.com/superzazu/z80>, commit `d64fe10`
- Vendored: `neogeorecomp/ext/z80/`
- License: `LICENSES/z80.txt`
- Used as the Z80 sound CPU.

## ymfm

- Copyright (c) 2021 Aaron Giles
- BSD 3-Clause License
- Upstream: <https://github.com/aaronsgiles/ymfm>, commit `81aec25`
- Vendored: `neogeorecomp/ext/ymfm/`
- License: `LICENSES/ymfm.txt`
- Synthesizes the YM2610 FM, SSG and ADPCM-A/B channels.

## SDL2

- Copyright (c) 1997-2025 Sam Lantinga and SDL contributors
- zlib License
- Upstream: <https://github.com/libsdl-org/SDL>
- Windows development package: `deps/sdl2/SDL2-2.32.10/`
- License: `LICENSES/SDL2.txt` and
  `deps/sdl2/SDL2-2.32.10/LICENSE.txt`
- Provides the window, renderer, audio device and gamepad input. The Windows
  distribution ships `SDL2.dll`; the AppImage bundles the SDL2 shared library
  it linked against.

## Game data

No game ROMs, Neo Geo BIOS images, sprites, music or other game assets are
included. The recompiled C is generated locally from the user's ROMs and is
not committed. The prebuilt binaries in `dist/` embed a recompilation derived
from the game program; distribute them only if you have the necessary rights.

*The Last Blade 2 / Bakumatsu Rouman Dai Ni Maku* is © SNK. The Neo Geo BIOS
is © SNK. MAME is a separate project and is used here only as a hardware and
ROM-layout reference; no MAME code is included.
