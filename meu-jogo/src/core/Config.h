#pragma once


constexpr int BLOCKSIZE = 32;
constexpr int WMAP = 60;
constexpr int HMAP = 34;
constexpr int SCREENWIDTH = BLOCKSIZE * WMAP;  // 1920
constexpr int SCREENHEIGHT = BLOCKSIZE * HMAP; // 1088


constexpr int MSG_LOAD_MAP = 1;
constexpr int MSG_END_GAME = 2;
constexpr int MSG_DEATH_SEQUENCE = 999;