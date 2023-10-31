/* adv_video.h
 * This unit is responsible for the high-level rendering, and for managing the OpenGL relationship.
 * We do not create the OpenGL context or talk to the OS, but we do bootstrap whatever opt unit handles those.
 */

#ifndef ADV_VIDEO_H
#define ADV_VIDEO_H

struct adv_sprgrp;

/* Global video operations.
 *****************************************************************************/

int adv_video_init();
void adv_video_quit();

// Draws everything, sleeps, and pushes to display.
// You do not need an external clock.
int adv_video_update();

int adv_video_lights_out(int do_effect,int lampx,int lampy);

/* One splash at a time may be displayed above all game graphics.
 * This is a full single image.
 * Use (miscimgid==0) to hide the splash.
 * If one is already showing, this replaces it.
 * (duration) is in frames, how long until the splash disappears on its own.
 * May be negative to last forever.
 */
int adv_video_splash(int miscimgid,int duration);

int adv_video_begin_main_menu();
int adv_video_end_main_menu();
int adv_video_is_main_menu();

/* Functions to deliver raw graphics to GPU stores.
 * These are invoked as necessary by the resource manager.
 * Nobody else should be calling them.
 *****************************************************************************/

/* Redraw the background texture.
 * The next adv_video_update() will use this new map image.
 * (tiledata) is 16-bit blocks (bgtileid,fgtileid) at (ADV_MAP_W,ADV_MAP_H).
 * (img_bg) is 24-bit RGB at (ADV_TILE_W*16,ADV_TILE_H*16).
 * Tile IDs run LRTB from the top-left, as you should expect.
 * Caller is responsible for validating the size of these buffers!
 */
int adv_video_draw_bg(const void *tiledata,const void *img_bg);
int adv_video_draw_bg_sub(const void *tiledata,const void *img_bg,int col,int row,int colc,int rowc);

/* Load a whole new spritesheet.
 * The spritesheet is 16x16 tiles of RGBA at (ADV_TILE_W,ADV_TILEH).
 * This will change the appearance of all sprites immediately.
 * Caller is responsible for validating buffer size.
 */
int adv_video_set_spritesheet(const void *spritesheet);

#endif
