#include "adv.h"
#include "adv_map.h"
#include "res/adv_res.h"
#include "video/adv_video.h"
#include "game/adv_game_progress.h"

/* new
 *****************************************************************************/

int adv_map_new(struct adv_map **dst) {
  if (!dst) return -1;
  if (!(*dst=calloc(1,sizeof(struct adv_map)))) return -1;
  return 0;
}

/* del
 *****************************************************************************/

void adv_map_del(struct adv_map *map) {
  if (!map) return;
  if (map->spawnv) free(map->spawnv);
  if (map->doorv) free(map->doorv);
  if (map->switchv) free(map->switchv);
  free(map);
}

/* decode raw data
 *****************************************************************************/

struct adv_map_macro {
  char text[2];
  unsigned char bg,fg;
};

static int _rduint(const char *src,int srcc) {
  if (srcc<1) return -1;
  int dst=0,srcp=0;
  while (srcp<srcc) {
    int digit=src[srcp++]-'0';
    if ((digit<0)||(digit>9)) return -1;
    if (dst>INT_MAX/10) return -1; dst*=10;
    if (dst>INT_MAX-digit) return -1; dst+=digit;
  }
  return dst;
}

static int _rdhex(char src) {
  if ((src>='0')&&(src<='9')) return src-'0';
  if ((src>='a')&&(src<='f')) return src-'a'+10;
  if ((src>='A')&&(src<='F')) return src-'A'+10;
  return -1;
}

static int _macrosearch(struct adv_map_macro *macrov,int macroc,const char *text) {
  int lo=0,hi=macroc;
  while (lo<hi) {
    int ck=(lo+hi)>>1;
         if (text[0]<macrov[ck].text[0]) hi=ck;
    else if (text[0]>macrov[ck].text[0]) lo=ck+1;
    else if (text[1]<macrov[ck].text[1]) hi=ck;
    else if (text[1]>macrov[ck].text[1]) lo=ck+1;
    else return ck;
  }
  return -lo-1;
}

static int _insmacro(struct adv_map_macro *macrov,int macroc,const char *src,int srcc) {
  if (srcc!=10) return -1;
  if (src[0]!='\'') return -1;
  if (src[3]!='\'') return -1;
  if (src[4]!=' ') return -1;
  if (src[7]!=' ') return -1;
  int bhi=_rdhex(src[5]);
  int blo=_rdhex(src[6]);
  int fhi=_rdhex(src[8]);
  int flo=_rdhex(src[9]);
  if ((bhi<0)||(blo<0)||(fhi<0)||(flo<0)) return -1;
  int p=_macrosearch(macrov,macroc,src+1);
  if (p>=0) return -2;
  p=-p-1;
  memmove(macrov+p+1,macrov+p,sizeof(struct adv_map_macro)*(macroc-p));
  memcpy(macrov[p].text,src+1,2);
  macrov[p].bg=(bhi<<4)|blo;
  macrov[p].fg=(fhi<<4)|flo;
  return 0;
}

static int _addsprite(struct adv_map *map,const char *src,int srcc) {
  int srcp=0,sprdefid,col,row;
  const char *sub; int subc;
  while ((srcp<srcc)&&((unsigned char)src[srcp]<=0x20)) srcp++;
  sub=src+srcp; subc=0;
  while ((srcp<srcc)&&((unsigned char)src[srcp]>0x20)) { srcp++; subc++; }
  if ((sprdefid=_rduint(sub,subc))<0) return -1;
  while ((srcp<srcc)&&((unsigned char)src[srcp]<=0x20)) srcp++;
  sub=src+srcp; subc=0;
  while ((srcp<srcc)&&((unsigned char)src[srcp]>0x20)) { srcp++; subc++; }
  if ((col=_rduint(sub,subc))<0) return -1;
  while ((srcp<srcc)&&((unsigned char)src[srcp]<=0x20)) srcp++;
  sub=src+srcp; subc=0;
  while ((srcp<srcc)&&((unsigned char)src[srcp]>0x20)) { srcp++; subc++; }
  if ((row=_rduint(sub,subc))<0) return -1;
  while ((srcp<srcc)&&((unsigned char)src[srcp]<=0x20)) srcp++;
  if (srcp<srcc) return -1;
  if (map->spawnc>=map->spawna) {
    int na=map->spawna+8;
    if (na>INT_MAX/sizeof(struct adv_spawn)) return -1;
    void *nv=realloc(map->spawnv,sizeof(struct adv_spawn)*na);
    if (!nv) return -1;
    map->spawnv=nv;
    map->spawna=na;
  }
  struct adv_spawn *spawn=map->spawnv+map->spawnc++;
  memset(spawn,0,sizeof(struct adv_spawn));
  spawn->sprdefid=sprdefid;
  spawn->col=col;
  spawn->row=row;
  return 0;
}

