#include "adv_input_internal.h"
#include "sprite/adv_sprite.h"
#include "sprite/class/adv_sprite_hero.h"

struct adv_input adv_input={0};
unsigned char adv_inputs[1+ADV_PLAYER_LIMIT]={0};

/* init
 *****************************************************************************/
 
int adv_input_init() {
  if (adv_input_read_maps()<0) return -1;
  if (linput_init(0,1)<0) return -1;
  if (linput_set_callbacks(0,adv_input_cb_connect,adv_input_cb_disconnect,adv_input_cb_event)<0) return -1;
  if (linput_scan()<0) return -1;
  return 0;
}

/* quit
 *****************************************************************************/
 
void adv_input_quit() {
  if (adv_input.useractionv) free(adv_input.useractionv);
  if (adv_input.controllerv) {
    while (adv_input.controllerc-->0) adv_controller_del(adv_input.controllerv[adv_input.controllerc]);
    free(adv_input.controllerv);
  }
  if (adv_input.inmapv) {
    while (adv_input.inmapc-->0) adv_inmap_del(adv_input.inmapv[adv_input.inmapc]);
    free(adv_input.inmapv);
  }
  memset(&adv_input,0,sizeof(struct adv_input));
  memset(adv_inputs,0,sizeof(adv_inputs));
  linput_quit();
}

/* trivial accessors
 *****************************************************************************/

int adv_input_quit_requested() {
  int rtn=adv_input.quit_requested;
  adv_input.quit_requested=0;
  return rtn;
}

int adv_input_get_pause() {
  return adv_input.pause;
}

int adv_input_set_pause(int pause) {
  adv_input.pause=pause;
  return 0;
}

/* public access by playerid
 *****************************************************************************/
 
int adv_input_player_exists(int playerid) {
  if ((playerid<1)||(playerid>ADV_PLAYER_LIMIT)) return 0;
  int i; for (i=0;i<adv_input.controllerc;i++) if (adv_input.controllerv[i]->playerid==playerid) return 1;
  return 0;
}

int adv_input_remove_player(int playerid) {
  if ((playerid<1)||(playerid>ADV_PLAYER_LIMIT)) return 0;
  int i; for (i=0;i<adv_input.controllerc;i++) if (adv_input.controllerv[i]->playerid==playerid) {
    adv_input.controllerv[i]->playerid=0;
  }
  adv_inputs[playerid]=0;
  for (i=0;i<adv_sprgrp_hero->sprc;i++) {
    struct adv_sprite_hero *hero=(struct adv_sprite_hero*)adv_sprgrp_hero->sprv[i];
    if (hero->playerid==playerid) hero->playerid=0;
  }
  return 0;
}

/* update
 *****************************************************************************/
 
int adv_input_update() {
  if (linput_update(0)<0) return -1;
  return 0;
}

/* connect
 *****************************************************************************/
 
void adv_input_cb_connect(void *userdata,int devid) {
  if (adv_input.controllerc>=adv_input.controllera) {
    int na=adv_input.controllera+4;
    if (na>INT_MAX/sizeof(void*)) return;
    void *nv=realloc(adv_input.controllerv,sizeof(void*)*na);
    if (!nv) return;
    adv_input.controllerv=nv;
    adv_input.controllera=na;
  }
  struct adv_controller *controller=0;
  if (adv_controller_new(&controller)<0) return;
  if (adv_controller_setup(controller,devid)<0) { 
    printf("Unable to configure input device '%s'.\n",linput_device_get_name(devid));
    adv_controller_del(controller); 
    return; 
  }
  adv_input.controllerv[adv_input.controllerc++]=controller;
}

/* disconnect
 *****************************************************************************/
 
void adv_input_cb_disconnect(void *userdata,int devid) {
  int i; for (i=0;i<adv_input.controllerc;i++) if (adv_input.controllerv[i]->devid==devid) {
    struct adv_controller *controller=adv_input.controllerv[i];
    if ((controller->playerid>=1)&&(controller->playerid<=ADV_PLAYER_LIMIT)) {
      int i; for (i=0;i<adv_sprgrp_hero->sprc;i++) {
        struct adv_sprite_hero *hero=(struct adv_sprite_hero*)adv_sprgrp_hero->sprv[i];
        if (hero->playerid==controller->playerid) {
          adv_sprite_create(0,11,hero->hdr.x,hero->hdr.y); // detachhighlight
          hero->playerid=0;
          break;
        }
      }
      adv_inputs[controller->playerid]=0;
    }
    adv_controller_del(controller);
    adv_input.controllerc--;
    memmove(adv_input.controllerv+i,adv_input.controllerv+i+1,sizeof(void*)*(adv_input.controllerc-i));
    return;
  }
}

