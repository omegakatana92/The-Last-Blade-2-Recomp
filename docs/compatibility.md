# Compatibility and verification report

## Environment

- Toolkit: neogeorecomp v0.2.0 (`af96b2b4b39b42b392b17bcaa6a74a5f27088202`) plus
  the generic patches listed in `THIRD_PARTY_NOTICES.md`.
- Windows: Visual Studio 2026 (MSVC 19.51.36257.0), CMake 4.3.3, SDL2 2.32.10.
- Linux: Ubuntu 22.04.5 (GCC 11.4.0, glibc 2.35), CMake 3.28.3, SDL2 2.0.20.
- ROMs: MAME `lastbld2` + `neogeo`; every staged file's CRC32 and SHA-1 matches
  MAME (table in `docs/game-notes.md`).

## Results

| Check | Result |
|---|---|
| ROM hashes / layout | **Pass** â€” all 18 staged files match MAME CRC32 and SHA-1; `NEO-GEO`/NGH `$0243` at `$100` after word swap |
| Interpreter boot | **Pass** â€” BIOS warning/slot check, How To Play, Player Select, stage, match |
| Native generation | **Pass** â€” 931,089 instructions, 15,849 routines, 25,193 dispatch entries |
| Windows x64 Release build | **Pass** â€” MSVC 19.51, `Last blade 2.exe` 31,882,752 bytes + `SDL2.dll` |
| Native boot / gameplay | **Pass** â€” coin/start to a match; screenshots at 2200 and 3000 byte-identical to interpreter |
| `--verify` | **Pass** â€” 5,071,980 blocks, **0 mismatches** (final staged Windows executable) |
| Native coverage, coin/start match | **63,729,007 native / 76 interpreted** (100.000% reported; 76 are work-RAM instructions) |
| Native coverage, 6,001-frame attract | **87,651,724 native / 8,367,995 interpreted (91.285%)**; interpreted code is the game's RAM-resident routine at `$1008xx` |
| Gameplay soak, 18,651 frames | **Pass** â€” scripted fight through a Victory screen into the ending/attract; 24,340,635 native blocks, 8,368,079 interpreted (97.066%), 0 uncovered entry points, no crash |
| Audio WAV analysis | **Pass** â€” 67.588 s, 55,555 Hz stereo, peak 17,492, RMS L/R 1,549/1,615, 0 clipped samples, no abnormal discontinuities; initial 8.7 s is the silent BIOS screen |
| Audio device, Windows | **Pass** â€” 55,555 Hz stereo, queue 32-43 ms over 1,200 frames, 0 drops, no warnings |
| Audio resampler, 44.1 kHz | **Pass** â€” forced 44.1 kHz device: `55555 -> 44100` SDL_AudioStream, queue 32-43 ms, no warnings |
| Audio queue, Linux PulseAudio | **Pass** â€” 55,555 Hz, queue 33-50 ms over 1,200 frames after the audio-clock PLL; 0 drops, no warnings |
| Windows windowed launch | **Pass** â€” 300 frames, 6.6 s wall clock, SDL window/renderer/audio/gamepad subsystems initialised |
| Windows gamepad | **Pass** â€” `ng_gamepad_test` (SDL virtual joystick, dummy video/audio) checks D-pad/stick, A/B/X/Y, Start/Back, two players and hot-unplug; no physical pad available |
| Linux build | **Pass** â€” GCC 11 Release binary (glibc 2.35), 5,071,895 blocks verified, 0 mismatches |
| AppImage build | **Pass** â€” 14 MB AppImage, executable, bundles SDL2, contains license/docs |
| AppImage launch (WSL) | **Pass** â€” 4,001-frame coin/start to a match, 5,063,190 blocks, 0 uncovered entries, audio WAV |
| AppImage fullscreen (Xvfb 1280x800) | **Pass** â€” AppRun default fullscreen, uniform 10:7 fit with pillarbox; windowed mode also launches |
| Resource use | Windows windowed: 186 MB working set (177 MB private), 1,200 frames exit 0; Linux AppImage headless: 1.77 s for 1,201 frames, 178.7 MB max RSS, 100% of one core |
| Steam Deck hardware | **Not tested** â€” no physical Deck available; WSL test only |
| Steam Deck Gaming/Desktop Mode | **Not tested on hardware** |

## Coverage interpretation

`--verify` compares every native block against Musashi with the same memory
and I/O values. It reported zero mismatches over more than five million blocks.

The `[coverage]` line counts 68000 instructions. The game copies a routine into
work RAM (`$100000-$10FFFF`) and executes it there; the recompiler analyses ROM
only, so those instructions are interpreted. The `[profile]` output from the
attract run named addresses such as `$100920`, `$10091C`, `$1000C4`, all in work
RAM. There were **0 uncovered entry points** in every run, which means no ROM
address was left without a native routine. The reported 91.285% is therefore
not missing ROM coverage; it is the cost of RAM-resident code.

If the game executes code from a P-ROM bank other than bank 0, the runtime
interprets it by design (the native routine is tied to the bytes of bank 0).
The scripted sessions did not show such code; the bank guard remains as a
correctness safety net.

## Reproduction commands

### Windows

```powershell
cmake -S . -B build-windows -G "Visual Studio 18 2026" -A x64 -DLASTBLADE2_ROM_DIR="$PWD/roms"
cmake --build build-windows --config Release --target lastblade2
.\build-windows\Release\'Last blade 2.exe' --headless --rom-path .\roms --frames 4001 --input .\tests\coin_start.txt --screenshot 3800:.\build-windows\frame3800.png
.\build-windows\Release\'Last blade 2.exe' --headless --verify --rom-path .\roms --frames 4001 --input .\tests\probe.txt
```

`tests/probe.txt` is the longer boot/menu sequence used for the 4,001-frame
verification. `tests/coin_start.txt` is the cleaned-up version.

### Linux / AppImage

```bash
cmake -S . -B build-linux -DCMAKE_BUILD_TYPE=Release -DLASTBLADE2_ROM_DIR="$PWD/roms"
cmake --build build-linux --target lastblade2 --parallel "$(nproc)"
./build-linux/lastblade2 --headless --rom-path ./roms --frames 4001 --input ./tests/coin_start.txt
./packaging/appimage/build-appimage.sh ./roms "dist/steamdeck/Last Blade 2-SteamDeck-x86_64.AppImage"
```

## Known limitations

- No physical Steam Deck, SteamOS, or controller testing in this environment.
- No netplay, save states, in-game button remapping, or settings menu.
- RAM-resident 68000 code is interpreted (correct, but slower).
- Code executed from P-ROM banks 1-3 is interpreted (correct, but slower).
- The prebuilt binaries still require the user's ROMs and BIOS; they contain
  generated code derived from the game program.

