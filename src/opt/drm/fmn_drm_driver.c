#include "fmn_drm_internal.h"

struct fmn_drm_driver fmn_drm={.fd=-1};

static int _drm_swap(const void *fb);

/* Cleanup.
 */
 
static void drm_fb_cleanup(struct drm_fb *fb) {
  if (fb->fbid) {
    drmModeRmFB(fmn_drm.fd,fb->fbid);
  }
}
 
void fmn_drm_quit() {

  // If waiting for a page flip, we must let it finish first.
  if ((fmn_drm.fd>=0)&&fmn_drm.flip_pending) {
    struct pollfd pollfd={.fd=fmn_drm.fd,.events=POLLIN|POLLERR|POLLHUP};
    if (poll(&pollfd,1,500)>0) {
      char dummy[64];
      if (read(fmn_drm.fd,dummy,sizeof(dummy))) ;
    }
  }
  
  if (fmn_drm.eglcontext) eglDestroyContext(fmn_drm.egldisplay,fmn_drm.eglcontext);
  if (fmn_drm.eglsurface) eglDestroySurface(fmn_drm.egldisplay,fmn_drm.eglsurface);
  if (fmn_drm.egldisplay) eglTerminate(fmn_drm.egldisplay);
  
  drm_fb_cleanup(fmn_drm.fbv+0);
  drm_fb_cleanup(fmn_drm.fbv+1);
  
  if (fmn_drm.crtc_restore&&(fmn_drm.fd>=0)) {
    drmModeCrtcPtr crtc=fmn_drm.crtc_restore;
    drmModeSetCrtc(
      fmn_drm.fd,crtc->crtc_id,crtc->buffer_id,
      crtc->x,crtc->y,&fmn_drm.connid,1,&crtc->mode
    );
    drmModeFreeCrtc(fmn_drm.crtc_restore);
  }
  
  if (fmn_drm.fd>=0) close(fmn_drm.fd);
  
  memset(&fmn_drm,0,sizeof(fmn_drm));
  fmn_drm.fd=-1;	
}

/* Init.
 */
 
int fmn_drm_init() {

  fmn_drm.fd=-1;
  fmn_drm.crtcunset=1;

  if (!drmAvailable()) {
    fprintf(stderr,"DRM not available.\n");
    return -1;
  }
  
  if (
    (drm_open_file()<0)||
    (drm_configure()<0)||
    (drm_init_gx()<0)||
  0) return -1;

  return 0;
}

/* Poll file.
 */
 
static void drm_cb_vblank(
  int fd,unsigned int seq,unsigned int times,unsigned int timeus,void *userdata
) {}
 
static void drm_cb_page1(
  int fd,unsigned int seq,unsigned int times,unsigned int timeus,void *userdata
) {
  fmn_drm.flip_pending=0;
}
 
static void drm_cb_page2(
  int fd,unsigned int seq,unsigned int times,unsigned int timeus,unsigned int ctrcid,void *userdata
) {
  drm_cb_page1(fd,seq,times,timeus,userdata);
}
 
static void drm_cb_seq(
  int fd,uint64_t seq,uint64_t timeus,uint64_t userdata
) {}
 
static int drm_poll_file(int to_ms) {
  struct pollfd pollfd={.fd=fmn_drm.fd,.events=POLLIN};
  if (poll(&pollfd,1,to_ms)<=0) return 0;
  drmEventContext ctx={
    .version=DRM_EVENT_CONTEXT_VERSION,
    .vblank_handler=drm_cb_vblank,
    .page_flip_handler=drm_cb_page1,
    .page_flip_handler2=drm_cb_page2,
    .sequence_handler=drm_cb_seq,
  };
  int err=drmHandleEvent(fmn_drm.fd,&ctx);
  if (err<0) return -1;
  return 0;
}

/* Swap.
 */
 
static int drm_swap_egl(uint32_t *fbid) { 
  eglSwapBuffers(fmn_drm.egldisplay,fmn_drm.eglsurface);
  
  struct gbm_bo *bo=gbm_surface_lock_front_buffer(fmn_drm.gbmsurface);
  if (!bo) return -1;
  
  int handle=gbm_bo_get_handle(bo).u32;
  struct drm_fb *fb;
  if (!fmn_drm.fbv[0].handle) {
    fb=fmn_drm.fbv;
  } else if (handle==fmn_drm.fbv[0].handle) {
    fb=fmn_drm.fbv;
  } else {
    fb=fmn_drm.fbv+1;
  }
  
  if (!fb->fbid) {
    int width=gbm_bo_get_width(bo);
    int height=gbm_bo_get_height(bo);
    int stride=gbm_bo_get_stride(bo);
    fb->handle=handle;
    if (drmModeAddFB(fmn_drm.fd,width,height,24,32,stride,fb->handle,&fb->fbid)<0) return -1;
    
    if (fmn_drm.crtcunset) {
      if (drmModeSetCrtc(
        fmn_drm.fd,fmn_drm.crtcid,fb->fbid,0,0,
        &fmn_drm.connid,1,&fmn_drm.mode
      )<0) {
        fprintf(stderr,"drmModeSetCrtc: %m\n");
        return -1;
      }
      fmn_drm.crtcunset=0;
    }
  }
  
  *fbid=fb->fbid;
  if (fmn_drm.bo_pending) {
    gbm_surface_release_buffer(fmn_drm.gbmsurface,fmn_drm.bo_pending);
  }
  fmn_drm.bo_pending=bo;
  
  return 0;
}

int fmn_drm_swap() {

  // There must be no more than one page flip in flight at a time.
  // If one is pending -- likely -- give it a chance to finish.
  if (fmn_drm.flip_pending) {
    if (drm_poll_file(20)<0) return -1;
    if (fmn_drm.flip_pending) {
      // Page flip didn't complete after a 20 ms delay? Drop the frame, no worries.
      return 0;
    }
  }
  fmn_drm.flip_pending=1;
  
  uint32_t fbid=0;
  if (drm_swap_egl(&fbid)<0) {
    fmn_drm.flip_pending=0;
    return -1;
  }
  
  if (drmModePageFlip(fmn_drm.fd,fmn_drm.crtcid,fbid,DRM_MODE_PAGE_FLIP_EVENT,0)<0) {
    fprintf(stderr,"drmModePageFlip: %m\n");
    fmn_drm.flip_pending=0;
    return -1;
  }

  return 0;
}

/* Trivial accessors.
 */
 
void fmn_drm_get_screen_size(int *w,int *h) {
  *w=fmn_drm.w;
  *h=fmn_drm.h;
}
