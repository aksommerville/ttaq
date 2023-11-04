#include "adv_video_internal.h"
#include "res/adv_res.h"
#include "game/adv_map.h"

struct adv_video adv_video={0};

/* Output bounds.
 *********************************************************/
 
static void adv_video_recalculate_mid_vtxv() {

  int dstw,dsth;
  if ((dstw=(ADV_SCREEN_W*adv_video.screenh)/ADV_SCREEN_H)<=adv_video.screenw) {
    dsth=adv_video.screenh;
  } else {
    dstw=adv_video.screenw;
    dsth=(ADV_SCREEN_H*adv_video.screenw)/ADV_SCREEN_W;
  }
  GLfloat xscale=(GLfloat)dstw/(GLfloat)adv_video.screenw;
  GLfloat yscale=(GLfloat)dsth/(GLfloat)adv_video.screenh;

  GLfloat *dst=adv_video.mid_vtxv;
  
  dst[0]=-1.0f*xscale;
  dst[1]=1.0f*yscale;
  dst[2]=0.0f;
  dst[3]=1.0f;
  dst[4]=1.0f;
  
  dst[5]=-1.0f*xscale;
  dst[6]=-1.0f*yscale;
  dst[7]=0.0f;
  dst[8]=0.0f;
  dst[9]=1.0f;
  
  dst[10]=1.0f*xscale;
  dst[11]=1.0f*yscale;
  dst[12]=1.0f;
  dst[13]=1.0f;
  dst[14]=1.0f;
  
  dst[15]=1.0f*xscale;
  dst[16]=-1.0f*yscale;
  dst[17]=1.0f;
  dst[18]=0.0f;
  dst[19]=1.0f;
}

void adv_video_resized(int w,int h) {
  if ((w<1)||(h<1)) return;
  adv_video.screenw=w;
  adv_video.screenh=h;
  adv_video_recalculate_mid_vtxv();
}

/* init
 *****************************************************************************/

int adv_video_init() {
  int err;
  memset(&adv_video,0,sizeof(struct adv_video));
  
  #if USE_glx
    if (glx_init()<0) {
      fprintf(stderr,"glx_init failed\n");
      return -1;
    }
    glx_get_screen_size(&adv_video.screenw,&adv_video.screenh);
  #elif USE_drm
    if (fmn_drm_init()<0) {
      fprintf(stderr,"fmn_drm_init failed\n");
      return -1;
    }
    fmn_drm_get_screen_size(&adv_video.screenw,&adv_video.screenh);
  #elif USE_bcm
    if (fmn_bcm_init()<0) {
      fprintf(stderr,"fmn_bcm_init failed\n");
      return -1;
    }
    adv_video.screenw=fmn_bcm_get_width();
    adv_video.screenh=fmn_bcm_get_height();
  #endif
  
  glClearColor(0.0,0.0,0.0,1.0);

  if (!(adv_video.bgbuffer=calloc(ADV_SCREEN_W*3,ADV_SCREEN_H))) return -1;

  if ((err=adv_shaders_load())<0) return err;
  
  glGenTextures(1,&adv_video.texture_mid);
  glBindTexture(GL_TEXTURE_2D,adv_video.texture_mid);
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR); // GL_NEAREST also sensible. Can we make it configurable?
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
  glTexImage2D(GL_TEXTURE_2D,0,GL_RGB,ADV_SCREEN_W,ADV_SCREEN_H,0,GL_RGB,GL_UNSIGNED_BYTE,0);
  glGenFramebuffers(1,&adv_video.fb_mid);
  glBindFramebuffer(GL_FRAMEBUFFER,adv_video.fb_mid);
  glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,adv_video.texture_mid,0);
  glBindFramebuffer(GL_FRAMEBUFFER,0);

  glGenTextures(1,&adv_video.texture_bg);
  glBindTexture(GL_TEXTURE_2D,adv_video.texture_bg);
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);

  glGenTextures(1,&adv_video.texture_plainsprites);
  glBindTexture(GL_TEXTURE_2D,adv_video.texture_plainsprites);
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
  
  glGenTextures(1,&adv_video.texture_splash);
  glBindTexture(GL_TEXTURE_2D,adv_video.texture_splash);
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);

  glBlendFuncSeparate(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA,GL_SRC_ALPHA,GL_DST_ALPHA);
  
  adv_video_recalculate_mid_vtxv();
  
  if (glGetError()) return -1;
  return 0;
}

/* quit
 *****************************************************************************/

