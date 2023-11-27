#include "adv_res_internal.h"
#include "png/png.h"

/* globals
 *****************************************************************************/

struct adv_map *adv_map=0;

struct adv_resmgr adv_resmgr={0};

static void adv_res_wipe_globals() {
  adv_map=0;
  memset(&adv_resmgr,0,sizeof(struct adv_resmgr));
}

/* Check one directory for our 'data' tree.
 * Return zero if not found, >0 if found, or <0 for real errors.
 *****************************************************************************/

static int adv_res_check_data_path(const char *path,int pathc) {
  if (!path) return 0;
  if (pathc<0) { pathc=0; while (path[pathc]) pathc++; }
  char subpath[1024];
  if (pathc>=sizeof(subpath)-5) return 0;
  memcpy(subpath,path,pathc);
  memcpy(subpath+pathc,"/data",6); // copy terminator
  struct stat st={0};
  if (stat(subpath,&st)<0) return 0;
  if (!S_ISDIR(st.st_mode)) return 0;
  if (adv_resmgr.root) free(adv_resmgr.root);
  if (!(adv_resmgr.root=malloc(pathc+6))) return -1;
  memcpy(adv_resmgr.root,subpath,pathc+6);
  adv_resmgr.rootc=pathc+5;
  return 1;
}

/* locate data directory, outer
 *****************************************************************************/

static int adv_res_locate_data(const char *programpath,int argc,char **argv) {
  int i,err;
  
  // If we got an argument "--data=PATH", that's the final answer.
  for (i=1;i<argc;i++) {
    if (memcmp(argv[i],"--data=",7)) continue;
    if (adv_resmgr.root) free(adv_resmgr.root);
    if (!(adv_resmgr.root=strdup(argv[i]+7))) return -1;
    adv_resmgr.rootc=0;
    while (adv_resmgr.root[adv_resmgr.rootc]) adv_resmgr.rootc++;
    return 0;
  }

  // If there is a slash in argv[0], look in the directory containing this program.
  // This will not look in the root directory.
  int programdirc=0,programpathc=0;
  if (programpath) {
    for (;programpath[programpathc];programpathc++) {
      if (programpath[programpathc]=='/') programdirc=programpathc;
    }
  }
  if (programdirc) {
    if (err=adv_res_check_data_path(programpath,programdirc)) return err;

  // If there was no slash in argv[0], we are probably in $PATH. Check it out.
  // Incidentally, it is not advisable to put the data directly in $PATH.
  // A smart user would drop it in a 'share' directory.
  // But smart users probably don't play my games. :P
  } else {
    const char *PATH=getenv("PATH");
    if (PATH) {
      char subpath[1024];
      struct stat st;
      int PATHp=0; while (PATH[PATHp]) {
        const char *pfx=PATH+PATHp;
        int pfxc=0;
        while (PATH[PATHp]&&(PATH[PATHp]!=':')) { pfxc++; PATHp++; }
        if (pfxc<sizeof(subpath)-programpathc-1) {
          memcpy(subpath,pfx,pfxc);
          subpath[pfxc]='/';
          memcpy(subpath+pfxc+1,programpath,programpathc);
          subpath[pfxc+1+programpathc]=0;
          if (stat(subpath,&st)>=0) {
            if (err=adv_res_check_data_path(pfx,pfxc)) return err;
          }
        }
        if (PATH[PATHp]==':') PATHp++;
      }
    }
  }

  // Check a few likely spots.
  if (err=adv_res_check_data_path("/usr/local/share/ttaq",-1)) return err;
  if (err=adv_res_check_data_path("/usr/share/ttaq",-1)) return err;

  // When running from the source distribution, use the data directory in the source tree.
  if (err=adv_res_check_data_path("src",-1)) return err;

  // Give up!
  fprintf(stderr,"Failed to locate data set!\n");
  return -1;
}

/* load tilesheet properties, single
 *****************************************************************************/
 
static inline int _hexeval(char src) {
  if ((src>='0')&&(src<='9')) return src-'0';
  if ((src>='a')&&(src<='f')) return src-'a'+10;
  if ((src>='A')&&(src<='F')) return src-'A'+10;
  return -1;
}

