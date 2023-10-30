/* adv_sprdef.h
 * Sprite definition.
 * These are managed by the resource manager.
 * Every sprite definition is loaded at startup.
 *
 * --- Sprite Resource Format ---
 * Line-oriented text.
 * '#' begins a line comment.
 * Excess whitespace is ignored.
 *
 * Each line takes the form "KEY VALUE".
 * KEY:
 *   tile: hexadecimal tile id
 *   class: name of sprite class
 * Keys may also be sprclass argument names. These must appear after "class".
 *
 */

#ifndef ADV_SPRDEF_H
#define ADV_SPRDEF_H

struct adv_sprclass;

struct adv_sprdef {
  int sprdefid;
  unsigned char tileid;
  const struct adv_sprclass *sprclass;
  int argv[4];
};

// Only interesting to resource manager:
int adv_sprdef_new(struct adv_sprdef **dst);
void adv_sprdef_del(struct adv_sprdef *sprdef);
int adv_sprdef_decode_file(struct adv_sprdef *sprdef,const char *path);
int adv_sprdef_decode(struct adv_sprdef *sprdef,const void *src,int srcc,const char *refname);

#endif
