The Last Blade 2 - Windows x64
==============================

Native static recompilation of The Last Blade 2 (SNK, 1998) on the
neogeorecomp Neo Geo board model.

NO ROMS OR BIOS ARE INCLUDED
----------------------------
This folder contains no game or BIOS data. You must supply your own legally
obtained MAME sets. Unzip these files into the "roms" folder next to the exe:

  From lastbld2.zip:
    243-pg1.p1   243-pg2.sp2
    243-c1.c1    243-c2.c2    243-c3.c3
    243-c4.c4    243-c5.c5    243-c6.c6
    243-s1.s1    243-m1.m1
    243-v1.v1    243-v2.v2    243-v3.v3    243-v4.v4

  From neogeo.zip:
    sp-s2.sp1    sfix.sfix    sm1.sm1    000-lo.lo

CRC32/SHA-1 values are in docs/game-notes.md.

RUNNING
-------
  Last blade 2.exe --rom-path roms

If the ROMs are in a different folder, pass --rom-path DIR. The default BIOS
is sp-s2.sp1 (MVS Asia/Europe); another 128 KB system ROM can be selected with
--bios FILE. The MVS BIOS requires a coin before Start does anything.

CONTROLS
--------
  Move .............. arrow keys          D-pad or left stick
  A / B / C / D ..... Z / X / C / V       A / B / X / Y (Xbox layout)
  Start ............. 1 (P1), 2 (P2)      Start
  Coin .............. 5 (P1), 6 (P2)      Back
  Select ............ 3                   Left shoulder
  Fullscreen ........ F11 or Alt+Enter    --
  Quit .............. Esc                 Steam Exit Game

Two controllers map to players 1 and 2 and can be plugged in at any time.
The picture keeps the board's 10:7 aspect ratio and is pillarboxed or
letterboxed, never stretched.

USEFUL OPTIONS
--------------
  --fullscreen / --windowed    start in either mode
  --scale N                    window scale (default 3)
  --bios FILE                  select the system ROM
  --headless                   no window; useful for testing
  --frames N                   stop after N frames
  --screenshot N:FILE          save a PNG
  --wav FILE                   write the audio
  --verify                     check native blocks against Musashi

LICENSING
---------
This project's original code is PolyForm Noncommercial 1.0.0. The bundled
neogeorecomp runtime is MIT, Musashi and z80 are MIT, ymfm is BSD-3-Clause,
and SDL2 is zlib; see LICENSE, THIRD_PARTY_NOTICES.md and LICENSES/.

The executable embeds a recompilation generated from the game program. It is
distributed for noncommercial preservation only, and only where the
distributor has the necessary rights. The Last Blade 2 and the Neo Geo BIOS
are the property of SNK. This is an unofficial project, not affiliated with
or endorsed by SNK.
