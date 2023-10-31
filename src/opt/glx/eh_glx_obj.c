#include "eh_glx_internal.h"
#include "adv.h"
#include "input/adv_input.h"

/* Delete.
 */

static void _glx_del(struct eh_video_driver *driver) {
  if (DRIVER->dpy) XCloseDisplay(DRIVER->dpy);
}

/* New.
 */

static int _glx_init(struct eh_video_driver *driver,const struct eh_video_setup *setup) {

  if (eh_glx_init_start(driver,setup)<0) return -1;
  if (eh_glx_init_opengl(driver,setup)<0) return -1;
  if (eh_glx_init_finish(driver,setup)<0) return -1;

  DRIVER->keyboard_devid=adv_input_devid_next();
  adv_input_connect(DRIVER->keyboard_devid,0,0,"System Keyboard",15);

  return 0;
}

/* Fullscreen.
 */

static void _glx_set_fullscreen(struct eh_video_driver *driver,int state) {
  state=state?1:0;
  if (state==driver->fullscreen) return;
  XEvent evt={
    .xclient={
      .type=ClientMessage,
      .message_type=DRIVER->atom__NET_WM_STATE,
      .send_event=1,
      .format=32,
      .window=DRIVER->win,
      .data={.l={
        state,
        DRIVER->atom__NET_WM_STATE_FULLSCREEN,
      }},
    }
  };
  XSendEvent(DRIVER->dpy,RootWindow(DRIVER->dpy,DRIVER->screen),0,SubstructureNotifyMask|SubstructureRedirectMask,&evt);
  XFlush(DRIVER->dpy);
  driver->fullscreen=state;
}

/* Screensaver.
 */
 
static void _glx_suppress_screensaver(struct eh_video_driver *driver) {
  if (DRIVER->screensaver_suppressed) return;
  XForceScreenSaver(DRIVER->dpy,ScreenSaverReset);
  DRIVER->screensaver_suppressed=1;
}

/* GX frame control.
 */

static int _glx_begin(struct eh_video_driver *driver) {
  DRIVER->screensaver_suppressed=0;
  glXMakeCurrent(DRIVER->dpy,DRIVER->win,DRIVER->ctx);
  glViewport(0,0,driver->w,driver->h);
  return 0;
}

static int _glx_end(struct eh_video_driver *driver) {
  glXSwapBuffers(DRIVER->dpy,DRIVER->win);
  return 0;
}

/* TTAQ glue to provide a static global interface.
 */
 
static struct eh_video_driver *eh_glx_global=0;

int glx_init() {
  if (eh_glx_global) return -1;
  if (!(eh_glx_global=calloc(1,sizeof(struct eh_video_driver_glx)))) return -1;
  struct eh_video_setup setup={
    .w=1280,//TODO default dimensions?
    .h=704,
    .fullscreen=0,
    .title="Tag Team Adventure Quest",
    .iconrgba=0,//TODO app icon
    .iconw=0,
    .iconh=0,
  };
  if (_glx_init(eh_glx_global,&setup)<0) {
    _glx_del(eh_glx_global);
    free(eh_glx_global);
    eh_glx_global=0;
    return -1;
  }
  return 0;
}

void glx_quit() {
  if (!eh_glx_global) return;
  _glx_del(eh_glx_global);
  free(eh_glx_global);
  eh_glx_global=0;
}

int glx_update() {
  _glx_suppress_screensaver(eh_glx_global);//TODO connect this to joystick events; we shouldn't force no screensaver at all times
  return _glx_update(eh_glx_global);
}

int glx_begin() {
  return _glx_begin(eh_glx_global);
}

int glx_end() {
  return _glx_end(eh_glx_global);
}

void glx_get_screen_size(int *w,int *h) {
  *w=eh_glx_global->w;
  *h=eh_glx_global->h;
}
