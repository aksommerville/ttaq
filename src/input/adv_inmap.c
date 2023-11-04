#include "adv_input_internal.h"

/* del
 *****************************************************************************/
 
void adv_inmap_del(struct adv_inmap *inmap) {
  if (!inmap) return;
  if (inmap->name) free(inmap->name);
  if (inmap->absmapv) free(inmap->absmapv);
  if (inmap->keymapv) free(inmap->keymapv);
  free(inmap);
}

/* new
 *****************************************************************************/
 
int adv_inmap_new(struct adv_inmap **dst) {
  if (!dst) return -1;
  if (!(*dst=calloc(1,sizeof(struct adv_inmap)))) return -1;
  return 0;
}

/* set name
 *****************************************************************************/
 
int adv_inmap_set_name(struct adv_inmap *inmap,const char *name,int namec) {
  if (!inmap||((namec>0)&&!name)) return -1;
  if (namec<0) { namec=0; if (name) while (name[namec]) namec++; }
  while (namec&&((unsigned char)name[0]<=0x20)) { namec--; name++; }
  while (namec&&((unsigned char)name[namec-1]<=0x20)) namec--;
  if (namec==INT_MAX) namec--;
  char *nname=0;
  if (namec) {
    if (!(nname=malloc(namec+1))) return -1;
    memcpy(nname,name,namec);
    nname[namec]=0;
  }
  if (inmap->name) free(inmap->name);
  inmap->name=nname;
  inmap->namec=namec;
  return 0;
}

/* map lists
 *****************************************************************************/
 
int adv_inmap_absmap_search(struct adv_inmap *inmap,int code) {
  if (!inmap) return -1;
  int lo=0,hi=inmap->absmapc;
  while (lo<hi) {
    int ck=(lo+hi)>>1;
         if (code<inmap->absmapv[ck].code) hi=ck;
    else if (code>inmap->absmapv[ck].code) lo=ck+1;
    else return ck;
  }
  return -lo-1;
}
 
int adv_inmap_keymap_search(struct adv_inmap *inmap,int code) {
  if (!inmap) return -1;
  int lo=0,hi=inmap->keymapc;
  while (lo<hi) {
    int ck=(lo+hi)>>1;
         if (code<inmap->keymapv[ck].code) hi=ck;
    else if (code>inmap->keymapv[ck].code) lo=ck+1;
    else return ck;
  }
  return -lo-1;
}

int adv_inmap_absmap_insert(struct adv_inmap *inmap,int p,int code) {
  if (!inmap||(p<0)||(p>inmap->absmapc)) return -1;
  if (p&&(code<=inmap->absmapv[p-1].code)) return -1;
  if ((p<inmap->absmapc)&&(code>=inmap->absmapv[p].code)) return -1;
  if (inmap->absmapc>=inmap->absmapa) {
    int na=inmap->absmapa+8;
    if (na>INT_MAX/sizeof(struct adv_absmap)) return -1;
    void *nv=realloc(inmap->absmapv,sizeof(struct adv_absmap)*na);
    if (!nv) return -1;
    inmap->absmapv=nv;
    inmap->absmapa=na;
  }
  memmove(inmap->absmapv+p+1,inmap->absmapv+p,sizeof(struct adv_absmap)*(inmap->absmapc-p));
  inmap->absmapc++;
  struct adv_absmap *absmap=inmap->absmapv+p;
  memset(absmap,0,sizeof(struct adv_absmap));
  absmap->code=code;
  return 0;
}

int adv_inmap_keymap_insert(struct adv_inmap *inmap,int p,int code) {
  if (!inmap||(p<0)||(p>inmap->keymapc)) return -1;
  if (p&&(code<=inmap->keymapv[p-1].code)) return -1;
  if ((p<inmap->keymapc)&&(code>=inmap->keymapv[p].code)) return -1;
  if (inmap->keymapc>=inmap->keymapa) {
    int na=inmap->keymapa+8;
    if (na>INT_MAX/sizeof(struct adv_keymap)) return -1;
    void *nv=realloc(inmap->keymapv,sizeof(struct adv_keymap)*na);
    if (!nv) return -1;
    inmap->keymapv=nv;
    inmap->keymapa=na;
  }
  memmove(inmap->keymapv+p+1,inmap->keymapv+p,sizeof(struct adv_keymap)*(inmap->keymapc-p));
  inmap->keymapc++;
  struct adv_keymap *keymap=inmap->keymapv+p;
  memset(keymap,0,sizeof(struct adv_keymap));
  keymap->code=code;
  return 0;
}

