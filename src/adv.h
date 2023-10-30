#ifndef ADV_H
#define ADV_H

#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>

/* Game setup constants.
 * These must agree with the provided data.
 * Code should not make any assumptions about their values (beyond the obvious).
 *****************************************************************************/

// Size of a tile in pixels.
#define ADV_TILE_W 32
#define ADV_TILE_H 32

// Size of one screen of play in tiles.
//XXX Was (22,14).
#define ADV_MAP_W 40
#define ADV_MAP_H 22

// Size of playing area in pixels.
// Game-space graphics are projected into this space, top-down.
#define ADV_SCREEN_W (ADV_TILE_W*ADV_MAP_W)
#define ADV_SCREEN_H (ADV_TILE_H*ADV_MAP_H)

// Player limit. This should always be 4.
#define ADV_PLAYER_LIMIT 4

/* Prototypes for functions implemented in src/misc.
 *****************************************************************************/

int adv_file_read(void *dstpp,const char *path);
int adv_file_write(const char *path,const void *src,int srcc);

#endif
