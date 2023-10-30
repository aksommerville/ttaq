#ifndef ADV_INPUT_INTERNAL_H
#define ADV_INPUT_INTERNAL_H

#include "adv.h"
#include "adv_input.h"
#include <linux/input.h>

struct adv_useraction {
  int keycode,useraction;
};

struct adv_absmap {
  int code;
  int lo,hi; // used by controller, not by inmap
  int btnlo,btnhi; // btnlo may be zero for 1-way axes
};

struct adv_keymap {
  int code;
  int btn;
};

struct adv_controller {
  int devid; // from linput
  int playerid; // 0 if detached, or 1..ADV_PLAYER_LIMIT (uniquely)
  unsigned char state;
  struct adv_absmap *absmapv; int absmapc,absmapa;
  struct adv_keymap *keymapv; int keymapc,keymapa;
};

// Permanent mapping rules for one device.
struct adv_inmap {
  char *name; int namec;
  int bustype,vendor,product;
  struct adv_absmap *absmapv; int absmapc,absmapa;
  struct adv_keymap *keymapv; int keymapc,keymapa;
};

extern struct adv_input {

  int pause;
  int quit_requested;
  
  struct adv_useraction *useractionv;
  int useractionc,useractiona;
  
  struct adv_controller **controllerv;
  int controllerc,controllera;
  
  struct adv_inmap **inmapv;
  int inmapc,inmapa;
  
} adv_input;

void adv_input_cb_connect(void *userdata,int devid);
void adv_input_cb_disconnect(void *userdata,int devid);
void adv_input_cb_event(void *userdata,int devid,int type,int code,int value);

int adv_useraction_search(int keycode);
int adv_useraction_insert(int p,int keycode,int useraction);

int adv_input_read_maps();

void adv_controller_del(struct adv_controller *controller);
int adv_controller_new(struct adv_controller **dst);
int adv_controller_setup(struct adv_controller *controller,int devid);
int adv_controller_event(struct adv_controller *controller,int type,int code,int value);
int adv_controller_absmap_search(struct adv_controller *controller,int code);
int adv_controller_absmap_insert(struct adv_controller *controller,int p,int code);
int adv_controller_keymap_search(struct adv_controller *controller,int code);
int adv_controller_keymap_insert(struct adv_controller *controller,int p,int code);

void adv_inmap_del(struct adv_inmap *inmap);
int adv_inmap_new(struct adv_inmap **dst);
int adv_inmap_set_name(struct adv_inmap *inmap,const char *name,int namec);
int adv_inmap_absmap_search(struct adv_inmap *inmap,int code);
int adv_inmap_absmap_insert(struct adv_inmap *inmap,int p,int code);
int adv_inmap_keymap_search(struct adv_inmap *inmap,int code);
int adv_inmap_keymap_insert(struct adv_inmap *inmap,int p,int code);
//TODO int adv_inmap_compare(struct adv_inmap *inmap,struct linput_device_layout *layout,int devid); // => score
#define ADV_INMAP_SCORE_BEST 39
#define ADV_BTNID_CRITICAL (ADV_BTNID_UP|ADV_BTNID_DOWN|ADV_BTNID_LEFT|ADV_BTNID_RIGHT|ADV_BTNID_ACTION)
int adv_inmap_eval_header(struct adv_inmap *inmap,const char *src,int srcc,const char *refname,int lineno);
int adv_inmap_eval_argument(struct adv_inmap *inmap,const char *src,int srcc,const char *refname,int lineno);

#endif