static int _adddoor(struct adv_map *map,const char *src,int srcc) {
  int srcp=0,subc;
  const char *sub;
  if (map->doorc>=map->doora) {
    int na=map->doora+4;
    if (na>INT_MAX/sizeof(struct adv_door)) return -1;
    void *nv=realloc(map->doorv,sizeof(struct adv_door)*na);
    if (!nv) return -1;
    map->doorv=nv;
    map->doora=na;
  }
  struct adv_door *door=map->doorv+map->doorc++;
  memset(door,0,sizeof(struct adv_door));
  while ((srcp<srcc)&&((unsigned char)src[srcp]<=0x20)) srcp++;
  sub=src+srcp; subc=0; while ((srcp<srcc)&&((unsigned char)src[srcp]>0x20)) { srcp++; subc++; }
  if ((door->srccol=_rduint(sub,subc))<0) return -1;
  while ((srcp<srcc)&&((unsigned char)src[srcp]<=0x20)) srcp++;
  sub=src+srcp; subc=0; while ((srcp<srcc)&&((unsigned char)src[srcp]>0x20)) { srcp++; subc++; }
  if ((door->srcrow=_rduint(sub,subc))<0) return -1;
  while ((srcp<srcc)&&((unsigned char)src[srcp]<=0x20)) srcp++;
  sub=src+srcp; subc=0; while ((srcp<srcc)&&((unsigned char)src[srcp]>0x20)) { srcp++; subc++; }
  if ((door->dstmapid=_rduint(sub,subc))<0) return -1;
  while ((srcp<srcc)&&((unsigned char)src[srcp]<=0x20)) srcp++;
  sub=src+srcp; subc=0; while ((srcp<srcc)&&((unsigned char)src[srcp]>0x20)) { srcp++; subc++; }
  if ((door->dstcol=_rduint(sub,subc))<0) return -1;
  while ((srcp<srcc)&&((unsigned char)src[srcp]<=0x20)) srcp++;
  sub=src+srcp; subc=0; while ((srcp<srcc)&&((unsigned char)src[srcp]>0x20)) { srcp++; subc++; }
  if ((door->dstrow=_rduint(sub,subc))<0) return -1;
  while ((srcp<srcc)&&((unsigned char)src[srcp]<=0x20)) srcp++;
  if (srcp<srcc) return -1;
  return 0;
}

static int _addswitch(struct adv_map *map,const char *src,int srcc) {
  int srcp=0,subc;
  const char *sub;
  if (map->switchc>=map->switcha) {
    int na=map->switcha+4;
    if (na>INT_MAX/sizeof(struct adv_switch)) return -1;
    void *nv=realloc(map->switchv,sizeof(struct adv_switch)*na);
    if (!nv) return -1;
    map->switchv=nv;
    map->switcha=na;
  }
  struct adv_switch *sw=map->switchv+map->switchc++;
  memset(sw,0,sizeof(struct adv_switch));
  while ((srcp<srcc)&&((unsigned char)src[srcp]<=0x20)) srcp++;
  sub=src+srcp; subc=0; while ((srcp<srcc)&&((unsigned char)src[srcp]>0x20)) { srcp++; subc++; }
  if ((sw->col=_rduint(sub,subc))<0) return -1;
  while ((srcp<srcc)&&((unsigned char)src[srcp]<=0x20)) srcp++;
  sub=src+srcp; subc=0; while ((srcp<srcc)&&((unsigned char)src[srcp]>0x20)) { srcp++; subc++; }
  if ((sw->row=_rduint(sub,subc))<0) return -1;
  while ((srcp<srcc)&&((unsigned char)src[srcp]<=0x20)) srcp++;
  if (srcp>srcc-5) return -1;
  int hi,lo;
  if ((hi=_rdhex(src[srcp++]))<0) return -1;
  if ((lo=_rdhex(src[srcp++]))<0) return -1;
  sw->tileoff=(hi<<4)|lo;
  srcp++;
  if ((hi=_rdhex(src[srcp++]))<0) return -1;
  if ((lo=_rdhex(src[srcp++]))<0) return -1;
  sw->tileon=(hi<<4)|lo;
  while ((srcp<srcc)&&((unsigned char)src[srcp]<=0x20)) srcp++;
  sub=src+srcp; subc=0; while ((srcp<srcc)&&((unsigned char)src[srcp]>0x20)) { srcp++; subc++; }
  if ((sw->globalid=_rduint(sub,subc))<0) return -1;
  while ((srcp<srcc)&&((unsigned char)src[srcp]<=0x20)) srcp++;
  sub=src+srcp; subc=0; while ((srcp<srcc)&&((unsigned char)src[srcp]>0x20)) { srcp++; subc++; }
  if ((sw->method=_rduint(sub,subc))<0) return -1;
  while ((srcp<srcc)&&((unsigned char)src[srcp]<=0x20)) srcp++;
  if (srcp<srcc) return -1;
  return 0;
}