void adv_video_quit() {
  if (adv_video.bgbuffer) free(adv_video.bgbuffer);
  if (adv_video.plainsprites_vtxv) free(adv_video.plainsprites_vtxv);
  glDeleteProgram(adv_video.program_bg);
  glDeleteTextures(1,&adv_video.texture_bg);
  glDeleteProgram(adv_video.program_plainsprites);
  glDeleteTextures(1,&adv_video.texture_plainsprites);
  glDeleteProgram(adv_video.program_splash);
  glDeleteTextures(1,&adv_video.texture_splash);
  glDeleteTextures(1,&adv_video.texture_mid);
  glDeleteFramebuffers(1,&adv_video.fb_mid);
  
  #if USE_glx
    glx_quit();
  #elif USE_drm
    fmn_drm_quit();
  #elif USE_bcm
    fmn_bcm_quit();
  #endif
  
  memset(&adv_video,0,sizeof(struct adv_video));
}

/* setup lights-out effect
 *****************************************************************************/
 
int adv_video_lights_out(int do_effect,int lampx,int lampy) {
  if (adv_video.do_lights_out=do_effect) {
    adv_video.lightsoutx=lampx/(float)ADV_SCREEN_W;
    adv_video.lightsouty=lampy/(float)ADV_SCREEN_H;
  }
  return 0;
}

/* update
 *****************************************************************************/

