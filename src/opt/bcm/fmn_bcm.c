#include "fmn_bcm.h"
#include <bcm_host.h>
#include <EGL/egl.h>
#include <stdio.h>

/* Globals.
 */

static struct {
  DISPMANX_DISPLAY_HANDLE_T vcdisplay;
  DISPMANX_ELEMENT_HANDLE_T vcelement;
  DISPMANX_UPDATE_HANDLE_T vcupdate;
  EGL_DISPMANX_WINDOW_T eglwindow;
  EGLDisplay egldisplay;
  EGLSurface eglsurface;
  EGLContext eglcontext;
  EGLConfig eglconfig;
  int initstate;
  int screenw,screenh;
} fmn_bcm={0};

/* Init.
 */

int fmn_bcm_init() {
  if (fmn_bcm.initstate) return -1;
  memset(&fmn_bcm,0,sizeof(fmn_bcm));

  bcm_host_init();
  fmn_bcm.initstate=1;

  // We enforce a screen size sanity limit of 4096. Could be as high as 32767 if we felt like it.
  int screenw,screenh;
  graphics_get_display_size(0,&screenw,&screenh);
  if ((screenw<1)||(screenh<1)) { fmn_bcm_quit(); return -1; }
  if ((screenw>4096)||(screenh>4096)) { fmn_bcm_quit(); return -1; }

  if (!(fmn_bcm.vcdisplay=vc_dispmanx_display_open(0))) { fmn_bcm_quit(); return -1; }
  if (!(fmn_bcm.vcupdate=vc_dispmanx_update_start(0))) { fmn_bcm_quit(); return -1; }

  int logw=screenw-80;
  int logh=screenh-50;
  VC_RECT_T srcr={0,0,screenw<<16,screenh<<16};
  VC_RECT_T dstr={(screenw>>1)-(logw>>1),(screenh>>1)-(logh>>1),logw,logh};
  VC_DISPMANX_ALPHA_T alpha={DISPMANX_FLAGS_ALPHA_FIXED_ALL_PIXELS,0xffffffff};
  if (!(fmn_bcm.vcelement=vc_dispmanx_element_add(
    fmn_bcm.vcupdate,fmn_bcm.vcdisplay,1,&dstr,0,&srcr,DISPMANX_PROTECTION_NONE,&alpha,0,0
  ))) { fmn_bcm_quit(); return -1; }

  fmn_bcm.eglwindow.element=fmn_bcm.vcelement;
  fmn_bcm.eglwindow.width=screenw;
  fmn_bcm.eglwindow.height=screenh;

  if (vc_dispmanx_update_submit_sync(fmn_bcm.vcupdate)<0) { fmn_bcm_quit(); return -1; }

  static const EGLint eglattr[]={
    EGL_RED_SIZE,8,
    EGL_GREEN_SIZE,8,
    EGL_BLUE_SIZE,8,
    EGL_ALPHA_SIZE,0,
    EGL_DEPTH_SIZE,0,
    EGL_LUMINANCE_SIZE,EGL_DONT_CARE,
    EGL_SURFACE_TYPE,EGL_WINDOW_BIT,
    EGL_SAMPLES,1,
  EGL_NONE};
  static EGLint ctxattr[]={
    EGL_CONTEXT_CLIENT_VERSION,2,
  EGL_NONE};

  fmn_bcm.egldisplay=eglGetDisplay(EGL_DEFAULT_DISPLAY);
  if (eglGetError()!=EGL_SUCCESS) { fmn_bcm_quit(); return -1; }

  eglInitialize(fmn_bcm.egldisplay,0,0);
  if (eglGetError()!=EGL_SUCCESS) { fmn_bcm_quit(); return -1; }
  fmn_bcm.initstate=2;

  eglBindAPI(EGL_OPENGL_ES_API);

  EGLint configc=0;
  eglChooseConfig(fmn_bcm.egldisplay,eglattr,&fmn_bcm.eglconfig,1,&configc);
  if (eglGetError()!=EGL_SUCCESS) { fmn_bcm_quit(); return -1; }
  if (configc<1) { fmn_bcm_quit(); return -1; }

  fmn_bcm.eglsurface=eglCreateWindowSurface(fmn_bcm.egldisplay,fmn_bcm.eglconfig,&fmn_bcm.eglwindow,0);
  if (eglGetError()!=EGL_SUCCESS) { fmn_bcm_quit(); return -1; }
  fmn_bcm.initstate=3;

  fmn_bcm.eglcontext=eglCreateContext(fmn_bcm.egldisplay,fmn_bcm.eglconfig,0,ctxattr);
  if (eglGetError()!=EGL_SUCCESS) { fmn_bcm_quit(); return -1; }

  eglMakeCurrent(fmn_bcm.egldisplay,fmn_bcm.eglsurface,fmn_bcm.eglsurface,fmn_bcm.eglcontext);
  if (eglGetError()!=EGL_SUCCESS) { fmn_bcm_quit(); return -1; }

  eglSwapInterval(fmn_bcm.egldisplay,1);

  fmn_bcm.screenw=screenw;
  fmn_bcm.screenh=screenh;

  fprintf(stderr,"fmn_bcm_init ok\n");

  return 0;
}

/* Quit.
 */

void fmn_bcm_quit() {
  if (fmn_bcm.initstate>=3) {
    eglMakeCurrent(fmn_bcm.egldisplay,EGL_NO_SURFACE,EGL_NO_SURFACE,EGL_NO_CONTEXT);
    eglDestroySurface(fmn_bcm.egldisplay,fmn_bcm.eglsurface);
  }
  if (fmn_bcm.initstate>=2) {
    eglTerminate(fmn_bcm.egldisplay);
    eglReleaseThread();
  }
  if (fmn_bcm.initstate>=1) bcm_host_deinit();
  memset(&fmn_bcm,0,sizeof(fmn_bcm));
}

/* Swap buffers.
 */

int fmn_bcm_swap() {
  if (!fmn_bcm.initstate) return -1;
  eglSwapBuffers(fmn_bcm.egldisplay,fmn_bcm.eglsurface);
  return 0;
}

/* Trivial accessors.
 */

int fmn_bcm_get_width() {
  return fmn_bcm.screenw;
}

int fmn_bcm_get_height() {
  return fmn_bcm.screenh;
}