/* event
 *****************************************************************************/
 
void adv_input_cb_event(void *userdata,int devid,int type,int code,int value) {

  // One-off global events.
  if ((type==EV_KEY)&&(value==1)) {
    int useractionp=adv_useraction_search(code);
    if (useractionp>=0) {
      adv_input_useraction(adv_input.useractionv[useractionp].useraction);
      return;
    }
  }
  
  // Controller.
  int i; for (i=0;i<adv_input.controllerc;i++) if (adv_input.controllerv[i]->devid==devid) {
    struct adv_controller *controller=adv_input.controllerv[i];
    adv_controller_event(controller,type,code,value);
    return;
  }
}

/* useraction list
 *****************************************************************************/
 
int adv_useraction_search(int keycode) {
  int lo=0,hi=adv_input.useractionc;
  while (lo<hi) {
    int ck=(lo+hi)>>1;
         if (keycode<adv_input.useractionv[ck].keycode) hi=ck;
    else if (keycode>adv_input.useractionv[ck].keycode) lo=ck+1;
    else return ck;
  }
  return -lo-1;
}

int adv_useraction_insert(int p,int keycode,int useraction) {
  if ((p<0)||(p>adv_input.useractionc)) return -1;
  if (p&&(keycode<=adv_input.useractionv[p-1].keycode)) return -1;
  if ((p<adv_input.useractionc)&&(keycode>=adv_input.useractionv[p].keycode)) return -1;
  if (adv_input.useractionc>=adv_input.useractiona) {
    int na=adv_input.useractiona+4;
    if (na>INT_MAX/sizeof(struct adv_useraction)) return -1;
    void *nv=realloc(adv_input.useractionv,sizeof(struct adv_useraction)*na);
    if (!nv) return -1;
    adv_input.useractionv=nv;
    adv_input.useractiona=na;
  }
  memmove(adv_input.useractionv+p+1,adv_input.useractionv+p,sizeof(struct adv_useraction)*(adv_input.useractionc-p));
  adv_input.useractionc++;
  adv_input.useractionv[p].keycode=keycode;
  adv_input.useractionv[p].useraction=useraction;
  return 0;
}

int adv_input_map_useraction(int keycode,int useraction) {
  int p=adv_useraction_search(keycode);
  if (p<0) {
    if (!useraction) return 0;
    return adv_useraction_insert(-p-1,keycode,useraction);
  }
  if (!useraction) {
    adv_input.useractionc--;
    memmove(adv_input.useractionv+p,adv_input.useractionv+p+1,sizeof(struct adv_useraction)*(adv_input.useractionc-p));
    return 0;
  }
  adv_input.useractionv[p].useraction=useraction;
  return 0;
}

/* map useraction from text
 *****************************************************************************/
 
static int adv_input_map_useraction_text(const char *src,int srcc,const char *refname,int lineno) {
  if ((srcc>0)&&!src) return -1;
  if (srcc<0) { srcc=0; if (src) while (src[srcc]) srcc++; }
  
  /* Split words. */
  int srcp=0;
  while ((srcp<srcc)&&((unsigned char)src[srcp]<=0x20)) srcp++;
  const char *keyname=src+srcp;
  int keynamec=0;
  while ((srcp<srcc)&&((unsigned char)src[srcp]>0x20)) { srcp++; keynamec++; }
  while ((srcp<srcc)&&((unsigned char)src[srcp]<=0x20)) srcp++;
  const char *btnname=src+srcp;
  int btnnamec=0;
  while ((srcp<srcc)&&((unsigned char)src[srcp]>0x20)) { srcp++; btnnamec++; }
  while ((srcp<srcc)&&((unsigned char)src[srcp]<=0x20)) srcp++;
  if ((srcp<srcc)||!keynamec||!btnnamec) {
    if (refname) fprintf(stderr,"%s:%d: invalid global event map, expected 'global KEYNAME ACTIONNAME'\n",refname,lineno);
    return -1;
  }
  
  /* Evaluate each word. */
  int keycode=adv_keycode_eval(keyname,keynamec);
  if (keycode<0) {
    if (refname) fprintf(stderr,"%s:%d: '%.*s' does not name a key\n",refname,lineno,keynamec,keyname);
    return -1;
  }
  int useraction=adv_useraction_eval(btnname,btnnamec);
  if (useraction<0) {
    if (refname) fprintf(stderr,"%s:%d: '%.*s' does not name a global action\n",refname,lineno,btnnamec,btnname);
    return -1;
  }
  
  return adv_input_map_useraction(keycode,useraction);
}