static inline int _adv_tsprop_mask_from_token(const char *src,int srcc) {
  if ((srcc==4)&&!memcmp(src,"hole",4)) return ADV_TILEPROP_HOLE;
  if ((srcc==5)&&!memcmp(src,"solid",5)) return ADV_TILEPROP_SOLID;
  if ((srcc==5)&&!memcmp(src,"latch",5)) return ADV_TILEPROP_LATCH|ADV_TILEPROP_SOLID;
  if ((srcc==10)&&!memcmp(src,"neighbor15",10)) return ADV_TILEPROP_NEIGHBOR15;
  if ((srcc==4)&&!memcmp(src,"heal",4)) return ADV_TILEPROP_HEAL;
  return -1;
}
 
static int adv_res_load_tsprop(unsigned char *dst,const char *src,int srcc,const char *path) {
  if (!dst||((srcc>0)&&!src)) return -1;
  if (srcc<0) { srcc=0; if (src) while (src[srcc]) srcc++; }
  if (!path) path="(unknown)";
  int srcp=0,lineno=1; while (srcp<srcc) {
    if (src[srcp]==0x0a) { lineno++; srcp++; continue; }
    if ((unsigned char)src[srcp]<=0x20) { srcp++; continue; }
    const char *line=src+srcp;
    int linec=0,cmt=0;
    while ((srcp<srcc)&&(src[srcp]!=0x0a)) {
      if (src[srcp]=='#') cmt=1;
      else if (!cmt) linec++;
      srcp++;
    }
    while (linec&&((unsigned char)line[linec-1]<=0x20)) linec--;
    if (!linec) continue;
    if (linec<2) { fprintf(stderr,"%s:%d: malformed line\n",path,lineno); return -1; }
    int hi=_hexeval(line[0]);
    int lo=_hexeval(line[1]);
    if ((hi<0)||(lo<0)) { fprintf(stderr,"%s:%d: line must begin with 2 hexadecimal digits\n",path,lineno); return -1; }
    int tileid=(hi<<4)|lo;
    int linep=2; while (linep<linec) {
      if ((unsigned char)line[linep]<=0x20) { linep++; continue; }
      const char *token=line+linep;
      int tokenc=0;
      while ((linep<linec)&&((unsigned char)line[linep]>0x20)) { linep++; tokenc++; }
      int mask=_adv_tsprop_mask_from_token(token,tokenc);
      if (mask<0) { fprintf(stderr,"%s:%d: undefined tile property token '%.*s'\n",path,lineno,tokenc,token); return -1; }
      dst[tileid]|=mask;
    }
  }
  return 0;
}

/* load tilesheet properties
 *****************************************************************************/
 
static int adv_res_load_tilesheet_properties() {
  char subpath[1024];
  if (adv_resmgr.rootc>=sizeof(subpath)-8) return -1;
  memcpy(subpath,adv_resmgr.root,adv_resmgr.rootc);
  memcpy(subpath+adv_resmgr.rootc,"/tsprop",8);
  int subpathc=adv_resmgr.rootc+7;
  DIR *dir=opendir(subpath);
  if (!dir) return 0;
  subpath[subpathc++]='/';
  struct dirent *de;
  while (de=readdir(dir)) {
    if (de->d_type!=DT_REG) continue;
    const char *base=de->d_name;
    if ((base[0]<'0')||(base[0]>'9')) continue;
    int basec=0,id=0,done=0; while (base[basec]) {
      if (!done) {
        if ((base[basec]<'0')||(base[basec]>'9')) done=1;
        else {
          int digit=base[basec]-'0';
          if (id>INT_MAX/10) id=INT_MAX; else id*=10;
          if (id>INT_MAX-digit) id=INT_MAX; else id+=digit;
        }
      }
      basec++;
    }
    int p=adv_res_tilesheet_search(id);
    if (p<0) {
      fprintf(stderr,"WARNING: have tilesheet properties for #%d, but no such tilesheet\n",id);
      continue;
    }
    if (subpathc>=sizeof(subpath)-basec) continue;
    memcpy(subpath+subpathc,base,basec+1);
    char *src=0;
    int srcc=adv_file_read(&src,subpath);
    if (srcc<0) { closedir(dir); return -1; }
    int err=adv_res_load_tsprop(adv_resmgr.tilesheetv[p].propv,src,srcc,subpath);
    if (src) free(src);
    if (err<0) return err;
  }
  closedir(dir);
  return 0;
}

