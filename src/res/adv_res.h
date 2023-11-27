/* adv_res.h
 * Global resource manager.
 */

#ifndef ADV_RES_H
#define ADV_RES_H

struct adv_map;
struct adv_sprdef;

#define ADV_TILEPROP_HOLE         0x01 // holes, water, etc -- negative space
#define ADV_TILEPROP_SOLID        0x02 // impassable
#define ADV_TILEPROP_LATCH        0x04 // never appears without SOLID; gadget can latch onto this
#define ADV_TILEPROP_NEIGHBOR15   0x08 // only relevant to editor
#define ADV_TILEPROP_HEAL         0x10 // passable; step on me to heal

// Shared map, the one currently in use.
// Between adv_res_init() and adv_res_quit(), this always points to something legal.
extern struct adv_map *adv_map;

int adv_res_init(const char *programpath,int argc,char **argv);
void adv_res_quit();

/* Replaces global map and redraws tilesheets as necessary.
 * Returns >0 if changed, 0 if requested current map (no action), or <0 for real errors.
 */
int adv_res_load_map(int mapid);
int adv_res_reload_map();

// Return WEAK reference to an installed sprite definition.
struct adv_sprdef *adv_res_get_sprdef(int sprdefid);

void adv_res_get_background_palette(void **bg);

#define ADV_SOUND_SWORD          0
#define ADV_SOUND_STRIKE         1
#define ADV_SOUND_ARROW          2
#define ADV_SOUND_GADGET         3
#define ADV_SOUND_GADGETNO       4
#define ADV_SOUND_GADGETYES      5
#define ADV_SOUND_REVIVE         6
#define ADV_SOUND_RESCUE         7
#define ADV_SOUND_CHANGECTL      8
#define ADV_SOUND_SWITCH         9
#define ADV_SOUND_HAT           10
#define ADV_SOUND_KICK          11
#define ADV_SOUND_RIDE          12
#define ADV_SOUND_SNARE         13
#define ADV_SOUND_MENUYES       14
#define ADV_SOUND_MENUNO        15

int ttaq_audio_play_sound(int soundid);
int ttaq_audio_play_song(int songid);

/* "miscellaneous images" are loaded from disk per request.
 * Output is always RGBA; we convert if necessary.
 * On success, caller frees (*dstpp).
 */
int adv_res_get_miscimg(void *dstpp,int *w,int *h,int miscimgid);

int adv_res_replace_spritesheet(int imgid);

#endif
