# The Last Blade 2: ROM set and board notes

## Identity

- MAME set: `lastbld2`
- Title: *The Last Blade 2 / Bakumatsu Rouman Dai Ni Maku: Gekka no Kenshi*
- NGH/NGM: 243 / 2430
- Release: MVS 1998-11-25; AES 1999-01-28
- Board: `NEO-AEG PROGBK1Y` (program) + `NEO-AEG CHA512Y` (graphics)
- MAME comment: "MVS AND AES VERSION", "later revision"

## Layout

The verified `ng_game_t` is in `src/game.c`. It mirrors MAME's `ROM_START( lastbld2 )`:

- **P ROM:** `$500000` total. `243-pg1.p1` is the fixed 1 MB at `$000000`; `243-pg2.sp2` is the banked 4 MB at `$100000`. Both are `ROM_LOAD16_WORD_SWAP`, so the loader swaps each 16-bit word.
- **C ROMs:** `$3000000` (48 MB) after pairing. `c1/c2`, `c3/c4`, `c5/c6` are three 8 MB odd/even pairs (`ROM_LOAD16_BYTE`), interleaved by the loader.
- **S ROM:** `243-s1.s1`, 128 KB.
- **M1:** `243-m1.m1`, 128 KB Z80 driver.
- **V ROMs:** `243-v1.v1` … `243-v4.v4`, 16 MB total ADPCM-A.
- **BIOS:** `sp-s2.sp1` (default), `sfix.sfix`, `sm1.sm1`, `000-lo.lo`.

After the word swap, the fixed bank has `NEO-GEO` at `$100` and NGH number `$0243`. The first 68000 vectors are at the start of the fixed bank; the cart header's USER/PLAYER_START/DEMO_END/COIN_SOUND entries are `JMP` instructions into the fixed bank.

## Hashes

All staged files match MAME's CRCs and SHA-1s. The archive supplied with the project has these values:

| File | CRC32 | SHA-1 |
|---|---|---|
| `243-pg1.p1` | `af1e6554` | `bd8526f60c2472937728a5d933fbd19d899f2cba` |
| `243-pg2.sp2` | `add4a30b` | `7db62564db49fe0218cbb35b119d62582a24d658` |
| `243-c1.c1` | `5839444d` | `0616921c4cce20422563578bd0e806d359508599` |
| `243-c2.c2` | `dd087428` | `ca27fdb60425664956a18c021ea465f452fb1527` |
| `243-c3.c3` | `6054cbe0` | `ec2f65e9c930250ee25fd064ee5ae76a7a9c61d9` |
| `243-c4.c4` | `8bd2a9d2` | `0935df65cd2b0891a708bcc0f1c188148058d4b5` |
| `243-c5.c5` | `6a503dcf` | `23241b16d7e20f923d41186b29487ab922c7f530` |
| `243-c6.c6` | `ec9c36d0` | `e145e9e359000dda6e1dfe95a996bc6d29cfca21` |
| `243-s1.s1` | `c9cd2298` | `a9a18b5347f9dbe29a2ccb63fd4c8fd19537bf8b` |
| `243-m1.m1` | `acf12d10` | `6e6b98cc1fa44f24a5168877559b0055e6957b60` |
| `243-v1.v1` | `f7ee6fbb` | `55137bcabeeb590e40a9b8a7c07dd106e4d12a90` |
| `243-v2.v2` | `aa9e4df6` | `a0b91f63e2552a8ad9e0d1af00e2c38288637161` |
| `243-v3.v3` | `4ac750b2` | `585a154acc67bd84ea5b944686b78ed082b768d9` |
| `243-v4.v4` | `f5c64ba6` | `2eac455def8c27090862cc042f65a3a8aad88283` |
| `sp-s2.sp1` | `9036d879` | `4f5ed7105b7128794654ce82b51723e16e389543` |
| `sfix.sfix` | `c2ea0cfd` | `fd4a618cdcdbf849374f0a50dd8efe9dbab706c3` |
| `sm1.sm1` | `94416d67` | `42f9d7ddd6c0931fd64226a60dc73602b2819dcf` |
| `000-lo.lo` | `5a86cff2` | `5992277debadeb64d1c1c64b0a92d9293eaf7e4a` |

The supplied `lastbld2.zip` also contains many BIOS variants (`uni-bios_*`, `sp-*`, `vs-bios.rom`). They are not staged or used; `sp-s2.sp1` is the default.

## Hardware behaviour that mattered

### Slot-check security and the watchdog

The game runs the post-1998 MVS slot check. On a first boot (backup RAM empty), it deliberately does not strobe the watchdog and waits for a watchdog reset; after the reset it sees initialised backup RAM and continues. If the watchdog never fires, the game displays the red "THIS ROM CARTRIDGE IS FOR USE WITH ORIGINAL SNK BOARDS ONLY" screen and locks up.

The v0.2.0 toolkit source used here has generic watchdog modelling added by this project line: a write with A0 set in `$300000-$3BFFFF` strobes it, and `ng_main()` resets the board when the deadline expires (preserving work and backup RAM). With that in place the game passes the check and reaches How To Play and Player Select.

The other slot-check conditions are also satisfied: the BIOS is in MVS mode (`REG_STATUS_B` bit 7 set, BIOS MVS flag set), the uPD4990A calendar returns the host's valid date, the backup RAM is present, and the Z80 replies to the check command. No title-specific patch is used.

### 48 MB C-ROM sprite addressing

The C ROMs decode to 393,216 16×16 tiles. Tile numbers are masked to the next power of two by the LSPC, but the fitted ROM is smaller than that mask. The renderer must reject codes past the decoded data instead of reading beyond the allocation. `src/video.c` carries that generic bound check, originally added for large carts.

### P-ROM banking

The cart has four 1 MB banks behind `$200000-$2FFFFF`, selected through `$2FFFF0-$2FFFFF`. The recompiler analyses the first banked megabyte, which is what a native address such as `$200123` means. When a different bank is selected, the bytes at that window address are different code, so the runtime interprets banked code rather than dispatching to the bank-0 native routine. The guard is generic and exact.

### RAM-resident code

The game copies a routine into 68000 work RAM and executes it. The profiler shows hot addresses around `$100800-$100920`. The recompiler only analyses ROM, so this code runs in Musashi. It is the source of the interpreted-instruction counts in the coverage report; it is not an uncovered ROM entry point.

## References

- MAME `src/mame/snk/neogeo.cpp`, `ROM_START( lastbld2 )`
- NeoGeo Development Wiki, *Slot check security*: <https://wiki.neogeodev.org/index.php?title=Slot_check_security>
- neogeorecomp `docs/hardware-notes.md`, `docs/recompiler.md`, `docs/running.md`