int adv_video_update() {
  int err,i;
  static const GLbyte limit_vtxv[8]={0,0, 1,0, 0,1, 1,1};
  
  #if USE_glx
    if (glx_update()<0) return err;
    if (glx_begin()<0) return err;
  #elif USE_drm
    // context always active, and no events
  #elif USE_bcm
    // context always active, and no events
  #endif
  
  glBindFramebuffer(GL_FRAMEBUFFER,adv_video.fb_mid);
  glViewport(0,0,ADV_SCREEN_W,ADV_SCREEN_H);
  
  /* Draw background. */
  glUseProgram(adv_video.program_bg);
  if (adv_video.do_lights_out) {
    glClearColor(0.0f,0.0f,0.0f,1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    glEnable(GL_BLEND);
    glUniform1f(adv_video.bg_lights_out_location,(adv_video.do_lights_out==1)?0.1:2.0);
    glUniform2f(adv_video.bg_lightpos_location,adv_video.lightsoutx,adv_video.lightsouty);
  } else {
    glDisable(GL_BLEND);
    glUniform1f(adv_video.bg_lights_out_location,0.0);
  }
  glBindTexture(GL_TEXTURE_2D,adv_video.texture_bg);
  glEnableVertexAttribArray(ADV_VTXATTR_POSITION);
  glDisableVertexAttribArray(ADV_VTXATTR_TEXPOSITION);
  glDisableVertexAttribArray(ADV_VTXATTR_XFORM);
  glVertexAttribPointer(ADV_VTXATTR_POSITION,2,GL_BYTE,GL_FALSE,0,limit_vtxv);
  glDrawArrays(GL_TRIANGLE_STRIP,0,4);

  /* Draw sprites. */
  if (adv_sprgrp_visible&&(adv_sprgrp_visible->sprc>0)) {
    adv_sprgrp_sort_by_depth(adv_sprgrp_visible);
    glEnable(GL_BLEND);
    glEnable(GL_POINT_SPRITE);
    glEnable(GL_PROGRAM_POINT_SIZE);
    glUseProgram(adv_video.program_plainsprites);
    glBindTexture(GL_TEXTURE_2D,adv_video.texture_plainsprites);
    glEnableVertexAttribArray(ADV_VTXATTR_POSITION);
    glEnableVertexAttribArray(ADV_VTXATTR_TEXPOSITION);
    glEnableVertexAttribArray(ADV_VTXATTR_XFORM);
    if (adv_sprgrp_visible->sprc>adv_video.plainsprites_vtxa) {
      if (adv_sprgrp_visible->sprc>INT_MAX/sizeof(struct adv_plainsprites_vtx)) return -1;
      void *nv=realloc(adv_video.plainsprites_vtxv,sizeof(struct adv_plainsprites_vtx)*adv_sprgrp_visible->sprc);
      if (!nv) return -1;
      adv_video.plainsprites_vtxv=nv;
      adv_video.plainsprites_vtxa=adv_sprgrp_visible->sprc;
    }
    for (i=0;i<adv_sprgrp_visible->sprc;i++) {
      struct adv_sprite *spr=adv_sprgrp_visible->sprv[i];
      if (spr->tileid&~0xff) continue;
      adv_video.plainsprites_vtxv[i].x=spr->x;
      adv_video.plainsprites_vtxv[i].y=spr->y;
      adv_video.plainsprites_vtxv[i].texcol=spr->tileid&15;
      adv_video.plainsprites_vtxv[i].texrow=spr->tileid>>4;
      adv_video.plainsprites_vtxv[i].xform=spr->flop?1:0;
    }
    glVertexAttribPointer(ADV_VTXATTR_POSITION,2,GL_SHORT,GL_FALSE,sizeof(struct adv_plainsprites_vtx),&adv_video.plainsprites_vtxv[0].x);
    glVertexAttribPointer(ADV_VTXATTR_TEXPOSITION,2,GL_UNSIGNED_BYTE,GL_FALSE,sizeof(struct adv_plainsprites_vtx),&adv_video.plainsprites_vtxv[0].texcol);
    glVertexAttribPointer(ADV_VTXATTR_XFORM,1,GL_UNSIGNED_BYTE,GL_FALSE,sizeof(struct adv_plainsprites_vtx),&adv_video.plainsprites_vtxv[0].xform);
    glDrawArrays(GL_POINTS,0,adv_sprgrp_visible->sprc);
  }
  
  /* Draw splash. */
  if (adv_video.splash_duration) {
    if (adv_video.splash_duration>0) adv_video.splash_duration--;
    if ((adv_video.splash_duration>=0)&&(adv_video.splash_duration<ADV_SPLASH_FADE_TIME)) {
      adv_video.splash_vtxv[4]=(GLfloat)adv_video.splash_duration/(GLfloat)ADV_SPLASH_FADE_TIME;
    } else {
      adv_video.splash_vtxv[4]=1.0;
    }
    adv_video.splash_vtxv[9]=adv_video.splash_vtxv[14]=adv_video.splash_vtxv[19]=adv_video.splash_vtxv[4];
    glEnable(GL_BLEND);
    glUseProgram(adv_video.program_splash);
    glBindTexture(GL_TEXTURE_2D,adv_video.texture_splash);
    glEnableVertexAttribArray(ADV_VTXATTR_POSITION);
    glEnableVertexAttribArray(ADV_VTXATTR_TEXPOSITION);
    glEnableVertexAttribArray(ADV_VTXATTR_OPACITY);
    glDisableVertexAttribArray(ADV_VTXATTR_XFORM);
    glVertexAttribPointer(ADV_VTXATTR_POSITION,2,GL_FLOAT,GL_FALSE,sizeof(GLfloat)*5,adv_video.splash_vtxv+0);
    glVertexAttribPointer(ADV_VTXATTR_TEXPOSITION,2,GL_FLOAT,GL_FALSE,sizeof(GLfloat)*5,adv_video.splash_vtxv+2);
    glVertexAttribPointer(ADV_VTXATTR_OPACITY,1,GL_FLOAT,GL_FALSE,sizeof(GLfloat)*5,adv_video.splash_vtxv+4);
    glDrawArrays(GL_TRIANGLE_STRIP,0,4);
    glDisableVertexAttribArray(ADV_VTXATTR_OPACITY);
  }
  
  /* Transfer to main output. */
  glBindFramebuffer(GL_FRAMEBUFFER,0);
  glViewport(0,0,adv_video.screenw,adv_video.screenh);
  glClearColor(0.0f,0.0f,0.0f,1.0f);
  glClear(GL_COLOR_BUFFER_BIT);
  glDisable(GL_BLEND);
  glUseProgram(adv_video.program_splash);
  glBindTexture(GL_TEXTURE_2D,adv_video.texture_mid);
  glEnableVertexAttribArray(ADV_VTXATTR_POSITION);
  glEnableVertexAttribArray(ADV_VTXATTR_TEXPOSITION);
  glEnableVertexAttribArray(ADV_VTXATTR_OPACITY);
  glDisableVertexAttribArray(ADV_VTXATTR_XFORM);
  glVertexAttribPointer(ADV_VTXATTR_POSITION,2,GL_FLOAT,GL_FALSE,sizeof(GLfloat)*5,adv_video.mid_vtxv+0);
  glVertexAttribPointer(ADV_VTXATTR_TEXPOSITION,2,GL_FLOAT,GL_FALSE,sizeof(GLfloat)*5,adv_video.mid_vtxv+2);
  glVertexAttribPointer(ADV_VTXATTR_OPACITY,1,GL_FLOAT,GL_FALSE,sizeof(GLfloat)*5,adv_video.mid_vtxv+4);
  glDrawArrays(GL_TRIANGLE_STRIP,0,4);
  glDisableVertexAttribArray(ADV_VTXATTR_OPACITY);

  #if USE_glx
    if (glx_end()<0) return -1;
  #elif USE_drm
    if (fmn_drm_swap()<0) return -1;
  #elif USE_bcm
    if (fmn_bcm_swap()<0) return -1;
  #endif
  
  return 0;
}

/* draw map
 *****************************************************************************/

static void _adv_video_draw_bg_tile(unsigned char *dst,int dststride,unsigned char tileid,const unsigned char *src,int srcstride,int tilerowsize) {
  int x=(tileid&15);
  int y=(tileid>>4)*ADV_TILE_H;
  src+=srcstride*y+x*tilerowsize;
  int i; for (i=0;i<ADV_TILE_H;i++) {
    memcpy(dst,src,tilerowsize);
    dst+=dststride;
    src+=srcstride;
  }
}

static void _adv_video_draw_bg_1(unsigned char *dst,const unsigned char *tiledata,const unsigned char *src,int pixelsize) {
  int tiledatastride=ADV_MAP_W;           // one row of tile data in bytes (2 bytes per cell)
  int tilerowsize=pixelsize*ADV_TILE_W;   // one row of one tile in bytes
  int dststride=ADV_SCREEN_W*pixelsize;   // one row of output buffer in bytes
  int dstlongstride=dststride*ADV_TILE_H; // one tile row of output buffer in bytes
  int srcstride=tilerowsize<<4;           // one row of input palette in bytes
  int row; for (row=0;row<ADV_MAP_H;row++) {
    const unsigned char *tdptr=tiledata; tiledata+=tiledatastride;
    unsigned char *dstptr=dst; dst+=dstlongstride;
    int col; for (col=0;col<ADV_MAP_W;col++,tdptr++,dstptr+=tilerowsize) {
      _adv_video_draw_bg_tile(dstptr,dststride,*tdptr,src,srcstride,tilerowsize);
    }
  }
}
 
int adv_video_draw_bg(const void *tiledata,const void *img_bg) {
  if (!tiledata||!img_bg) return -1;
  if (!adv_video.bgbuffer) return -1;
  _adv_video_draw_bg_1(adv_video.bgbuffer,tiledata,img_bg,3);
  glBindTexture(GL_TEXTURE_2D,adv_video.texture_bg);
  glTexImage2D(GL_TEXTURE_2D,0,GL_RGB,ADV_SCREEN_W,ADV_SCREEN_H,0,GL_RGB,GL_UNSIGNED_BYTE,adv_video.bgbuffer);
  return 0;
}

int adv_video_draw_bg_sub(const void *_tiledata,const void *img_bg,int col,int row,int colc,int rowc) {
  if (!_tiledata||!img_bg) return -1;
  if (!adv_video.bgbuffer) return -1;
  if ((colc<1)||(rowc<1)) return 0;
  if (col<0) { colc+=col; col=0; }
  if (row<0) { rowc+=row; row=0; }
  if (col>ADV_MAP_W-colc) colc=ADV_MAP_W-col;
  if (row>ADV_MAP_H-rowc) rowc=ADV_MAP_H-row;
  if ((colc<1)||(rowc<1)) return 0;
  int tiledatastride=ADV_MAP_W;           // one row of tile data in bytes (2 bytes per cell)
  int tilerowsize=3*ADV_TILE_W;           // one row of one tile in bytes
  int dststride=ADV_SCREEN_W*3;           // one row of output buffer in bytes
  int dstlongstride=dststride*ADV_TILE_H; // one tile row of output buffer in bytes
  int srcstride=tilerowsize<<4;           // one row of input palette in bytes
  const unsigned char *tiledata=((const unsigned char*)_tiledata)+tiledatastride*row+col;
  const unsigned char *src=((const unsigned char*)img_bg);
  unsigned char *dst=((unsigned char*)adv_video.bgbuffer)+dstlongstride*row+(col*ADV_TILE_W*3);
  int ri; for (ri=0;ri<rowc;ri++) {
    const unsigned char *tdptr=tiledata; tiledata+=tiledatastride;
    unsigned char *dstptr=dst; dst+=dstlongstride;
    int ci; for (ci=0;ci<colc;ci++,tdptr++,dstptr+=tilerowsize) {
      _adv_video_draw_bg_tile(dstptr,dststride,*tdptr,src,srcstride,tilerowsize);
    }
  }
  glBindTexture(GL_TEXTURE_2D,adv_video.texture_bg);
  dst=((unsigned char*)adv_video.bgbuffer)+dstlongstride*row+(col*ADV_TILE_W*3);
  int x=col*ADV_TILE_W,y=row*ADV_TILE_H,w=colc*ADV_TILE_W;
  int i; for (i=rowc*ADV_TILE_H;i-->0;dst+=dststride,y++) {
    glTexSubImage2D(GL_TEXTURE_2D,0,x,y,w,1,GL_RGB,GL_UNSIGNED_BYTE,dst);
  }
  return 0;
}

/* add sprite image
 *****************************************************************************/

int adv_video_set_spritesheet(const void *spritesheet) {
  if (!spritesheet) return -1;
  glBindTexture(GL_TEXTURE_2D,adv_video.texture_plainsprites);
  glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,ADV_TILE_W<<4,ADV_TILE_H<<4,0,GL_RGBA,GL_UNSIGNED_BYTE,spritesheet);
  return 0;
}