/* compare name
 * Returns 0 if no match, 1 if matched with wildcard, or 2 if matched without wildcard.
 *****************************************************************************/
 
static int adv_inmap_compare_name(const char *pat,int patc,const char *name,int namec) {
  if (!name) name="";
  if (namec<0) {
    namec=0; while (name[namec]) namec++;
    while (namec&&((unsigned char)name[0]<=0x20)) { name++; namec--; }
    while (namec&&((unsigned char)name[namec-1]<=0x20)) namec--;
  }
  // Pattern is already trimmed; we do that at adv_inmap_set_name().
  int patp=0,namep=0;
  while (1) {
  
    // Both strings depleted? Awesome, we matched precisely.
    if ((patp>=patc)&&(namep>=namec)) return 2;
    
    // Pattern depleted? No match.
    if (patp>=patc) return 0;
    
    // Wildcard -- this recurs, but we finish here.
    if (pat[patp]=='*') {
      patp++;
      if (patp>=patc) return 1; // terminal wildcard always matches
      while (namep<=namec) {
        if (adv_inmap_compare_name(pat+patp,patc-patp,name+namep,namec-namep)) return 1;
        namep++;
      }
      return 0;
    }
    
    // Name depleted? No match.
    if (namep>=namec) return 0;
    
    // Whitespace must match at least one byte of whitespace, possibly more.
    if ((unsigned char)pat[patp]<=0x20) {
      if ((unsigned char)name[namep]>0x20) return 0;
      while ((patp<patc)&&((unsigned char)pat[patp]<=0x20)) patp++;
      while ((namep<namec)&&((unsigned char)name[namep]<=0x20)) namep++;
      continue;
    }
    
    // Letters are case-insensitive; everything else must match verbatim.
    char chp=pat[patp++]; if ((chp>=0x41)&&(chp<=0x5a)) chp+=0x20;
    char chn=name[namep++]; if ((chn>=0x41)&&(chn<=0x5a)) chn+=0x20;
    if (chp!=chn) return 0;
    
  }
}

/* compare device, outer
 *****************************************************************************/

int adv_inmap_compare(struct adv_inmap *inmap,int vid,int pid,const char *name,int namec,int devid) {
  if (!inmap) return 0;
  if (!name||(namec<0)) namec=0;
  int score=0;
  
  // (bustype,vendor,product) must match if specified. One point for bus, ten each for vendor and product
  //if (inmap->bustype) {
  //  if (inmap->bustype!=linput_device_get_bustype(devid)) return 0;
  //  score++;
  //}
  if (inmap->vendor) {
    if (inmap->vendor!=vid) return 0;
    score+=10;
  }
  if (inmap->product) {
    if (inmap->product!=pid) return 0;
    score+=10;
  }
  
  // Name must match always. One point for wildcard matches, ten points for verbatim matches.
  int add=adv_inmap_compare_name(inmap->name,inmap->namec,name,namec);
  if (add<1) return 0;
  score+=(add>1)?10:1;
  
  return score;
}

/* evaluate header
 *****************************************************************************/
 