/* read maps text
 *****************************************************************************/
 
static int adv_input_read_maps_text(const char *src,int srcc,const char *refname) {
  int srcp=0,lineno=1;
  struct adv_inmap *inmap=0; // weak; also stored in adv_input.inmapv
  int baselineno=1; // line where inmap was created
  while (srcp<srcc) {
  
    /* Whitespace, etc. */
    if ((src[srcp]==0x0a)||(src[srcp]==0x0d)) {
      if (++srcp>=srcc) break;
      lineno++;
      if (((src[srcp]==0x0a)||(src[srcp]==0x0d))&&(src[srcp]!=src[srcp-1])) srcp++;
      continue;
    }
    if ((unsigned char)src[srcp]<=0x20) { srcp++; continue; }
    const char *line=src+srcp;
    int linec=0,cmt=0;
    while ((srcp<srcc)&&(src[srcp]!=0x0a)&&(src[srcp]!=0x0d)) {
      if (src[srcp]==0x09) ; // ok, we grudgingly accept HT
      else if ((src[srcp]<0x20)||(src[srcp]>0x7e)) {
        fprintf(stderr,"%s:%d: illegal byte 0x%02x\n",refname,lineno,(unsigned char)src[srcp]);
        return -1;
      } else if (src[srcp]=='#') cmt=1;
      else if (!cmt) linec++;
      srcp++;
    }
    while (linec&&((unsigned char)line[linec-1]<=0x20)) linec--;
    if (!linec) continue;
    
    /* Measure first word and remainder. */
    const char *kw=line;
    int kwc=0; while ((kwc<linec)&&((unsigned char)kw[kwc]>0x20)) kwc++;
    const char *arg=line+kwc;
    int argc=linec-kwc;
    while (argc&&((unsigned char)arg[0]<=0x20)) { arg++; argc--; }
    
    /* Global action? */
    if ((kwc==6)&&!memcmp(kw,"global",6)) {
      inmap=0; // implicitly ends device block
      if (adv_input_map_useraction_text(arg,argc,refname,lineno)<0) return -1;
      continue;
    }
    
    /* Explicit end of device block? */
    if ((kwc==3)&&!memcmp(kw,"end",3)&&!argc) {
      if (!inmap) {
        fprintf(stderr,"%s:%d: 'end' outside device block\n",refname,lineno);
        return -1;
      }
      inmap=0;
      continue;
    }
    
    /* New inmap? */
    if ((kwc==6)&&!memcmp(kw,"device",6)) {
      baselineno=lineno;
      if (adv_input.inmapc>=adv_input.inmapa) {
        int na=adv_input.inmapa+8;
        if (na>INT_MAX/sizeof(void*)) return -1;
        void *nv=realloc(adv_input.inmapv,sizeof(void*)*na);
        if (!nv) return -1;
        adv_input.inmapv=nv;
        adv_input.inmapa=na;
      }
      if (adv_inmap_new(&inmap)<0) return -1;
      adv_input.inmapv[adv_input.inmapc++]=inmap;
      if (adv_inmap_eval_header(inmap,arg,argc,refname,lineno)<0) return -1;
      continue;
    }
    
    /* Anything else is processed by the inmap. */
    if (!inmap) {
      fprintf(stderr,"%s:%d: expected 'global' or 'device'\n",refname,lineno);
      return -1;
    }
    if (adv_inmap_eval_argument(inmap,line,linec,refname,lineno)<0) return -1;
  }
  return 0;
}

/* read maps file
 *****************************************************************************/
 
int adv_input_read_maps() {
  char path[1024];
  const char *HOME=getenv("HOME");
  if (HOME&&HOME[0]) snprintf(path,sizeof(path),"%s/.ttaq/input",HOME);
  else strcpy(path,"/usr/local/share/ttaq/input");
  char *src=0;
  int srcc=adv_file_read(&src,path);
  if (srcc<0) return -1;
  if (!src) return 0;
  int err=adv_input_read_maps_text(src,srcc,path);
  free(src);
  if (err<0) return err;
  printf("Loaded input maps from '%s'.\n",path);
  return 0;
}