/* enumerate tilesheets
 *****************************************************************************/

static int adv_res_enumerate_tilesheets() {
  char subpath[1024];
  if (adv_resmgr.rootc>=sizeof(subpath)-11) return -1;
  memcpy(subpath,adv_resmgr.root,adv_resmgr.rootc);
  memcpy(subpath+adv_resmgr.rootc,"/tilesheet",11);
  int subpathc=adv_resmgr.rootc+10;
  DIR *dir=opendir(subpath);
  if (!dir) return -1;
  subpath[subpathc++]='/';
  struct dirent *de;
  while (de=readdir(dir)) {
    if (de->d_type!=DT_REG) continue;
    const char *base=de->d_name;
    if ((base[0]<'0')||(base[0]>'9')) continue;
    int basec=0,id=0,done=0; while (base[basec]) {
      if (!done) {
        if ((base[basec]<'0')||(base[basec]>'9')) done=1;
        else {
          int digit=base[basec]-'0';
          if (id>INT_MAX/10) id=INT_MAX; else id*=10;
          if (id>INT_MAX-digit) id=INT_MAX; else id+=digit;
        }
      }
      basec++;
    }
    if ((basec<4)||memcmp(base+basec-4,".png",4)) continue;
    if (subpathc>sizeof(subpath)-basec) continue;
    int tilesheetp=adv_res_tilesheet_search(id);
    if (tilesheetp>=0) {
      fprintf(stderr,"WARNING: Multiple tilesheets with ID %d. Keeping '%s'; ignoring '%s'.\n",id,adv_resmgr.tilesheetv[tilesheetp].path,base);
      continue;
    }
    tilesheetp=-tilesheetp-1;
    memcpy(subpath+subpathc,base,basec);
    if (adv_res_tilesheet_insert(tilesheetp,id,subpath,subpathc+basec)<0) { closedir(dir); return -1; }
  }
  closedir(dir);
  return adv_res_load_tilesheet_properties();
}

/* init
 *****************************************************************************/

int adv_res_init(const char *programpath,int argc,char **argv) {
  adv_res_wipe_globals();
  
  adv_resmgr.bgimageid=-1;
  adv_resmgr.sprimageid=-1;
  
  if (adv_res_locate_data(programpath,argc,argv)<0) return -1;
  printf("%s: Using data set at '%.*s'.\n",programpath,adv_resmgr.rootc,adv_resmgr.root);
  if (adv_res_enumerate_tilesheets()<0) return -1;

  if (adv_res_load_sprdefs()<0) return -1;
  
  if (adv_res_load_songs()<0) return -1;

  if (adv_res_load_maps()<0) return -1;
  if (adv_res_load_map(1)<0) return -1;
  
  return 0;
}

/* quit
 *****************************************************************************/

void adv_res_quit() {
  int i;
  if (adv_resmgr.root) free(adv_resmgr.root);
  if (adv_resmgr.bgimage) free(adv_resmgr.bgimage);
  if (adv_resmgr.tilesheetv) {
    for (i=0;i<adv_resmgr.tilesheetc;i++) free(adv_resmgr.tilesheetv[i].path);
    free(adv_resmgr.tilesheetv);
  }
  if (adv_resmgr.mapv) {
    for (i=0;i<adv_resmgr.mapc;i++) adv_map_del(adv_resmgr.mapv[i]);
    free(adv_resmgr.mapv);
  }
  if (adv_resmgr.sprdefv) {
    for (i=0;i<adv_resmgr.sprdefc;i++) adv_sprdef_del(adv_resmgr.sprdefv[i]);
    free(adv_resmgr.sprdefv);
  }
  adv_res_wipe_globals();
}

/* tilesheet list, private
 *****************************************************************************/
 
