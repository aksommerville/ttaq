#include "eh_glx_internal.h"
#include "adv.h"
#include "input/adv_input.h"
#include "video/adv_video.h"

/* Key press, release, or repeat.
 */
 
static int eh_glx_evt_key(struct eh_video_driver *driver,XKeyEvent *evt,int value) {
  KeySym keysym=XkbKeycodeToKeysym(DRIVER->dpy,evt->keycode,0,0);
  if (keysym) {
    int keycode=eh_glx_usb_usage_from_keysym((int)keysym);
    if (keycode) {
      return adv_input_event(DRIVER->keyboard_devid,keycode,value);
    }
  }
  return 0;
}

/* Client message.
 */
 
static int eh_glx_evt_client(struct eh_video_driver *driver,XClientMessageEvent *evt) {
  if (evt->message_type==DRIVER->atom_WM_PROTOCOLS) {
    if (evt->format==32) {
      if (evt->data.l[0]==DRIVER->atom_WM_DELETE_WINDOW) {
        adv_input_request_quit();
      }
    }
  }
  return 0;
}

/* Configuration event (eg resize).
 */
 
static int eh_glx_evt_configure(struct eh_video_driver *driver,XConfigureEvent *evt) {
  int nw=evt->width,nh=evt->height;
  if ((nw!=driver->w)||(nh!=driver->h)) {
    driver->w=nw;
    driver->h=nh;
    adv_video_resized(nw,nh);
  }
  return 0;
}

/* Focus.
 */
 
static int eh_glx_evt_focus(struct eh_video_driver *driver,XFocusInEvent *evt,int value) {
  if (value==DRIVER->focus) return 0;
  DRIVER->focus=value;
  // App isn't built to react to this. Whatever.
  return 0;
}

/* Process one event.
 */
 
static int eh_glx_receive_event(struct eh_video_driver *driver,XEvent *evt) {
  if (!evt) return -1;
  switch (evt->type) {
  
    case KeyPress: return eh_glx_evt_key(driver,&evt->xkey,1);
    case KeyRelease: return eh_glx_evt_key(driver,&evt->xkey,0);
    case KeyRepeat: return eh_glx_evt_key(driver,&evt->xkey,2);
    
    case ClientMessage: return eh_glx_evt_client(driver,&evt->xclient);
    
    case ConfigureNotify: return eh_glx_evt_configure(driver,&evt->xconfigure);
    
    case FocusIn: return eh_glx_evt_focus(driver,&evt->xfocus,1);
    case FocusOut: return eh_glx_evt_focus(driver,&evt->xfocus,0);
    
  }
  return 0;
}

/* Update.
 */
 
int _glx_update(struct eh_video_driver *driver) {
  int evtc=XEventsQueued(DRIVER->dpy,QueuedAfterFlush);
  while (evtc-->0) {
    XEvent evt={0};
    XNextEvent(DRIVER->dpy,&evt);
    if ((evtc>0)&&(evt.type==KeyRelease)) {
      XEvent next={0};
      XNextEvent(DRIVER->dpy,&next);
      evtc--;
      if ((next.type==KeyPress)&&(evt.xkey.keycode==next.xkey.keycode)&&(evt.xkey.time>=next.xkey.time-EH_GLX_KEY_REPEAT_INTERVAL)) {
        evt.type=KeyRepeat;
        if (eh_glx_receive_event(driver,&evt)<0) return -1;
      } else {
        if (eh_glx_receive_event(driver,&evt)<0) return -1;
        if (eh_glx_receive_event(driver,&next)<0) return -1;
      }
    } else {
      if (eh_glx_receive_event(driver,&evt)<0) return -1;
    }
  }
  return 0;
}