/* setup splash
 *****************************************************************************/
 
int adv_video_splash(int imgid,int duration) {
  if (imgid==adv_video.splash_id) {
    adv_video.splash_duration=duration;
    return 0;
  }
  if (!imgid) {
    adv_video.splash_duration=0;
    return 0;
  }
  void *pixels=0;
  int w=0,h=0;
  if (adv_res_get_miscimg(&pixels,&w,&h,imgid)<0) return -1;
  if (!pixels) return -1;
  glBindTexture(GL_TEXTURE_2D,adv_video.texture_splash);
  glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,w,h,0,GL_RGBA,GL_UNSIGNED_BYTE,pixels);
  free(pixels);
  adv_video.splash_id=imgid;
  adv_video.splash_duration=duration;
  
  /* Try to put the image's center at 1/2 horizontally and 1/3 vertically.
   * Clamp to screen edges as necessary.
   */
  GLfloat adjw=(w*2.0f)/ADV_SCREEN_W;
  GLfloat adjh=(h*2.0f)/ADV_SCREEN_H;
  if (adjw<0.1) adjw=0.1; else if (adjw>2.0) adjw=2.0;
  if (adjh<0.1) adjh=0.1; else if (adjh>2.0) adjh=2.0;
  GLfloat left=adjw*-0.5f;
  GLfloat top=0.33f+adjh*0.5f;
  if (top>1.0f) top=1.0f;
  adv_video.splash_vtxv[ 0]=left;      adv_video.splash_vtxv[ 1]=top;      adv_video.splash_vtxv[ 2]=0.0f; adv_video.splash_vtxv[ 3]=0.0f;
  adv_video.splash_vtxv[ 5]=left;      adv_video.splash_vtxv[ 6]=top-adjh; adv_video.splash_vtxv[ 7]=0.0f; adv_video.splash_vtxv[ 8]=1.0f;
  adv_video.splash_vtxv[10]=left+adjw; adv_video.splash_vtxv[11]=top;      adv_video.splash_vtxv[12]=1.0f; adv_video.splash_vtxv[13]=0.0f;
  adv_video.splash_vtxv[15]=left+adjw; adv_video.splash_vtxv[16]=top-adjh; adv_video.splash_vtxv[17]=1.0f; adv_video.splash_vtxv[18]=1.0f;
  
  return 0;
}

