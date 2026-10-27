# The Last Blade 2: static recompilation

**The Last Blade 2 / Bakumatsu Rouman Dai Ni Maku (SNK, 1998) running as a native PC program: its Motorola 68000 program is recompiled to C and runs on the [neogeorecomp](https://github.com/sp00nznet/neogeorecomp) Neo Geo board model.**

This project describes the MAME `lastbld2` ROM set, builds a generator and a player, and packages a Windows x64 build and a Linux x86_64 AppImage for Steam Deck. The toolkit does the hardware work; this repository supplies only the ROM layout, build files, scripts and documentation.

> **No game data here.** You must supply your own legally obtained `lastbld2.zip` and `neogeo.zip` (MAME sets). The recompiled C is generated from your dump on your machine into `build-*/generated/`, and is not committed. The prebuilt binaries in `dist/` embed a locally generated recompilation and still need your ROM files to run. See [Licensing and legal notes](#licensing-and-legal-notes).

## Status

**v0.1.0, alpha, Windows verified; Linux/AppImage built and smoke-tested.**

- Boot through the real MVS system ROM (`sp-s2.sp1`), the slot-check security screen, How To Play, title/attract, Player Select and a playable match.
- Windows x64 Release build with SDL2 window, keyboard, Xbox-compatible gamepad mapping (verified with SDL's virtual joystick test), YM2610 audio and fullscreen/aspect-correct presentation.
- Native execution verified against Musashi: **5,071,980 recompiled blocks, 0 mismatches** on a 4,001-frame coin/start/play session (final staged Windows executable).
- An 18,651-frame scripted soak fought a match to a **Victory** screen, then ran into the ending/attract: **97.066% native**, 0 uncovered entry points, no crash.
- Measured coverage on that session: **63,729,007 native instructions, 76 interpreted (100.000% reported; the 76 are RAM-resident code)**. On a 6,001-frame no-input attract run: **87,651,724 native, 8,367,995 interpreted (91.285%)**, again because the game executes a copied routine in work RAM.
- Frames 2200 and 3000 of the interpreter and native runs are byte-identical PNGs. Later frames can differ because the game seeds presentation/RNG from the host RTC.
- Audio is produced through the real Z80 + YM2610 path and validated on the SDL output path. The 4,001-frame capture is a 67.588 s 55,555 Hz stereo WAV: peak 17,492, RMS 1,549/1,615, **0 clipped samples**, and no abnormal discontinuities. The first ~8.7 s is the silent BIOS screen. On Windows the device opened at 55,555 Hz with a stable **32-43 ms** queue; a forced 44.1 kHz test exercised the new resampler with a **32-43 ms** queue. Linux PulseAudio (WSLg) held **33-50 ms** with the audio-clock PLL, with **0 dropped frames** and no warnings.
- Linux x86_64 build and AppImage packaging use Ubuntu 22.04 / glibc 2.35, older than SteamOS 3.x, and are smoke-tested under WSL. **Not yet tested on physical Steam Deck hardware.**

## ROM set

The layout follows MAME's `ROM_START( lastbld2 )` (`NEO-AEG PROGBK1Y / NEO-AEG CHA512Y`, NGH-243, later revision). Unzip these files into one directory (`roms/` by default):

| File | Size | CRC32 | Role |
|---|---:|---|---|
| `243-pg1.p1` | 1,048,576 | `af1e6554` | P ROM, fixed 1 MB |
| `243-pg2.sp2` | 4,194,304 | `add4a30b` | P ROM, banked 4 MB |
| `243-c1.c1` â€¦ `243-c6.c6` | 8,388,608 each | `5839444d`, `dd087428`, `6054cbe0`, `8bd2a9d2`, `6a503dcf`, `ec9c36d0` | C ROMs, three 8 MB odd/even pairs (48 MB interleaved) |
| `243-s1.s1` | 131,072 | `c9cd2298` | S ROM, fixed layer |
| `243-m1.m1` | 131,072 | `acf12d10` | M1, Z80 sound driver |
| `243-v1.v1` â€¦ `243-v4.v4` | 4,194,304 each | `f7ee6fbb`, `aa9e4df6`, `4ac750b2`, `f5c64ba6` | V ROMs, 16 MB ADPCM-A |

Required Neo Geo system ROMs (from the `neogeo` set):

| File | Size | CRC32 | Role |
|---|---:|---|---|
| `sp-s2.sp1` | 131,072 | `9036d879` | MVS Asia/Europe BIOS (default; `--bios` selects another) |
| `sfix.sfix` | 131,072 | `c2ea0cfd` | BIOS fixed-layer graphics |
| `sm1.sm1` | 131,072 | `94416d67` | BIOS Z80 sound driver |
| `000-lo.lo` | 131,072 | `5a86cff2` | Sprite shrink table (L0) |

The archive supplied with this project contains the full set and the BIOS files. Only the files above are staged into `roms/`; the original archive is left untouched. The SHA-1 values match MAME for every staged file; see [docs/game-notes.md](docs/game-notes.md).

## Getting started

### Prerequisites

- **Windows:** Visual Studio 2022 or 2026 with *Desktop development with C++*, CMake 3.21+, and the bundled SDL2 VC package in `deps/sdl2/` (included). Ninja is optional.
- **Linux:** `build-essential`, `cmake` 3.21+, `libsdl2-dev`, `wget`, `python3`. Build on Ubuntu 22.04 (glibc 2.35) or older for Steam Deck compatibility.
- **ROMs:** your own `lastbld2` and `neogeo` dumps, unzipped into `roms/`.

### Windows x64

From a Developer PowerShell or a normal PowerShell (CMake finds Visual Studio):

```powershell
cmake -S . -B build-windows -G "Visual Studio 18 2026" -A x64 -DLASTBLADE2_ROM_DIR="$PWD/roms"
cmake --build build-windows --config Release --target lastblade2
.\build-windows\Release\'Last blade 2.exe' --rom-path .\roms
```

If you use Visual Studio 2022, pass `-G "Visual Studio 17 2022"`. Without `LASTBLADE2_ROM_DIR`, the player builds interpreter-only (useful for quick runtime work). The generator output appears in `build-windows/generated/`; it is never committed.

To produce the staged Windows folder:

```powershell
.\scripts\package-windows.ps1
```

Output: `dist/windows/Last Blade 2-Windows-x64/` with `Last blade 2.exe`, `SDL2.dll`, license/notice files and a `roms/README.txt` placeholder. It contains no ROMs or BIOS.

### Linux x86_64

```bash
sudo apt install build-essential cmake libsdl2-dev wget python3
cmake -S . -B build-linux -DCMAKE_BUILD_TYPE=Release -DLASTBLADE2_ROM_DIR="$PWD/roms"
cmake --build build-linux --target lastblade2 --parallel "$(nproc)"
./build-linux/lastblade2 --rom-path ./roms
```

### Steam Deck AppImage

On Ubuntu 22.04 (or in a matching container/WSL):

```bash
./packaging/appimage/build-appimage.sh ./roms \
  "dist/steamdeck/Last Blade 2-SteamDeck-x86_64.AppImage"
chmod +x "dist/steamdeck/Last Blade 2-SteamDeck-x86_64.AppImage"
```

The script builds Release, assembles an AppDir, bundles the SDL2 it linked against, generates an original icon, and runs `appimagetool --appimage-extract-and-run`. It does not copy ROMs.

On the Deck:

1. Copy the AppImage to the Deck and make it executable.
2. Put the `lastbld2` and `neogeo` files in `roms/` beside the AppImage, or pass `--rom-path` in Steam's launch options.
3. Add the AppImage as a non-Steam game. The AppRun starts fullscreen and searches `roms/` beside the AppImage plus common Deck paths. Pass `--windowed` to override fullscreen, or `--rom-path DIR` to select a folder explicitly.
4. In Steam Input, the default gamepad layout works: D-pad/left stick, A/B/X/Y = Neo Geo A/B/C/D, Start = start, Back = coin, left shoulder = select. Steam's **Exit Game** action closes it; `Esc` works with a keyboard.

Presentation is a uniform fit of the board's 320Ã—224 (10:7) picture. On the 1280Ã—800 (16:10) Deck screen it is pillarboxed, never stretched. `F11`/`Alt+Enter` toggles fullscreen in windowed mode.

## Controls

| Action | Keyboard | Gamepad (Xbox layout) |
|---|---|---|
| Move | Arrow keys | D-pad or left stick |
| A / B / C / D | Z / X / C / V | A / B / X / Y |
| Start | 1 (P1), 2 (P2) | Start |
| Coin | 5 (P1), 6 (P2) | Back |
| Select | 3 | Left shoulder |
| Service / test | 9 / F2 | â€” |
| Fullscreen | F11 or Alt+Enter | â€” |
| Quit | Esc | Steam **Exit Game** |

The MVS BIOS requires a coin before Start does anything. Two controllers map to players 1 and 2 and can be plugged in at any time. A controller SDL does not recognise can be described with `SDL_GAMECONTROLLERCONFIG`.

## Verification and known limitations

See [docs/compatibility.md](docs/compatibility.md) for the full test log and commands. Summary:

- **Correctness:** `--verify` re-ran 5,071,980 native blocks in Musashi: **0 mismatches**.
- **Audio:** the WAV has no clipping and real signal from 8.7 s onward; the Windows SDL device and the forced 44.1 kHz resampler both kept a 32-43 ms queue with no errors; Linux/PulseAudio kept 33-50 ms with no drops after the PLL. `NG_AUDIO_DEBUG=1` prints the queue level and any drop for diagnosis.
- **Native coverage:** 100% of executed ROM instructions; the only interpreted instructions are code the game copies into 68000 work RAM (`$100000-$10FFFF`) and executes there. The recompiler analyses ROM, so RAM-resident code correctly runs in Musashi. Attract mode spends more time in such a routine (91.285% of all executed instructions native).
- **P-ROM banking:** the cart has a 1 MB fixed bank and 4 MB banked. The recompiler analyses the first banked megabyte; if the game executes from another bank, the runtime interprets it rather than running the wrong bank's native code. This is exact, but may reduce native coverage for code that lives only in banks 1-3.
- **Not implemented:** netplay, save states, button remapping inside the player (Steam Input can remap), and a settings menu. Physical Steam Deck hardware was not available for this build; the AppImage was tested in WSL only.
- **Title-specific hacks:** none. The runtime changes carried by this project are generic: watchdog reset handling, P-ROM bank guarding, odd-PC guarding, sprite-tile bounds for 48 MB carts, fullscreen/aspect-correct presentation, and `--windowed`.

## Licensing and legal notes

This project's own code â€” the ROM description, build files, scripts, icon generator and documentation â€” is licensed under the **PolyForm Noncommercial License 1.0.0** ([LICENSE](LICENSE)). Commercial use is not permitted. The license was confirmed by the project owner for this code.

The vendored [neogeorecomp](neogeorecomp/) toolkit is a separate MIT-licensed project (`Copyright (c) 2026 sp00nznet`); it keeps its own [LICENSE](LICENSE-neogeorecomp) and [NOTICE](NOTICE-neogeorecomp). Musashi (MIT), superzazu/z80 (MIT), ymfm (BSD-3-Clause) and SDL2 (zlib) keep their own licenses; see [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md) and [LICENSES/](LICENSES/). Those upstream files are not relicensed under PolyForm.

**ROMs, BIOS and generated code.** No ROMs, BIOS images, game artwork or generated recompiled C are included in this repository. The recompiled C is derived from the game's copyrighted program; the prebuilt executables embed that generated code. Distribute those binaries only if you have the rights to do so. *The Last Blade 2 / Bakumatsu Rouman Dai Ni Maku* and the Neo Geo BIOS are the property of their respective copyright holders (SNK). This is an unofficial, noncommercial preservation project, not affiliated with or endorsed by SNK or MAME.