int adv_inmap_eval_header(struct adv_inmap *inmap,const char *src,int srcc,const char *refname,int lineno) {
  if (!inmap||((srcc>0)&&!src)) return -1;
  if (srcc<0) { srcc=0; if (src) while (src[srcc]) srcc++; }
  int srcp=0,havename=0;
  while (srcp<srcc) {
    if ((unsigned char)src[srcp]<=0x20) { srcp++; continue; }
    
    if (src[srcp]=='"') {
      if (havename) {
        if (refname) fprintf(stderr,"%s:%d: multiple name patterns in device header\n",refname,lineno);
        return -1;
      }
      havename=1;
      srcp++;
      const char *name=src+srcp;
      int namec=0;
      while (1) {
        if (srcp>=srcc) {
          if (refname) fprintf(stderr,"%s:%d: unclosed string\n",refname,lineno);
          return -1;
        }
        if (src[srcp++]=='"') break;
        namec++;
      }
      if (adv_inmap_set_name(inmap,name,namec)<0) return -1;
      continue;
    }
    
    const char *k=src+srcp;
    int kc=0;
    while ((srcp<srcc)&&((unsigned char)src[srcp]>0x20)&&(src[srcp]!='=')) { srcp++; kc++; }
    const char *v=0; int vc=0;
    if ((srcp<srcc)&(src[srcp]=='=')) {
      srcp++;
      v=src+srcp;
      while ((srcp<srcc)&&((unsigned char)src[srcp]>0x20)) { srcp++; vc++; }
    }
    if ((kc==3)&&!memcmp(k,"bus",3)) {
      if (inmap->bustype) {
        if (refname) fprintf(stderr,"%s:%d: multiple 'bus' keys in device header\n",refname,lineno);
        return -1;
      }
      int bus=adv_bus_eval(v,vc);
      if (bus<0) {
        if (refname) fprintf(stderr,"%s:%d: '%.*s' does not name an input bus (see 'BUS_*' in <linux/input.h>)\n",refname,lineno,vc,v);
        return -1;
      }
      inmap->bustype=bus;
    } else if ((kc==6)&&!memcmp(k,"vendor",6)) {
      if (inmap->vendor) {
        if (refname) fprintf(stderr,"%s:%d: multiple 'vendor' keys in device header\n",refname,lineno);
        return -1;
      }
      int vendor=adv_uint_eval(v,vc);
      if (vendor<0) {
        if (refname) fprintf(stderr,"%s:%d: expected integer for vendor ID, found '%.*s'\n",refname,lineno,vc,v);
        return -1;
      }
      inmap->vendor=vendor;
    } else if ((kc==7)&&!memcmp(k,"product",7)) {
      if (inmap->product) {
        if (refname) fprintf(stderr,"%s:%d: multiple 'product' keys in device header\n",refname,lineno);
        return -1;
      }
      int product=adv_uint_eval(v,vc);
      if (product<0) {
        if (refname) fprintf(stderr,"%s:%d: expected integer for product ID, found '%.*s'\n",refname,lineno,vc,v);
        return -1;
      }
      inmap->product=product;
    } else {
      if (refname) fprintf(stderr,"%s:%d: unexpected key '%.*s' in device header\n",refname,lineno,kc,k);
      return -1;
    }
  }
  if (!havename) return adv_inmap_set_name(inmap,"*",1);
  return 0;
}

/* evaluate single mapping
 *****************************************************************************/
 
