#ifndef ADV_VIDEO_INTERNAL_H
#define ADV_VIDEO_INTERNAL_H

#include "adv.h"
#include "adv_video.h"
#include "sprite/adv_sprite.h"
#include <pig.h>
#include <GLES2/gl2.h>

#define ADV_SPLASH_FADE_TIME 120

/* Vertex attributes. */
#define ADV_VTXATTR_POSITION         0
#define ADV_VTXATTR_COLOR            1
#define ADV_VTXATTR_ROTATION         2
#define ADV_VTXATTR_TEXPOSITION      3
#define ADV_VTXATTR_SCALE            4
#define ADV_VTXATTR_OPACITY          5
#define ADV_VTXATTR_XFORM            6

struct adv_plainsprites_vtx {
  GLshort x,y;
  GLubyte texcol,texrow;
  GLubyte xform;
};

extern struct adv_video {
  int screenw,screenh;

  GLuint program_bg;
  GLuint texture_bg;
  unsigned char *bgbuffer; // RGB, ADV_SCREEN_W*ADV_SCREEN_H
  int bg_lights_out_location;
  int bg_lightpos_location;
  int do_lights_out;
  float lightsoutx,lightsouty;
  
  GLuint program_plainsprites;
  GLuint texture_plainsprites;
  struct adv_plainsprites_vtx *plainsprites_vtxv;
  int plainsprites_vtxa;
  
  GLuint program_splash;
  GLuint texture_splash;
  GLfloat splash_vtxv[20]; // (x,y,tx,ty,opacity)*4; (x,y) are in (-1..1); (tx,ty) are in (0..1).
  int splash_id;
  int splash_duration; // counts down
  
  int main_menu_enable;
  
} adv_video;

int adv_shaders_load();

#endif
