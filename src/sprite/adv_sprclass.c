#include "adv.h"
#include "adv_sprclass.h"

/* master list
 *****************************************************************************/

extern const struct adv_sprclass adv_sprclass_hero;
extern const struct adv_sprclass adv_sprclass_sword;
extern const struct adv_sprclass adv_sprclass_arrow;
extern const struct adv_sprclass adv_sprclass_fire;
extern const struct adv_sprclass adv_sprclass_anim4;
extern const struct adv_sprclass adv_sprclass_buggly;
extern const struct adv_sprclass adv_sprclass_prize;
extern const struct adv_sprclass adv_sprclass_gadget;
extern const struct adv_sprclass adv_sprclass_bonfire;
extern const struct adv_sprclass adv_sprclass_princess;
extern const struct adv_sprclass adv_sprclass_missile;
extern const struct adv_sprclass adv_sprclass_livestock;
extern const struct adv_sprclass adv_sprclass_phantasm;
extern const struct adv_sprclass adv_sprclass_squirgoyle;
extern const struct adv_sprclass adv_sprclass_knight;
extern const struct adv_sprclass adv_sprclass_pitcher;

static const struct adv_sprclass *adv_sprclass_all[]={
  &adv_sprclass_hero,
  &adv_sprclass_sword,
  &adv_sprclass_arrow,
  &adv_sprclass_fire,
  &adv_sprclass_anim4,
  &adv_sprclass_buggly,
  &adv_sprclass_prize,
  &adv_sprclass_gadget,
  &adv_sprclass_bonfire,
  &adv_sprclass_princess,
  &adv_sprclass_missile,
  &adv_sprclass_livestock,
  &adv_sprclass_phantasm,
  &adv_sprclass_squirgoyle,
  &adv_sprclass_knight,
  &adv_sprclass_pitcher,
0};

/* retrieve by name
 *****************************************************************************/

const struct adv_sprclass *adv_sprclass_for_name(const char *name,int namec) {
  if (!name) return 0;
  if (namec<0) { namec=0; while (name[namec]) namec++; }
  if (!namec) return 0;
  const struct adv_sprclass **cls; for (cls=adv_sprclass_all;*cls;cls++) {
    if (!(*cls)->name) continue;
    if (memcmp((*cls)->name,name,namec)) continue;
    if ((*cls)->name[namec]) continue;
    return *cls;
  }
  return 0;
}

/* argument for name
 *****************************************************************************/
 
static int _matchname(const char *k,const char *src,int srcc) {
  if (!k) return 0;
  if (memcmp(k,src,srcc)) return 0;
  if (k[srcc]) return 0;
  return 1;
}
 
int adv_sprclass_lookup_argument(const struct adv_sprclass *sprclass,const char *name,int namec) {
  if (!sprclass||((namec>0)&&!name)) return -1;
  if (namec<0) { namec=0; if (name) while (name[namec]) namec++; }
  if (!namec) return -1;
  if ((namec==4)&&!memcmp(name,"arg",3)&&(name[3]>='0')&&(name[3]<='3')) return name[3]-'0';
  if (_matchname(sprclass->arg0name,name,namec)) return 0;
  if (_matchname(sprclass->arg1name,name,namec)) return 1;
  if (_matchname(sprclass->arg2name,name,namec)) return 2;
  if (_matchname(sprclass->arg3name,name,namec)) return 3;
  return -1;
}