int adv_res_tilesheet_search(int id) {
  int lo=0,hi=adv_resmgr.tilesheetc;
  while (lo<hi) {
    int ck=(lo+hi)>>1;
         if (id<adv_resmgr.tilesheetv[ck].id) hi=ck;
    else if (id>adv_resmgr.tilesheetv[ck].id) lo=ck+1;
    else return ck;
  }
  return -lo-1;
}

int adv_res_tilesheet_insert(int p,int id,const char *path,int pathc) {
  if ((p<0)||(p>adv_resmgr.tilesheetc)||((pathc>0)&&!path)) return -1;
  if (pathc<0) { pathc=0; if (path) while (path[pathc]) pathc++; }
  if (pathc==INT_MAX) pathc--;
  if (p&&(id<=adv_resmgr.tilesheetv[p-1].id)) return -1;
  if ((p<adv_resmgr.tilesheetc)&&(id>=adv_resmgr.tilesheetv[p].id)) return -1;
  if (adv_resmgr.tilesheetc>=adv_resmgr.tilesheeta) {
    int na=adv_resmgr.tilesheeta+8;
    if (na>INT_MAX/sizeof(struct adv_tilesheet)) return -1;
    void *nv=realloc(adv_resmgr.tilesheetv,sizeof(struct adv_tilesheet)*na);
    if (!nv) return -1;
    adv_resmgr.tilesheetv=nv;
    adv_resmgr.tilesheeta=na;
  }
  char *npath=malloc(pathc+1);
  if (!npath) return -1;
  memcpy(npath,path,pathc);
  npath[pathc]=0;
  memmove(adv_resmgr.tilesheetv+p+1,adv_resmgr.tilesheetv+p,sizeof(struct adv_tilesheet)*(adv_resmgr.tilesheetc-p));
  adv_resmgr.tilesheetc++;
  struct adv_tilesheet *tilesheet=adv_resmgr.tilesheetv+p;
  memset(tilesheet,0,sizeof(struct adv_tilesheet));
  tilesheet->id=id;
  tilesheet->path=npath;
  return 0;
}

/* get palettes
 *****************************************************************************/

void adv_res_get_background_palette(void **bg) {
  if (bg) *bg=adv_resmgr.bgimage;
}

/* get miscimg
 * TODO: It would probably be wise to read the directory only once, at startup, and store all the relevant paths.
 *****************************************************************************/
 
int adv_res_get_miscimg(void *dstpp,int *w,int *h,int miscimgid) {
  if (!dstpp||!w||!h||(miscimgid<1)) return -1;
  char path[1024];
  if (adv_resmgr.rootc>=sizeof(path)-8) return -1;
  memcpy(path,adv_resmgr.root,adv_resmgr.rootc);
  memcpy(path+adv_resmgr.rootc,"/miscimg",9);
  int pathc=adv_resmgr.rootc+8;
  DIR *dir=opendir(path);
  if (!dir) return -1;
  path[pathc++]='/';
  struct dirent *de=0;
  while (de=readdir(dir)) {
    if (de->d_type!=DT_REG) continue;
    const char *base=de->d_name;
    int basec=0,id=0;
    while (base[basec]&&(base[basec]>='0')&&(base[basec]<='9')) {
      int digit=base[basec++]-'0';
      if (id>INT_MAX/10) { id=-1; break; } id*=10;
      if (id>INT_MAX-digit) { id=-1; break; } id+=digit;
    }
    if (id!=miscimgid) continue;
    while (base[basec]) basec++;
    if (pathc>=sizeof(path)-basec) { closedir(dir); return -1; }
    memcpy(path+pathc,base,basec+1);
    
    closedir(dir);
    void *serial=0;
    int serialc=adv_file_read(&serial,path);
    if (serialc<0) return -1;
    struct png_image *image=png_decode(serial,serialc);
    free(serial);
    if (!image) return -1;
    if ((image->depth!=8)||(image->colortype!=6)) {
      fprintf(stderr,"%s: Must be 32-bit RGBA (have depth=%d colortype=%d)\n",path,image->depth,image->colortype);
      png_image_del(image);
      return -1;
    }
    *(void**)dstpp=image->pixels;
    image->pixels=0;
    *w=image->w; 
    *h=image->h;
    png_image_del(image);
    return 0;
  }
  closedir(dir);
  return -1;
}
