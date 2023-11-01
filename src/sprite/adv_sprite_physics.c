#include "adv.h"
#include "adv_sprite.h"
#include "adv_sprclass.h"
#include "game/adv_map.h"
#include "res/adv_res.h"
#include "sprite/class/adv_sprite_hero.h"

/* Compose new-coverage rectangle.
 * If (dx,dy)==(0,0), return the sprite's natural area.
 *****************************************************************************/
 
static void adv_sprite_get_new_coverage(int *x,int *y,int *w,int *h,struct adv_sprite *spr,int dx,int dy) {
  if (dx<0) {
    *x=spr->x-(ADV_TILE_W>>1)+dx;
    *y=spr->y-(ADV_TILE_H>>1);
    *w=-dx;
    *h=ADV_TILE_H;
  } else if (dx>0) {
    *x=spr->x+(ADV_TILE_W>>1);
    *y=spr->y-(ADV_TILE_H>>1);
    *w=dx;
    *h=ADV_TILE_H;
  } else if (dy<0) {
    *x=spr->x-(ADV_TILE_W>>1);
    *y=spr->y-(ADV_TILE_H>>1)+dy;
    *w=ADV_TILE_W;
    *h=-dy;
  } else if (dy>0) {
    *x=spr->x-(ADV_TILE_W>>1);
    *y=spr->y+(ADV_TILE_H>>1);
    *w=ADV_TILE_W;
    *h=dy;
  } else {
    *x=spr->x-(ADV_TILE_W>>1);
    *y=spr->y-(ADV_TILE_H>>1);
    *w=ADV_TILE_W;
    *h=ADV_TILE_H;
  }
}
 
/* move
 *****************************************************************************/
 
int adv_sprite_move(struct adv_sprite *spr,int dx,int dy,int pushing) {
  if (!spr) return -1;
  if (dx&&dy) {
    int rx=adv_sprite_move(spr,dx,0,pushing); if (rx<0) return rx;
    int ry=adv_sprite_move(spr,0,dy,pushing); if (ry<0) return ry;
    return rx+ry;
  }
  if (!dx&&!dy) return 0;
  int possible=adv_sprite_premove(spr,dx,dy);
  if (possible<=0) return possible;
  if (dx<0) spr->x-=possible;
  else if (dx>0) spr->x+=possible;
  else if (dy<0) spr->y-=possible;
  else if (dy>0) spr->y+=possible;
  
  if (pushing) {
    if ((dx<0)&&(possible==-dx)) pushing=0;
    else if ((dx>0)&&(possible==dx)) pushing=0;
    else if ((dy<0)&&(possible==-dy)) pushing=0;
    else if ((dy>0)&&(possible==dy)) pushing=0;
    if (pushing) {
      //TODO push sprites
      // Now that I think about it, we might want to check this earlier in the process...
    }
  }
  
  return possible;
}

/* premove
 *****************************************************************************/
 
