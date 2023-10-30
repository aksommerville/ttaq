#include "adv_video_internal.h"

/* shader source
 * These are generated from ESSL files in our source tree.
 * The text of those ESSL files is written directly into our executable.
 * TODO: Look into precompiling the shaders. Not a big deal.
 *****************************************************************************/

extern const char adv_shader_bg_v[];
extern const char adv_shader_bg_f[];
extern const char adv_shader_plainsprites_v[];
extern const char adv_shader_plainsprites_f[];
extern const char adv_shader_splash_v[];
extern const char adv_shader_splash_f[];

/* compile and link one shader pair
 *****************************************************************************/

static GLuint adv_shaders_compile(const char *refname,const char *vsrc,const char *fsrc) {

  GLuint vshader=glCreateShader(GL_VERTEX_SHADER);
  GLuint fshader=glCreateShader(GL_FRAGMENT_SHADER);
  GLuint program=glCreateProgram();
  if (!program) return 0;

  glShaderSource(vshader,1,&vsrc,0);
  glCompileShader(vshader);
  glShaderSource(fshader,1,&fsrc,0);
  glCompileShader(fshader);
  glAttachShader(program,vshader);
  glAttachShader(program,fshader);

  glBindAttribLocation(program,ADV_VTXATTR_POSITION,"position");
  glBindAttribLocation(program,ADV_VTXATTR_COLOR,"color");
  glBindAttribLocation(program,ADV_VTXATTR_ROTATION,"rotation");
  glBindAttribLocation(program,ADV_VTXATTR_TEXPOSITION,"texposition");
  glBindAttribLocation(program,ADV_VTXATTR_SCALE,"scale");
  glBindAttribLocation(program,ADV_VTXATTR_OPACITY,"opacity");
  glBindAttribLocation(program,ADV_VTXATTR_XFORM,"xform");

  glLinkProgram(program);
  glDeleteShader(vshader);
  glDeleteShader(fshader);
  
  GLint status=0;
  glGetProgramiv(program,GL_LINK_STATUS,&status);
  if (status) return program;

  fprintf(stderr,"Failed to compile shader '%s'. Program log follows...\n",refname);
  GLint loga=0;
  glGetProgramiv(program,GL_INFO_LOG_LENGTH,&loga);
  if (loga>0) {
    if (loga<INT_MAX) loga++;
    GLchar *log=calloc(1,loga);
    if (log) {
      GLint logc=0;
      glGetProgramInfoLog(program,loga,&logc,log);
      if (logc<0) logc=0; else if (logc>=loga) logc=loga-1;
      while (logc&&((unsigned char)log[logc-1]<=0x20)) logc--;
      log[logc]=0;
      fprintf(stderr,"%.*s\n",logc,log);
      free(log);
    }
  }
  
  glDeleteProgram(program);
  return 0;
}

/* load shaders, main entry point
 *****************************************************************************/

int adv_shaders_load() {

  int screenw=0,screenh=0;
  //TODO if (pig_get_screen_size(&screenw,&screenh)<0) return -1;
  if ((screenw<1)||(screenh<1)) return -1;
  GLfloat nscreenx=-(GLfloat)ADV_SCREEN_W/screenw;
  GLfloat nscreeny= (GLfloat)ADV_SCREEN_H/screenh;
  GLfloat nscreenw=(ADV_SCREEN_W* 2.0)/screenw;
  GLfloat nscreenh=(ADV_SCREEN_H*-2.0)/screenh;

  if (!(adv_video.program_bg=adv_shaders_compile(
    "bg",adv_shader_bg_v,adv_shader_bg_f
  ))) return -1;
  glUseProgram(adv_video.program_bg);
  glUniform2f(glGetUniformLocation(adv_video.program_bg,"screenposition"),nscreenx,nscreeny);
  glUniform2f(glGetUniformLocation(adv_video.program_bg,"screensize"),nscreenw,nscreenh);
  adv_video.bg_lights_out_location=glGetUniformLocation(adv_video.program_bg,"lights_out");
  adv_video.bg_lightpos_location=glGetUniformLocation(adv_video.program_bg,"lightpos");

  if (!(adv_video.program_plainsprites=adv_shaders_compile(
    "plainsprites",adv_shader_plainsprites_v,adv_shader_plainsprites_f
  ))) return -1;
  glUseProgram(adv_video.program_plainsprites);
  glUniform2f(glGetUniformLocation(adv_video.program_plainsprites,"screenposition"),nscreenx,nscreeny);
  glUniform2f(glGetUniformLocation(adv_video.program_plainsprites,"screensize"),nscreenw,nscreenh);
  glUniform2f(glGetUniformLocation(adv_video.program_plainsprites,"gamescreensize"),ADV_SCREEN_W,ADV_SCREEN_H);
  
  if (!(adv_video.program_splash=adv_shaders_compile(
    "splash",adv_shader_splash_v,adv_shader_splash_f
  ))) return -1;

  if (glGetError()) return -1;
  return 0;
}