/* main menu
 *****************************************************************************/
 
int adv_video_begin_main_menu() {
  if (!adv_video.bgbuffer) return -1;
  if (adv_video.main_menu_enable) return 0;
  memset(adv_video.bgbuffer,0x80,ADV_SCREEN_W*ADV_SCREEN_H*3);
  glBindTexture(GL_TEXTURE_2D,adv_video.texture_bg);
  glTexImage2D(GL_TEXTURE_2D,0,GL_RGB,ADV_SCREEN_W,ADV_SCREEN_H,0,GL_RGB,GL_UNSIGNED_BYTE,adv_video.bgbuffer);
  adv_video.main_menu_enable=1;
  adv_sprgrp_kill(adv_sprgrp_keepalive);
  adv_video_splash(5,-1);
  if (adv_res_replace_spritesheet(110)<0) return -1;
  return 0;
}

int adv_video_end_main_menu() {
  if (!adv_video.main_menu_enable) return 0;
  adv_sprgrp_kill(adv_sprgrp_keepalive);
  if (adv_res_reload_map()<0) return -1;
  adv_video.main_menu_enable=0;
  adv_video.splash_duration=0;
  return 0;
}

int adv_video_is_main_menu() {
  return adv_video.main_menu_enable;
}

/* Toggle fullscreen.
 *************************************************************/
 
void adv_video_toggle_fullscreen() {
  #if USE_glx
    glx_toggle_fullscreen();
  #endif
  // drm and bcm won't do this; they are inherently fullscreen always.
}

void adv_video_suppress_screensaver() {
  #if USE_glx
    glx_suppress_screensaver();
  #endif
}