int adv_sprite_premove(struct adv_sprite *spr,int dx,int dy) {

  if (!spr) return -1;
  if (dx&&dy) {
    int rx=adv_sprite_premove(spr,dx,0); if (rx<0) return rx;
    int ry=adv_sprite_premove(spr,0,dy); if (ry<0) return ry;
    return rx+ry;
  }
  if (!dx&&!dy) return 0;
  int adx=dx; if (adx<0) adx=-adx;
  int ady=dy; if (ady<0) ady=-ady;
  
  int x,y,w,h; // area covered by potential move...
  adv_sprite_get_new_coverage(&x,&y,&w,&h,spr,dx,dy);
  int x0=spr->x-(ADV_TILE_W>>1),y0=spr->y-(ADV_TILE_H>>1); // natural position (top-left)
  
  /* Edges. */
  if (spr->collide_edges) {
    if ((dx<0)&&(x<0)) {
      if (x0<=0) return 0;
      dx=-x0; adx=x0; x=0; w=x0;
    }
    if ((dy<0)&&(y<0)) {
      if (y0<=0) return 0;
      dy=-y0; ady=y0; y=0; h=y0;
    }
    if ((dx>0)&&(x+w>ADV_SCREEN_W)) {
      if (x0+ADV_TILE_W>=ADV_SCREEN_W) return 0;
      dx=adx=w=ADV_SCREEN_W-ADV_TILE_W-x0;
    }
    if ((dy>0)&&(y+h>ADV_SCREEN_H)) {
      if (y0+ADV_TILE_H>=ADV_SCREEN_H) return 0;
      dy=ady=h=ADV_SCREEN_H-ADV_TILE_H-y0;
    }
  }
  
  /* Map. */
  if (spr->collide_map_positive||spr->collide_map_negative) {
    int cola=x/ADV_TILE_W; if (cola<0) cola=0;
    int rowa=y/ADV_TILE_H; if (rowa<0) rowa=0;
    int colz=(x+w-1)/ADV_TILE_W; if (colz>=ADV_MAP_W) colz=ADV_MAP_W-1;
    int rowz=(y+h-1)/ADV_TILE_H; if (rowz>=ADV_MAP_H) rowz=ADV_MAP_H-1;
    if (cola>colz) colz=cola;
    if (rowa>rowz) rowz=rowa;
    if (dx<0) {
      int col; for (col=colz;col>=cola;col--) {
        int row; for (row=rowa;row<=rowz;row++) {
          if (adv_map_cell_is_solid(adv_map,col,row,spr->collide_map_positive,spr->collide_map_negative)) {
            int colright=(col+1)*ADV_TILE_W;
            if (x0<=colright) return 0;
            dx=colright-x0; adx=x0-colright; x=colright; w=adx;
            goto _done_map_;
          }
        }
      }
    } else if (dx>0) {
      int col; for (col=cola;col<=colz;col++) {
        int row; for (row=rowa;row<=rowz;row++) {
          if (adv_map_cell_is_solid(adv_map,col,row,spr->collide_map_positive,spr->collide_map_negative)) {
            int colleft=col*ADV_TILE_W;
            if (x0+ADV_TILE_W>=colleft) return 0;
            dx=adx=w=colleft-ADV_TILE_W-x0;
            goto _done_map_;
          }
        }
      }
    } else if (dy<0) {
      int row; for (row=rowz;row>=rowa;row--) {
        int col; for (col=cola;col<=colz;col++) {
          if (adv_map_cell_is_solid(adv_map,col,row,spr->collide_map_positive,spr->collide_map_negative)) {
            int rowbottom=(row+1)*ADV_TILE_H;
            if (y0<=rowbottom) return 0;
            dy=rowbottom-y0; ady=y0-rowbottom; y=rowbottom; h=ady;
            goto _done_map_;
          }
        }
      }
    } else if (dy>0) {
      int row; for (row=rowa;row<=rowz;row++) {
        int col; for (col=cola;col<=colz;col++) {
          if (adv_map_cell_is_solid(adv_map,col,row,spr->collide_map_positive,spr->collide_map_negative)) {
            int rowtop=row*ADV_TILE_H;
            if (y0+ADV_TILE_H>=rowtop) return 0;
            dy=ady=h=rowtop-ADV_TILE_H-y0;
            goto _done_map_;
          }
        }
      }
    }
   _done_map_:;
  }
  
  /* Solid sprites. */
  if (spr->collide_solids) {
    // Rather than calcuating the bounds of each solid sprite, get bounds in which to compare solid sprites' simple position.
    // We can get away with this because all sprites are uniform size.
    int left=x-(ADV_TILE_W>>1);
    int top=y-(ADV_TILE_H>>1);
    int right=x+w+(ADV_TILE_W>>1);
    int bottom=y+h+(ADV_TILE_H>>1);
    int i; for (i=0;i<adv_sprgrp_solid->sprc;i++) {
      struct adv_sprite *qspr=adv_sprgrp_solid->sprv[i];
      if (qspr==spr) continue;
      if (qspr->x<=left) continue;
      if (qspr->y<=top) continue;
      if (qspr->x>=right) continue;
      if (qspr->y>=bottom) continue;
      if (dx<0) {
        int nx=qspr->x+(ADV_TILE_W>>1);
        if (x0<=nx) return 0;
        dx=nx-x0; adx=x0-nx; x=nx; w=adx;
        left=x-(ADV_TILE_W>>1);
        right=x+(ADV_TILE_W>>1);
      } else if (dx>0) {
        int nx=qspr->x-(ADV_TILE_W>>1)-ADV_TILE_W;
        if (x0>=nx) return 0;
        dx=adx=w=nx-x0; x=x0+ADV_TILE_W;
        left=x-(ADV_TILE_W>>1);
        right=x+(ADV_TILE_W>>1);
      } else if (dy<0) {
        int ny=qspr->y+(ADV_TILE_H>>1);
        if (y0<=ny) return 0;
        dy=ny-y0; ady=y0-ny; y=ny; w=ady;
        top=y-(ADV_TILE_H>>1);
        bottom=y+(ADV_TILE_H>>1);
      } else if (dy>0) {
        int ny=qspr->y-(ADV_TILE_H>>1)-ADV_TILE_H;
        if (y0>=ny) return 0;
        dy=ady=h=ny-y0; y=y0+ADV_TILE_H;
        top=y-(ADV_TILE_H>>1);
        bottom=y+(ADV_TILE_H>>1);
      }
    }
  }
  
  return adx+ady;
}

/* hurt
 *****************************************************************************/
 
int adv_sprite_hurt_default(struct adv_sprite *spr) {
  if (!spr) return -1;
  if (!adv_sprgrp_has(adv_sprgrp_fragile,spr)) return 0;
  ttaq_audio_play_sound(ADV_SOUND_STRIKE);
  if (adv_sprite_create(0,90,spr->x,spr->y)<0) return -1;
  if (adv_sprite_create(0,10,spr->x,spr->y)<0) return -1;
  if (adv_sprite_kill(spr)<0) return -1;
  return 0;
}
 
int adv_sprite_hurt(struct adv_sprite *spr) {
  if (!spr) return -1;
  if (spr->sprclass&&spr->sprclass->hurt) return spr->sprclass->hurt(spr);
  return adv_sprite_hurt_default(spr);
}
