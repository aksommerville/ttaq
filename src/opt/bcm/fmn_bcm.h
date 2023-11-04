/* fmn_bcm.h
 * Interface to Broadcom video, for Raspberry Pi.
 * This uses EGL to set up an OpenGL ES 2.x context.
 * Only our video unit should use this unit.
 */

#ifndef FMN_BCM_H
#define FMN_BCM_H

int fmn_bcm_init();
void fmn_bcm_quit();
int fmn_bcm_swap();
int fmn_bcm_get_width();
int fmn_bcm_get_height();

#endif
