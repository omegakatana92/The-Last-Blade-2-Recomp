# Scripted tests

The input format is neogeorecomp's `<frame>[-<end>] <button>[+<button>...]`,
one event per line. Buttons: `up down left right a b c d`, `p2*`, `start`,
`start2`, `select`, `coin`, `coin2`, `service`, `test`.

- `coin_start.txt` — boot through the BIOS/slot check, insert a coin, start,
  accept the default character and stage; reaches a match by ~frame 3800.
- `play_long.txt` — the same boot plus a scripted fight soak through frame
  18650.
- `probe.txt` — the longer boot/menu sequence used for the 4,001-frame
  `--verify` run.

Run headless:

```powershell
.\build-windows\Release\'Last blade 2.exe' --headless --rom-path .\roms --frames 4001 --input .\tests\coin_start.txt --screenshot 3800:frame3800.png
.\build-windows\Release\'Last blade 2.exe' --headless --verify --rom-path .\roms --frames 4001 --input .\tests\probe.txt
```

```bash
./build-linux/lastblade2 --headless --rom-path ./roms --frames 4001 --input ./tests/coin_start.txt
./build-linux/lastblade2 --headless --verify --rom-path ./roms --frames 4001 --input ./tests/probe.txt
```

For coverage, add `--dump-misses entries.txt` and `NG_PROFILE=1`; then rebuild
so the generator reads `entries.txt`. RAM-resident code is not a ROM entry
point and will remain interpreted.