int adv_inmap_eval_argument(struct adv_inmap *inmap,const char *src,int srcc,const char *refname,int lineno) {
  if (!inmap||((srcc>0)&&!src)) return -1;
  if (srcc<0) { srcc=0; if (src) while (src[srcc]) srcc++; }
  const char *wordv[3]={0};
  int lenv[3]={0};
  int wordc=0,srcp=0;
  while (1) {
    while ((srcp<srcc)&&((unsigned char)src[srcp]<=0x20)) srcp++;
    if (srcp>=srcc) break;
    if (wordc>=3) {
      if (refname) fprintf(stderr,"%s:%d: unexpected extra tokens\n",refname,lineno);
      return -1;
    }
    wordv[wordc]=src+srcp;
    while ((srcp<srcc)&&((unsigned char)src[srcp]>0x20)) { srcp++; lenv[wordc]++; }
    wordc++;
  }
  if (!wordc) return 0; // ok, whatever
  
  if (wordc==3) { // must be absolute axis
    int code=adv_abscode_eval(wordv[0],lenv[0]);
    if ((code<0)||(code>ABS_MAX)) {
      if (refname) fprintf(stderr,"%s:%d: '%.*s' does not name an absolute axis (see 'ABS_*' in <linux/input.h>)\n",refname,lineno,lenv[0],wordv[0]);
      return -1;
    }
    int btnlo=adv_btnid_eval(wordv[1],lenv[1]);
    if (btnlo<0) {
      if (refname) fprintf(stderr,"%s:%d: '%.*s' does not name a player button (UP,DOWN,LEFT,RIGHT,ACTION,SWITCH,DETACH,FOLLOWME)\n",refname,lineno,lenv[1],wordv[1]);
      return -1;
    }
    int btnhi=adv_btnid_eval(wordv[2],lenv[2]);
    if (btnhi<0) {
      if (refname) fprintf(stderr,"%s:%d: '%.*s' does not name a player button (UP,DOWN,LEFT,RIGHT,ACTION,SWITCH,DETACH,FOLLOWME)\n",refname,lineno,lenv[2],wordv[2]);
      return -1;
    }
    int p=adv_inmap_absmap_search(inmap,code);
    if (p>=0) {
      if (refname) fprintf(stderr,"%s:%d: axis '%.*s' is already assigned\n",refname,lineno,lenv[0],wordv[0]);
      return -1;
    }
    p=-p-1;
    if (adv_inmap_absmap_insert(inmap,p,code)<0) return -1;
    inmap->absmapv[p].btnlo=btnlo;
    inmap->absmapv[p].btnhi=btnhi;
    
  } else if (wordc==2) { // axis or button
    int btnid=adv_btnid_eval(wordv[1],lenv[1]); // get this out of the way first since both branches will need it
    if (btnid<0) {
      if ((btnid=adv_useraction_eval(wordv[1],lenv[1]))>=0) {
        btnid|=ADV_KEYMAP_USERACTION;
      } else {
        if (refname) fprintf(stderr,"%s:%d: '%.*s' does not name a player button (UP,DOWN,LEFT,RIGHT,ACTION,SWITCH,DETACH,FOLLOWME)\n",refname,lineno,lenv[1],wordv[1]);
        return -1;
      }
    }
    int code=adv_keycode_eval(wordv[0],lenv[0]);
    if (
      ((code>=0)&&(code<=KEY_MAX))||
      ((code>=0x00070000)&&(code<0x00080000))
    ) { // key
      int p=adv_inmap_keymap_search(inmap,code);
      if (p>=0) {
        if (refname) fprintf(stderr,"%s:%d: button '%.*s' is already assigned\n",refname,lineno,lenv[0],wordv[0]);
        return -1;
      }
      p=-p-1;
      if (adv_inmap_keymap_insert(inmap,p,code)<0) return -1;
      inmap->keymapv[p].btn=btnid;
    
    } else { // axis
      code=adv_abscode_eval(wordv[0],lenv[0]);
      if ((code>=0)&&(code<=ABS_MAX)) { // axis
        int p=adv_inmap_absmap_search(inmap,code);
        if (p>=0) {
          if (refname) fprintf(stderr,"%s:%d: axis '%.*s' is already assigned\n",refname,lineno,lenv[0],wordv[0]);
          return -1;
        }
        p=-p-1;
        if (adv_inmap_absmap_insert(inmap,p,code)<0) return -1;
        inmap->absmapv[p].btnlo=0;
        inmap->absmapv[p].btnhi=btnid;
        
      } else {
        if (refname) fprintf(stderr,"%s:%d: '%.*s' does not name a key or axis (see 'ABS_*', 'KEY_*', 'BTN_*', in <linux/input.h>)\n",refname,lineno,lenv[0],wordv[0]);
        return -1;
      }
    }
  } else {
    if (refname) fprintf(stderr,"%s:%d: expected 'AXIS LOWBUTTON HIGHBUTTON' or 'KEY BUTTON'\n",refname,lineno);
    return -1;
  }
  return 0;
}
