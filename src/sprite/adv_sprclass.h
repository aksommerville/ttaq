/* adv_sprclass.h
 * A "class" is a predefined set of sprite behavior.
 * Sprites are not required to have a class.
 * If using it, class must be established before construction and persists throughout the sprite's life.
 * 
 * All sprite classes are allocated statically, and can be retrieved by name.
 *
 */

#ifndef ADV_SPRCLASS_H
#define ADV_SPRCLASS_H

struct adv_sprite;

struct adv_sprclass {
  const char *name;
  int objlen;
  
  /* 'init' and 'del' are always called once for each sprite.
   * These should handle basic setup like private groups, or anything you need to allocate.
   * 'postinit' is called by adv_sprite_create() as the very last thing.
   * It is NOT called by any other ctor.
   * None of these functions should kill the sprite.
   */
  int (*init)(struct adv_sprite *spr);
  void (*del)(struct adv_sprite *spr);
  int (*postinit)(struct adv_sprite *spr);
  
  // Abstractable operations.
  // If you implement 'update', you are automatically assigned to group 'update'.
  // If you implement 'hurt', you are automatically assigned to group 'fragile'.
  int (*update)(struct adv_sprite *spr);
  int (*hurt)(struct adv_sprite *spr); // if not implemented, you die
  
  // Flags for collision detection (esp adv_sprite_move()):
  int collide_edges;
  int collide_map_positive;
  int collide_map_negative;
  int collide_solids;
  
  // Argument names in sprdef.
  const char *arg0name;
  const char *arg1name;
  const char *arg2name;
  const char *arg3name;
  
};

const struct adv_sprclass *adv_sprclass_for_name(const char *name,int namec);

int adv_sprclass_lookup_argument(const struct adv_sprclass *sprclass,const char *name,int namec);

#endif
