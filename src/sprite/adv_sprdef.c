#include "adv.h"
#include "adv_sprdef.h"
#include "adv_sprclass.h"

/* new
 *****************************************************************************/

int adv_sprdef_new(struct adv_sprdef **dst) {
  if (!dst) return -1;
  if (!(*dst=calloc(1,sizeof(struct adv_sprdef)))) return -1;
  return 0;
}

/* del
 *****************************************************************************/

void adv_sprdef_del(struct adv_sprdef *sprdef) {
  if (!sprdef) return;
  free(sprdef);
}

/* decode single line
 *****************************************************************************/

static int _rdhex(const char *src,int srcc) {
  int dst=0,srcp=0;
  if (srcc<1) return -1;
  while (srcp<srcc) {
    char digit=src[srcp++];
         if ((digit>='0')&&(digit<='9')) digit=digit-'0';
    else if ((digit>='a')&&(digit<='f')) digit=digit-'a'+10;
    else if ((digit>='A')&&(digit<='F')) digit=digit-'A'+10;
    else return -1;
    if (dst>INT_MAX>>4) return -1; 
    dst<<=4;
    dst|=digit;
  }
  return dst;
}

static int _rdint(const char *src,int srcc) {
  int dst=0,srcp=0;
  if (srcc<1) return -1;
  while (srcp<srcc) {
    int digit=src[srcp++]-'0';
    if ((digit<0)||(digit>9)) return -1;
    if (dst>INT_MAX/10) return -1; dst*=10;
    if (dst>INT_MAX-digit) return -1; dst+=digit;
  }
  return dst;
}

static int adv_sprdef_decode_line(struct adv_sprdef *sprdef,const char *src,int srcc,const char *refname,int lineno) {
  int srcp=0,argid;
  while ((srcp<srcc)&&((unsigned char)src[srcp]<=0x20)) srcp++;
  const char *k=src+srcp; int kc=0;
  while ((srcp<srcc)&&((unsigned char)src[srcp]>0x20)) { srcp++; kc++; }
  while ((srcp<srcc)&&((unsigned char)src[srcp]<=0x20)) srcp++;
  const char *arg=src+srcp; int argc=srcc-srcp;
  while (argc&&((unsigned char)arg[argc-1]<=0x20)) argc--;

  if ((kc==4)&&!memcmp(k,"tile",4)) {
    if ((sprdef->tileid=_rdhex(arg,argc))<0) {
      fprintf(stderr,"%s:%d: expected hexadecimal integer after 'tile'\n",refname,lineno);
      return -1;
    }
    if (sprdef->tileid>255) {
      fprintf(stderr,"%s:%d: tile must be in 0..ff (have %x)\n",refname,lineno,sprdef->tileid);
      return -1;
    }

  } else if ((kc==5)&&!memcmp(k,"class",5)) {
    if (!(sprdef->sprclass=adv_sprclass_for_name(arg,argc))) {
      fprintf(stderr,"%s:%d: undefined sprite class '%.*s'\n",refname,lineno,argc,arg);
      return -1;
    }
    
  } else if ((argid=adv_sprclass_lookup_argument(sprdef->sprclass,k,kc))>=0) {
    if ((sprdef->argv[argid]=_rdint(arg,argc))<0) {
      fprintf(stderr,"%s:%d: expected decimal integer after '%.*s'\n",refname,lineno,kc,k);
      return -1;
    }

  } else {
    fprintf(stderr,"%s:%d: unexpected key '%.*s'\n",refname,lineno,kc,k);
    return -1;
  }
  return 0;
}

/* decode file
 *****************************************************************************/

int adv_sprdef_decode(struct adv_sprdef *sprdef,const void *src,int srcc,const char *refname) {
  if (!sprdef||(srcc<0)||(srcc&&!src)) return -1;
  if (!refname) refname="(unknown)";
  const unsigned char *SRC=src;
  int srcp=0,lineno=1,err;
  while (srcp<srcc) {
    if ((SRC[srcp]==0x0a)||(SRC[srcp]==0x0d)) {
      lineno++;
      if (++srcp>=srcc) break;
      if (((SRC[srcp]==0x0a)||(SRC[srcp]==0x0d))&&(SRC[srcp]!=SRC[srcp-1])) srcp++;
      continue;
    }
    if (SRC[srcp]<=0x20) { srcp++; continue; }
    const unsigned char *line=src+srcp;
    int linec=0,cmt=0;
    while ((srcp<srcc)&&(SRC[srcp]!=0x0a)&&(SRC[srcp]!=0x0d)) {
      if (SRC[srcp]=='#') cmt=1;
      else if (SRC[srcp]>0x7e) { fprintf(stderr,"%s:%d: illegal byte 0x%02x\n",refname,lineno,SRC[srcp]); return -1; }
      else if (!cmt) linec++;
      srcp++;
    }
    while (linec&&(line[linec-1]<=0x20)) linec--;
    if (!linec) continue;
    if (adv_sprdef_decode_line(sprdef,line,linec,refname,lineno)<0) return -1;
  }
  return 0;
}
 
int adv_sprdef_decode_file(struct adv_sprdef *sprdef,const char *path) {
  void *src=0;
  int srcc=adv_file_read(&src,path);
  if (srcc<0) return -1;
  int err=adv_sprdef_decode(sprdef,src,srcc,path);
  if (src) free(src);
  return err;
}
