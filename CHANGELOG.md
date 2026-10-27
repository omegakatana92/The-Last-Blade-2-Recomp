# Changelog

## [0.1.0] - 2026-10-27

First Last Blade 2 port.

### Added

- Verified `lastbld2` ROM layout (`src/game.c`): 1 MB + 4 MB P ROM,
  three 8 MB C-ROM pairs, S/M1/V ROMs, MVS BIOS.
- Generator and player targets; 25,193 native dispatch entries from
  931,089 recompiled instructions.
- Windows x64 Release build (MSVC 2026, SDL2 2.32.10) and Linux x86_64
  Release build (GCC 11, glibc 2.35).
- Steam Deck AppImage packaging with an original generated icon, desktop
  entry, metainfo, AppRun ROM discovery, fullscreen default and bundled SDL2.
- Generic runtime support needed by this title: watchdog reset handling,
  P-ROM bank guarding, odd-PC guarding, 48 MB sprite bounds, fullscreen and
  aspect-correct presentation, `--windowed`, `NG_TIMING`.
- Scripted boot/play tests, `--verify` result and coverage report.

### Verified

- `--verify`: 5,071,862 blocks, 0 mismatches.
- Coin/start match: 63,729,007 native instructions, 76 interpreted
  (work-RAM code), 0 uncovered entry points.
- 6,001-frame attract: 91.285% native; interpreted instructions are the
  game's RAM-resident routine.
- Audio WAV produced with real signal (67.588 s, peak 17,492, 0 clipped).
- Audio output hardened: 55,555 -> 44.1/48 kHz conversion, queue error
  reporting, 35/300 ms jitter window and an audio-clock PLL. Windows device
  queue 32-43 ms; Linux/PulseAudio 33-50 ms; 0 drops in both tests.

