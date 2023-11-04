/* fmn_drm.h
 * Linux Direct Rendering Manager.
 */
 
#ifndef FMN_DRM_H
#define FMN_DRM_H

void fmn_drm_quit();
int fmn_drm_init();
int fmn_drm_swap();

void fmn_drm_get_screen_size(int *w,int *h);

#endif
