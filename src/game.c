/*
 * game.c - The Last Blade 2 (MAME set `lastbld2`, NGH-243, later revision)
 * ROM layout, shared by the player and the generator so both load the
 * program identically.
 *
 * The layout is MAME's ROM_START( lastbld2 ) in src/mame/snk/neogeo.cpp:
 *
 *   Board  NEO-AEG PROGBK1Y / NEO-AEG CHA512Y.
 *   P ROM  $500000 = 1 MB fixed (243-pg1.p1) + 4 MB banked (243-pg2.sp2),
 *          both stored 16-bit word-swapped (ROM_LOAD16_WORD_SWAP).
 *   C ROMs 48 MB in three 8 MB odd/even pairs (ROM_LOAD16_BYTE: c1 at even
 *          bytes, c2 at odd bytes, and so on).
 *   S ROM  243-s1.s1  (fixed layer)
 *   M1     243-m1.m1  (Z80 sound driver)
 *   V ROMs 16 MB ADPCM-A: v1..v4 (4 MB each), concatenated.
 *
 * The cart uses the ordinary PROGBK1 banking window: the 4 MB banked image
 * is selected in 1 MB banks through $2FFFF0-$2FFFFF. No encryption, no CMC
 * protection, no daughterboard and no SMA chip. The fixed bank carries the
 * "NEO-GEO" signature at $100 and NGH number $0243 after the word swap,
 * which is what the loader assumes.
 *
 * Files are read from --rom-path as unzipped MAME names. The system BIOS
 * (sp-s2.sp1, sfix.sfix, sm1.sm1, 000-lo.lo) is read from --bios-path.
 */
#include "game.h"

const ng_game_t k_lastblade2 = {
    .name = "lastbld2",
    .title = "The Last Blade 2",
    .prom_size = 0x500000,
    .prom = {
        /* file          file offset  dest       length    swap */
        {"243-pg1.p1",  0x000000,    0x000000,  0x100000, 1},
        {"243-pg2.sp2", 0x000000,    0x100000,  0x400000, 1},
    },
    .crom_size = 0x3000000,
    .crom_pairs = {
        {"243-c1.c1", "243-c2.c2"},
        {"243-c3.c3", "243-c4.c4"},
        {"243-c5.c5", "243-c6.c6"},
    },
    .srom = "243-s1.s1",
    .m1 = "243-m1.m1",
    .vrom = {"243-v1.v1", "243-v2.v2", "243-v3.v3", "243-v4.v4"},
};