/* decode main (text; XXX)
 *****************************************************************************/
 
int adv_map_decode_text(struct adv_map *map,const void *src,int srcc,const char *refname) {
  int err;
  if (!map||(srcc<0)||(srcc&&!src)) return -1;
  if (!refname) refname="(unknown)";
  int bodyc=ADV_MAP_H*(ADV_MAP_W*2+1);
  if (srcc<bodyc) {
    fprintf(stderr,"%s: File is too small to be a map.\n",refname);
    return -1;
  }
  srcc-=bodyc;
  const unsigned char *SRC=src;
  const unsigned char *BODY=SRC+srcc;
  #define macroa 256 // arbitrary macro limit -- I'd rather not allocate a fresh buffer for every decode.
  struct adv_map_macro macrov[macroa];
  int macroc=0;

  /* Read header and macros. */
  int srcp=0,lineno=1;
  while (srcp<srcc) {
    if (SRC[srcp]==0x0a) lineno++;
    if (SRC[srcp]<=0x20) { srcp++; continue; }
    const unsigned char *line=SRC+srcp;
    int linec=0,cmt=0;
    while ((srcp<srcc)&&(SRC[srcp]!=0x0a)) {
      if (SRC[srcp]=='#') cmt=1;
      else if (!cmt) {
        if ((SRC[srcp]<0x20)||(SRC[srcp]>0x7e)) { // if these show up in a comment, ignore it
          fprintf(stderr,"%s:%d: illegal byte 0x%02x in map file\n",refname,lineno,SRC[srcp]);
          return -1;
        }
        linec++;
      }
      srcp++;
    }
    while (linec&&(line[linec-1]<=0x20)) linec--;
    if (!linec) continue;
    int kwc=0;
    while ((kwc<linec)&&(line[kwc]>0x20)) kwc++;
    int p=kwc; while ((p<linec)&&(line[p]<=0x20)) p++;
    const unsigned char *arg=line+p;
    int argc=linec-p;

    #define KWINT(tag) if ((kwc==sizeof(#tag)-1)&&!memcmp(line,#tag,kwc)) { \
      if ((map->tag=_rduint(arg,argc))<0) { \
        fprintf(stderr,"%s:%d: failed to parse '%.*s' as unsigned integer\n",refname,lineno,argc,arg); \
        return -1; \
      } \
    }
    
    if ((kwc==4)&&!memcmp(line,"tile",4)) {
      if (macroc>=macroa) {
        fprintf(stderr,"%s:%d: too many tile macros (limit %d)\n",refname,lineno,macroa);
        return -1;
      }
      if ((err=_insmacro(macrov,macroc,arg,argc))<0) {
        if (err==-2) fprintf(stderr,"%s:%d: duplicate tile macro '%.2s'\n",refname,lineno,arg+1);
        else fprintf(stderr,"%s:%d: malformed tile macro\n",refname,lineno);
        return -1;
      }
      macroc++;
    } else if ((kwc==6)&&!memcmp(line,"sprite",6)) {
      if (_addsprite(map,arg,argc)<0) {
        fprintf(stderr,"%s:%d: bad 'sprite' line, expected 'sprite ID COLUMN ROW'\n",refname,lineno);
        return -1;
      }
    } else if ((kwc==4)&&!memcmp(line,"door",4)) {
      if (_adddoor(map,arg,argc)<0) {
        fprintf(stderr,"%s:%d: bad 'door' line, expected 'door SRCCOL SRCROW MAPID DSTCOL DSTROW'\n",refname,lineno);
        return -1;
      }
    } else if ((kwc==6)&&!memcmp(line,"switch",6)) {
      if (_addswitch(map,arg,argc)<0) {
        fprintf(stderr,"%s:%d: bad 'switch' line, expected 'switch COL ROW TILEOFF TILEON GLOBAL HOLDVALUE'\n",refname,lineno);
        return -1;
      }
    }
    else KWINT(bg)
    //else KWINT(fg)
    else if ((kwc==2)&&!memcmp(line,"fg",2)) ; // ignore foreground tilesheet
    else KWINT(spr)
    else KWINT(west)
    else KWINT(east)
    else KWINT(north)
    else KWINT(south)
    else KWINT(lights_out)
    else KWINT(song)
    else {
      fprintf(stderr,"%s:%d: undefined map keyword '%.*s'\n",refname,lineno,kwc,line);
      return -1;
    }
    #undef KWINT
  }

  /* Read tile data. */
  unsigned char *celldst=map->cellv;
  int row; for (row=0;row<ADV_MAP_H;row++) {
    if (BODY[ADV_MAP_W<<1]!=0x0a) {
      fprintf(stderr,"%s: malformed map data at row %d\n",refname,row);
      return -1;
    }
    const unsigned char *srci=BODY;
    unsigned char *dsti=celldst;
    int col; for (col=0;col<ADV_MAP_W;col++) {
      int macrop=_macrosearch(macrov,macroc,srci);
      if (macrop<0) {
        if ((srci[0]<0x20)||(srci[0]>0x7e)||(srci[1]<0x20)||(srci[1]>0x7e)) {
          fprintf(stderr,"%s: undefined tile macro '\\%02x\\%02x' at row %d, column %d\n",refname,srci[0],srci[1],row,col);
        } else {
          fprintf(stderr,"%s: undefined tile macro '%.2s' at row %d, column %d\n",refname,srci,row,col);
        }
        return -1;
      }
      *dsti++=macrov[macrop].bg;
      //*dsti++=macrov[macrop].fg;
      srci+=2;
    }
    celldst+=ADV_MAP_W;
    BODY+=(ADV_MAP_W<<1)+1;
  }
  
  #undef macroa
  return 0;
}

