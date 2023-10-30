#ifndef ADV_SPRITE_HERO_H
#define ADV_SPRITE_HERO_H

#define ADV_WEAPON_SWORD          1
#define ADV_WEAPON_ARROW          2
#define ADV_WEAPON_GADGET         3
#define ADV_WEAPON_FIRE           4

struct adv_sprclass;

extern const struct adv_sprclass adv_sprclass_hero;

struct adv_sprite_hero {
  struct adv_sprite hdr;
  int playerid; // 1..ADV_PLAYER_LIMIT, or 0 to disable input
  unsigned char input; // previous state of input controller
  int walkdx,walkdy; // hint for direction we are currently walking
  int walking; // hint for animation
  int animcounter;
  int animstep; // which frame of animation
  int weapon;
  int lock_direction; // prevent changing direction due to weapon
  int lock_movement; // prevent walking due to weapon
  struct adv_sprgrp *grp_weapon;
  int fire_angle,fire_radius;
  int ghost;
  int base_tileid;
  struct adv_sprgrp *grp_flag;
  int flagdir;
  int offscreen;
  int offscreen_target; // map id
  int drag;
  int ghost_prevention; // counts down after a ghost<~>flesh change, during which no more changing
};

int adv_sprite_hero_set_playerid(struct adv_sprite *spr,int playerid);
int adv_sprite_hero_ghostify(struct adv_sprite *spr);
int adv_sprite_hero_fleshify(struct adv_sprite *spr,int with_sound_effect);

#endif
