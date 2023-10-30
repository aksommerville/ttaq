/* adv_sprite.h
 * Basic sprite and group.
 * Sprites and groups use simple reference counting.
 * Every sprite<~>sprgrp reference is mutual.
 * Other entities may retain them too.
 *
 * DO NOT MODIFY sprite->grpv or group->sprv except through the provided functions!
 *
 * No, but seriously, DO NOT MODIFY sprite->grpv or group->sprv except through the provided functions!
 *
 */

#ifndef ADV_SPRITE_H
#define ADV_SPRITE_H

/* Type definitions: sprite and sprgrp
 *****************************************************************************/

struct adv_sprite;
struct adv_sprgrp;
struct adv_sprdef;
struct adv_sprclass;

struct adv_sprite {
  int refc;
  struct adv_sprgrp **grpv; int grpc,grpa; // sorted by address
  int x,y;
  int tileid;
  int flop;
  int depth; // for sorting
  int sprdefid; // for reference only
  const struct adv_sprdef *sprdef; // may be NULL
  const struct adv_sprclass *sprclass; // may be NULL
  int collide_edges;
  int collide_map_positive;
  int collide_map_negative;
  int collide_solids;
};

struct adv_sprgrp {
  int refc;
  struct adv_sprite **sprv; int sprc,spra; // unsorted
  int sort_direction;
  int sort_insertions; // if nonzero, we try to insert at the correct place
};

/* Basic sprite and group API.
 *****************************************************************************/

int adv_sprite_new(struct adv_sprite **dst);
void adv_sprite_del(struct adv_sprite *spr);
int adv_sprite_ref(struct adv_sprite *spr);

int adv_sprgrp_new(struct adv_sprgrp **dst);
void adv_sprgrp_del(struct adv_sprgrp *grp);
int adv_sprgrp_ref(struct adv_sprgrp *grp);

/* 'has' returns 0 or 1, no errors.
 * 'insert' and 'remove' return -1 for real errors, 0 if redundant, or 1 on success.
 */
int adv_sprgrp_has(struct adv_sprgrp *grp,struct adv_sprite *spr);
int adv_sprgrp_insert(struct adv_sprgrp *grp,struct adv_sprite *spr);
int adv_sprgrp_remove(struct adv_sprgrp *grp,struct adv_sprite *spr);

// Remove sprite from all groups (which typically deletes the sprite).
int adv_sprite_kill(struct adv_sprite *spr);

// Remove all sprites from group. 
// This deletes the group if you don't have a strong reference to it elsewhere!
int adv_sprgrp_clear(struct adv_sprgrp *grp);

// Kill all sprites in this group.
int adv_sprgrp_kill(struct adv_sprgrp *grp);

// Does only one pass of a bubble sort, alternating direction each time you call.
int adv_sprgrp_sort_by_depth(struct adv_sprgrp *grp);

/* Sprite instantiation.
 *****************************************************************************/

/* Use the definition (sprdefid) to create a new sprite and install it in the appropriate groups.
 * Returned object is weak and optional.
 */
int adv_sprite_create(struct adv_sprite **dst_WEAK,int sprdefid,int x,int y);

/* Create a sprite from the given class.
 * This is a component of adv_sprite_create().
 */
int adv_sprite_instantiate(struct adv_sprite **dst_WEAK,const struct adv_sprclass *sprclass);

/* Global groups.
 *****************************************************************************/

#define ADV_FOR_EACH_GROUP \
  ADV_GRPACTION(keepalive) \
  ADV_GRPACTION(visible) \
  ADV_GRPACTION(update) \
  ADV_GRPACTION(hero) \
  ADV_GRPACTION(solid) \
  ADV_GRPACTION(hazard) \
  ADV_GRPACTION(fragile) \
  ADV_GRPACTION(moveable) \
  ADV_GRPACTION(button) \
  ADV_GRPACTION(burnable) \
  ADV_GRPACTION(latchable) \
  ADV_GRPACTION(prize) \
  ADV_GRPACTION(princess)

#define ADV_GRPACTION(tag) extern struct adv_sprgrp *adv_sprgrp_##tag;
ADV_FOR_EACH_GROUP
#undef ADV_GRPACTION

int adv_sprgrp_init();
void adv_sprgrp_quit();

/* Helpful sprite operations.
 *****************************************************************************/
 
/* Move a sprite in the given vector, applying all relevant collision logic.
 * Returns the absolute value of total movement, manhattan distance.
 * We don't check for overflow or anything, we assume more-or-less sane arguments.
 * (pushing) is provided as an argument here, and not as a sprite property.
 * This is because it must cascade through all pushed sprites, whether or not they themselves are pushers.
 */
int adv_sprite_move(struct adv_sprite *spr,int dx,int dy,int pushing);

/* How far can this sprite move in the given vector without touching something?
 * If the sprite can push other sprites, we don't care, they appear solid.
 * Returns absolute value of total possible movement (up to (abs(dx)+abs(dy))), manhattan distance.
 * For useful results, one of (dx,dy) should be zero.
 */
int adv_sprite_premove(struct adv_sprite *spr,int dx,int dy);

int adv_sprite_hurt(struct adv_sprite *spr); // usually kills sprite
int adv_sprite_hurt_default(struct adv_sprite *spr); // kills sprite

#endif