/* decode file (choose format)
 *****************************************************************************/

int adv_map_decode_file(struct adv_map *map,const char *path) {
  if (!map||!path) return -1;
  char *src=0;
  int srcc=adv_file_read(&src,path);
  if (srcc<0) return srcc;
  if (!src) return 0;
  int err;
  if ((srcc>=4)&&!memcmp(src,"\0MP\xff",4)) err=adv_map_decode_binary(map,src,srcc,path);
  else err=adv_map_decode_text(map,src,srcc,path);
  free(src);
  return err;
}

/* decode binary file
 *****************************************************************************/
 
int adv_map_decode_binary(struct adv_map *map,const void *src,int srcc,const char *refname) {
  if (!map||((srcc>0)&&!src)) return -1;
  if (!refname) refname="(unknown)";
  if ((srcc<4)||memcmp(src,"\0MP\xff",4)) { fprintf(stderr,"%s: map signature mismatch\n",refname); return -1; }
  const unsigned char *SRC=src;
  int srcp=4;
  
  /* Validate dimensions and yoink cells. */
  if (srcp>srcc-2) { fprintf(stderr,"%s: unexpected end of file\n",refname); return -1; }
  if ((SRC[srcp]!=ADV_MAP_W)||(SRC[srcp+1]!=ADV_MAP_H)) {
    fprintf(stderr,"%s: map file is %dx%d cells; we require %dx%d\n",refname,SRC[srcp],SRC[srcp+1],ADV_MAP_W,ADV_MAP_H);
    return -1;
  }
  srcp+=2;
  if (srcp>srcc-ADV_MAP_W*ADV_MAP_H) { fprintf(stderr,"%s: unexpected end of file\n",refname); return -1; }
  memcpy(map->cellv,SRC+srcp,ADV_MAP_W*ADV_MAP_H);
  srcp+=ADV_MAP_W*ADV_MAP_H;
  
  /* Read loose fields. */
  while (srcp<srcc) {
    int refp=srcp;
    switch (SRC[srcp++]) {
      case 0x00: goto _done_;
      
      // Don't you love cpp? I love cpp.
      #define GROW(tag) if (map->tag##c>=map->tag##a) { \
          int na=map->tag##a+8; \
          if (na>INT_MAX/sizeof(struct adv_##tag)) return -1; \
          void *nv=realloc(map->tag##v,sizeof(struct adv_##tag)*na); \
          if (!nv) return -1; \
          map->tag##v=nv; \
          map->tag##a=na; \
        }
 
      #define REQ(c,fld) if (srcp>srcc-c) { fprintf(stderr,"%s:%#x/%#x: unexpected end of file reading field '%s'\n",refname,refp,srcc,#fld); return -1; }
      #define U1(fld) REQ(1,fld) map->fld=SRC[srcp++];
      #define U2(fld) REQ(2,fld) map->fld=(SRC[srcp]<<8)|SRC[srcp+1]; srcp+=2;
 
      case 0x01: U2(bg) break;
      case 0x02: U2(spr) break;
      case 0x03: U2(north) break;
      case 0x04: U2(south) break;
      case 0x05: U2(east) break;
      case 0x06: U2(west) break;
      case 0x07: U2(song) break;
      case 0x08: U1(lights_out) break;
      
      case 0x09: {
          GROW(spawn)
          U1(spawnv[map->spawnc].col)
          U1(spawnv[map->spawnc].row)
          U2(spawnv[map->spawnc].sprdefid)
          map->spawnc++;
        } break;
      
      case 0x0a: {
          GROW(door)
          U1(doorv[map->doorc].srccol)
          U1(doorv[map->doorc].srcrow)
          U2(doorv[map->doorc].dstmapid)
          U1(doorv[map->doorc].dstcol)
          U1(doorv[map->doorc].dstrow)
          map->doorc++;
        } break;
      
      case 0x0b: {
          GROW(switch)
          U1(switchv[map->switchc].col)
          U1(switchv[map->switchc].row)
          U1(switchv[map->switchc].tileoff)
          U1(switchv[map->switchc].tileon)
          U1(switchv[map->switchc].globalid)
          U1(switchv[map->switchc].method)
          map->switchc++;
        } break;
      
      #undef U2
      #undef U1
      #undef REQ
      #undef GROW
      default: fprintf(stderr,"%s:%#x/%#x: unexpected field 0x%02x\n",refname,refp,srcc,SRC[refp]); return -1;
    }
  }
 _done_:
  
  // Hey, that was easy. :)
  return 0;
}

/* encode to binary format
 *****************************************************************************/
 
int adv_map_encode_binary(void *dst,int dsta,const struct adv_map *map) {
  if (((dsta>0)&&!dst)||!map) return -1;
  if (dsta<0) dsta=0;
  unsigned char *DST=dst;
  int dstc=0;
  
  if (dstc<=dsta-4) memcpy(DST+dstc,"\0MP\xff",4); dstc+=4;
  if (dstc<dsta) DST[dstc]=ADV_MAP_W; dstc++;
  if (dstc<dsta) DST[dstc]=ADV_MAP_H; dstc++;
  if (dstc<=dsta-ADV_MAP_W*ADV_MAP_H) memcpy(DST+dstc,map->cellv,ADV_MAP_W*ADV_MAP_H); dstc+=ADV_MAP_W*ADV_MAP_H;
  
  #define OPT2(tag,key) if (map->tag) { if (dstc<=dsta-3) { DST[dstc++]=key; DST[dstc++]=map->tag>>8; DST[dstc++]=map->tag; } else dstc+=3; }
  #define OPT1(tag,key) if (map->tag) { if (dstc<=dsta-2) { DST[dstc++]=key; DST[dstc++]=map->tag; } else dstc+=2; }
  OPT2(bg,0x01)
  OPT2(spr,0x02)
  OPT2(north,0x03)
  OPT2(south,0x04)
  OPT2(east,0x05)
  OPT2(west,0x06)
  OPT2(song,0x07)
  OPT1(lights_out,0x08)
  #undef OPT2
  #undef OPT1
  
  int i;
  for (i=0;i<map->spawnc;i++) {
    if (dstc<=dsta-5) {
      DST[dstc++]=0x09;
      DST[dstc++]=map->spawnv[i].col;
      DST[dstc++]=map->spawnv[i].row;
      DST[dstc++]=map->spawnv[i].sprdefid>>8;
      DST[dstc++]=map->spawnv[i].sprdefid;
    } else dstc+=5;
  }
  for (i=0;i<map->doorc;i++) {
    if (dstc<=dsta-7) {
      DST[dstc++]=0x0a;
      DST[dstc++]=map->doorv[i].srccol;
      DST[dstc++]=map->doorv[i].srcrow;
      DST[dstc++]=map->doorv[i].dstmapid>>8;
      DST[dstc++]=map->doorv[i].dstmapid;
      DST[dstc++]=map->doorv[i].dstcol;
      DST[dstc++]=map->doorv[i].dstrow;
    } else dstc+=7;
  }
  for (i=0;i<map->switchc;i++) {
    if (dstc<=dsta-7) {
      DST[dstc++]=0x0b;
      DST[dstc++]=map->switchv[i].col;
      DST[dstc++]=map->switchv[i].row;
      DST[dstc++]=map->switchv[i].tileoff;
      DST[dstc++]=map->switchv[i].tileon;
      DST[dstc++]=map->switchv[i].globalid;
      DST[dstc++]=map->switchv[i].method;
    } else dstc+=7;
  }
  
  return dstc;
}

/* test tile physics
 *****************************************************************************/
 
int adv_map_cell_is_solid(struct adv_map *map,int col,int row,int check_positive,int check_negative) {
  if (!map) return 0;
  if (!map->propv) return 0;
  if (!check_positive&&!check_negative) return 0;
  if (col<0) col=0; else if (col>=ADV_MAP_W) col=ADV_MAP_W-1;
  if (row<0) row=0; else if (row>=ADV_MAP_H) row=ADV_MAP_H-1;
  unsigned char bgtileid=map->cellv[row*ADV_MAP_W+col];
  if (check_positive&&(map->propv[bgtileid]&ADV_TILEPROP_LATCH)) return 2;
  if (check_positive&&(map->propv[bgtileid]&ADV_TILEPROP_SOLID)) return 1;
  if (check_negative&&(map->propv[bgtileid]&ADV_TILEPROP_HOLE)) return 1;
  return 0;
}

int adv_map_cell_is_heal(struct adv_map *map,int col,int row) {
  if (!map) return 0;
  if (!map->propv) return 0;
  if (col<0) col=0; else if (col>=ADV_MAP_W) col=ADV_MAP_W-1;
  if (row<0) row=0; else if (row>=ADV_MAP_H) row=ADV_MAP_H-1;
  unsigned char bgtileid=map->cellv[row*ADV_MAP_W+col];
  if (map->propv[bgtileid]&ADV_TILEPROP_HEAL) return 1;
  return 0;
}

/* check switches, adjust my tiles to suit
 *****************************************************************************/
 
int adv_map_update_switches(struct adv_map *map,int draw) {
  if (!map) return -1;
  int i; for (i=0;i<map->switchc;i++) {
    struct adv_switch *sw=map->switchv+i;
    if ((sw->col<0)||(sw->row<0)||(sw->col>=ADV_MAP_W)||(sw->row>=ADV_MAP_H)) continue;
    int p=sw->row*ADV_MAP_W+sw->col;
    if (sw->method==ADV_SWITCH_METHOD_THREEWAY) { // everything else is 2-state
      int v=adv_global_get(sw->globalid);
      int expect;
      if (v<=0) expect=sw->tileoff;
      else if (v==1) expect=sw->tileon;
      else expect=sw->tileon+1;
      if (map->cellv[p]==expect) continue;
      map->cellv[p]=expect;
    } else {
      int on_now=(map->cellv[p]==sw->tileon)?1:0;
      int on_should=adv_global_get(sw->globalid)?1:0;
      if (on_now==on_should) continue;
      if (on_should) map->cellv[p]=sw->tileon;
      else map->cellv[p]=sw->tileoff;
    }
    if (draw) {
      void *bg=0;
      adv_res_get_background_palette(&bg);
      if (adv_video_draw_bg_sub(map->cellv,bg,sw->col,sw->row,1,1)<0) return -1;
    }
  }
  return 0;
}
