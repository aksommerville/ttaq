/* eh_glx.h
 * X11 and GLX.
 * Link: -lX11 -lGL -lGLX
 * With USE_xinerama: -lXinerama
 *
 * Copied from my recent project ra3, and hammered into shape for ttaq.
 */
 
#ifndef EH_GLX_H
#define EH_GLX_H

int glx_init();
void glx_quit();
int glx_update();
int glx_begin();
int glx_end();

void glx_get_screen_size(int *w,int *h);

#endif
