#include "ttaq_synth_internal.h"
#include "res/adv_res.h"

/* Simple voice mode for pure sine waves.
 * Bendable pitch, no modulation.
 */
 
static void ttaq_voice_update_sine(float *v,int c,struct ttaq_synth *synth,struct ttaq_voice *voice) {
  if (ttaq_env_is_finished(&voice->level)) {
    voice->defunct=1;
    return;
  }
  for (;c-->0;v++) {
    (*v)+=sinf(voice->p)*ttaq_env_next(&voice->level);
    voice->p+=ttaq_env_next(&voice->pd);
  }
  while (voice->p>M_PI) voice->p-=M_PI*2.0f;
}

/* Relative FM, typical for musical instruments.
 */

static void ttaq_voice_update_relfm(float *v,int c,struct ttaq_synth *synth,struct ttaq_voice *voice) {
  if (ttaq_env_is_finished(&voice->level)) {
    voice->defunct=1;
    return;
  }
  for (;c-->0;v++) {
    (*v)+=sinf(voice->p)*ttaq_env_next(&voice->level);
    float carpd=ttaq_env_next(&voice->pd);
    voice->modp+=carpd*voice->modrate;
    float mod=sinf(voice->modp)*ttaq_env_next(&voice->modrange);
    voice->p+=carpd+carpd*mod;
  }
  while (voice->p>M_PI) voice->p-=M_PI*2.0f;
  while (voice->modp>M_PI) voice->modp-=M_PI*2.0f;
}

/* Absolute FM, usually the modulator has a very low rate to produce an audible pitch bend effect.
 */

static void ttaq_voice_update_absfm(float *v,int c,struct ttaq_synth *synth,struct ttaq_voice *voice) {
  if (ttaq_env_is_finished(&voice->level)) {
    voice->defunct=1;
    return;
  }
  for (;c-->0;v++) {
    (*v)+=sinf(voice->p)*ttaq_env_next(&voice->level);
    float carpd=ttaq_env_next(&voice->pd);
    voice->modp+=voice->modrate;
    float mod=sinf(voice->modp)*ttaq_env_next(&voice->modrange);
    voice->p+=carpd+carpd*mod;
  }
  while (voice->p>M_PI) voice->p-=M_PI*2.0f;
  while (voice->modp>M_PI) voice->modp-=M_PI*2.0f;
}

/* Specific sound effects.
 */
 
static void ttaq_sound_begin_SWORD(struct ttaq_voice *voice,struct ttaq_synth *synth) {
  voice->modrate=3.7f;
  ttaq_env_init_generic(&voice->pd,synth,
    (700.0f*M_PI*2.0f)/(float)synth->rate,
    50,
    (500.0f*M_PI*2.0f)/(float)synth->rate,
    100,
    (900.0f*M_PI*2.0f)/(float)synth->rate,
    0,
    200,
    (400.0f*M_PI*2.0f)/(float)synth->rate
  );
  ttaq_env_init_generic(&voice->modrange,synth,
    5.0f,
    10,
    0.0f,
    10,
    0.0f,
    0,
    200,
    5.0f
  );
  ttaq_env_init_level(&voice->level,synth,0x60,10,0x20,0,200);
  voice->update=ttaq_voice_update_relfm;
}
 
static void ttaq_sound_begin_STRIKE(struct ttaq_voice *voice,struct ttaq_synth *synth) {
  voice->modrate=0.5f;
  ttaq_env_init_generic(&voice->pd,synth,
    (300.0f*M_PI*2.0f)/(float)synth->rate,
    1,
    (300.0f*M_PI*2.0f)/(float)synth->rate,
    1,
    (300.0f*M_PI*2.0f)/(float)synth->rate,
    0,
    350,
    (80.0f*M_PI*2.0f)/(float)synth->rate
  );
  ttaq_env_init_generic(&voice->modrange,synth,
    0.0f,
    50,
    2.0f,
    1,
    2.0f,
    0,
    350,
    0.0f
  );
  ttaq_env_init_level(&voice->level,synth,0x60,20,0x80,0,300);
  voice->update=ttaq_voice_update_relfm;
}
 
static void ttaq_sound_begin_ARROW(struct ttaq_voice *voice,struct ttaq_synth *synth) {
  voice->modrate=2.5f;
  ttaq_env_init_generic(&voice->pd,synth,
    (900.0f*M_PI*2.0f)/(float)synth->rate,
    50,
    (400.0f*M_PI*2.0f)/(float)synth->rate,
    100,
    (1300.0f*M_PI*2.0f)/(float)synth->rate,
    0,
    200,
    (800.0f*M_PI*2.0f)/(float)synth->rate
  );
  ttaq_env_init_generic(&voice->modrange,synth,
    5.0f,
    10,
    5.0f,
    10,
    5.0f,
    0,
    200,
    0.0f
  );
  ttaq_env_init_level(&voice->level,synth,0x60,10,0x20,0,200);
  voice->update=ttaq_voice_update_relfm;
}
 
static void ttaq_sound_begin_GADGET(struct ttaq_voice *voice,struct ttaq_synth *synth) {
  voice->modrate=9.2f;
  ttaq_env_init_ramp_hz(&voice->pd,synth,100.0f,150,400.0f);
  ttaq_env_init_ramp(&voice->modrange,synth,1.0f,150,1.0f);
  ttaq_env_init_level(&voice->level,synth,0x60,20,0x80,0,130);
  voice->update=ttaq_voice_update_relfm;
}
 
static void ttaq_sound_begin_GADGETNO(struct ttaq_voice *voice,struct ttaq_synth *synth) {
  voice->modrate=0.77f;
  ttaq_env_init_ramp_hz(&voice->pd,synth,200.0f,150,50.0f);
  ttaq_env_init_ramp(&voice->modrange,synth,1.0f,150,1.0f);
  ttaq_env_init_level(&voice->level,synth,0x60,20,0x80,0,130);
  voice->update=ttaq_voice_update_relfm;
}
 
static void ttaq_sound_begin_GADGETYES(struct ttaq_voice *voice,struct ttaq_synth *synth) {
  voice->modrate=1.5f;
  ttaq_env_init_ramp_hz(&voice->pd,synth,300.0f,150,700.0f);
  ttaq_env_init_ramp(&voice->modrange,synth,2.0f,150,0.5f);
  ttaq_env_init_level(&voice->level,synth,0x40,10,0x80,0,140);
  voice->update=ttaq_voice_update_relfm;
}
 
static void ttaq_sound_begin_REVIVE(struct ttaq_voice *voice,struct ttaq_synth *synth) {
  voice->modrate=2.0f;
  ttaq_env_init_ramp_hz(&voice->pd,synth,500.0f,150,750.0f);
  ttaq_env_init_generic(&voice->modrange,synth,0.0f,50,2.0f,100,0.0f,0,1,0.0f);
  ttaq_env_init_level(&voice->level,synth,0x40,10,0x80,0,140);
  voice->update=ttaq_voice_update_relfm;
}
 
static void ttaq_sound_begin_RESCUE(struct ttaq_voice *voice,struct ttaq_synth *synth) {
  voice->modrate=1.0f;
  ttaq_env_init_generic(&voice->pd,synth,
    (440.0f*M_PI*2.0f)/(float)synth->rate,
    200,
    (440.0f*M_PI*2.0f)/(float)synth->rate,
    10,
    (660.0f*M_PI*2.0f)/(float)synth->rate,
    0,
    400,
    (660.0f*M_PI*2.0f)/(float)synth->rate
  );
  ttaq_env_init_constant(&voice->modrange,1.0f);
  ttaq_env_init_generic(&voice->level,synth,
    0.000f,50,0.200f,200,0.400f,0,400,0.0f
  );
  voice->update=ttaq_voice_update_relfm;
}
 
static void ttaq_sound_begin_CHANGECTL(struct ttaq_voice *voice,struct ttaq_synth *synth) {
  voice->modrate=0.5f;
  ttaq_env_init_ramp_hz(&voice->pd,synth,300.0f,150,450.0f);
  ttaq_env_init_ramp(&voice->modrange,synth,0.0f,150,1.0f);
  ttaq_env_init_level(&voice->level,synth,0x40,10,0x80,0,140);
  voice->update=ttaq_voice_update_relfm;
}
 
static void ttaq_sound_begin_SWITCH(struct ttaq_voice *voice,struct ttaq_synth *synth) {
  voice->modrate=3.8f;
  ttaq_env_init_ramp_hz(&voice->pd,synth,200.0f,150,190.0f);
  ttaq_env_init_ramp(&voice->modrange,synth,7.0f,150,6.0f);
  ttaq_env_init_level(&voice->level,synth,0x50,5,0x18,0,140);
  voice->update=ttaq_voice_update_relfm;
}
 
static void ttaq_sound_begin_HAT(struct ttaq_voice *voice,struct ttaq_synth *synth) {
  voice->modrate=1.8f;
  ttaq_env_init_ramp_hz(&voice->pd,synth,1200.0f,150,1900.0f);
  ttaq_env_init_ramp(&voice->modrange,synth,1.0f,150,6.0f);
  ttaq_env_init_level(&voice->level,synth,0x18,5,0x18,0,140);
  voice->update=ttaq_voice_update_relfm;
}
 
static void ttaq_sound_begin_KICK(struct ttaq_voice *voice,struct ttaq_synth *synth) {
  voice->modrate=1.0f;
  ttaq_env_init_ramp_hz(&voice->pd,synth,130.0f,200,20.0f);
  ttaq_env_init_constant(&voice->modrange,1.0f);
  ttaq_env_init_level(&voice->level,synth,0x40,10,0x80,0,200);
  voice->update=ttaq_voice_update_relfm;
}
 
static void ttaq_sound_begin_RIDE(struct ttaq_voice *voice,struct ttaq_synth *synth) {
  voice->modrate=3.7f;
  ttaq_env_init_generic(&voice->pd,synth,
    (1000.0f*M_PI*2.0f)/(float)synth->rate,
    50,
    (1000.0f*M_PI*2.0f)/(float)synth->rate,
    100,
    (1000.0f*M_PI*2.0f)/(float)synth->rate,
    0,
    200,
    (1000.0f*M_PI*2.0f)/(float)synth->rate
  );
  ttaq_env_init_generic(&voice->modrange,synth,
    5.0f,
    10,
    5.0f,
    10,
    5.0f,
    0,
    500,
    3.0f
  );
  ttaq_env_init_level(&voice->level,synth,0x18,10,0x40,0,200);
  voice->update=ttaq_voice_update_relfm;
}
 
static void ttaq_sound_begin_SNARE(struct ttaq_voice *voice,struct ttaq_synth *synth) {
  voice->modrate=4.7f;
  ttaq_env_init_ramp_hz(&voice->pd,synth,800.0f,200,750.0f);
  ttaq_env_init_ramp(&voice->modrange,synth,5.0f,200,3.0f);
  ttaq_env_init_level(&voice->level,synth,0x40,10,0x40,0,200);
  voice->update=ttaq_voice_update_relfm;
}
 
static void ttaq_sound_begin_MENUYES(struct ttaq_voice *voice,struct ttaq_synth *synth) {
  voice->modrate=2.0f;
  ttaq_env_init_ramp_hz(&voice->pd,synth,600.0f,200,800.0f);
  ttaq_env_init_ramp(&voice->modrange,synth,1.0f,200,2.0f);
  ttaq_env_init_level(&voice->level,synth,0x50,10,0x40,0,200);
  voice->update=ttaq_voice_update_relfm;
}
 
static void ttaq_sound_begin_MENUNO(struct ttaq_voice *voice,struct ttaq_synth *synth) {
  voice->modrate=1.5f;
  ttaq_env_init_ramp_hz(&voice->pd,synth,400.0f,200,200.0f);
  ttaq_env_init_ramp(&voice->modrange,synth,1.0f,200,1.0f);
  ttaq_env_init_level(&voice->level,synth,0x70,10,0x10,0,200);
  voice->update=ttaq_voice_update_relfm;
}

/* Begin sound effect.
 */
 
void ttaq_synth_play_sound(struct ttaq_synth *synth,int soundid,float level) {
  struct ttaq_voice *voice=ttaq_voice_new(synth);
  if (!voice) return;
  voice->p=0.0f;
  voice->modp=0.0;
  voice->defunct=0;
  switch (soundid) {
    #define _(tag) case ADV_SOUND_##tag: ttaq_sound_begin_##tag(voice,synth); break;
    _(SWORD)
    _(STRIKE)
    _(ARROW)
    _(GADGET)
    _(GADGETNO)
    _(GADGETYES)
    _(REVIVE)
    _(RESCUE)
    _(CHANGECTL)
    _(SWITCH)
    _(HAT)
    _(KICK)
    _(RIDE)
    _(SNARE)
    _(MENUYES)
    _(MENUNO)
    #undef _
    default: {
        fprintf(stderr,"Sound effect %d not configured.\n",soundid);
        voice->defunct=1;
        voice->update=0;
      }
  }
  ttaq_env_trim(&voice->level,level);
}

/* Begin tuned note.
 */
 
void ttaq_synth_play_note(struct ttaq_synth *synth,int instrumentid,float normrate,float level,int ttl) {
  //fprintf(stderr,"%s %d %f %d\n",__func__,instrumentid,normrate,ttl);
  struct ttaq_voice *voice=ttaq_voice_new(synth);
  if (!voice) return;
  voice->p=0.0f;
  voice->modp=0.0f;
  voice->defunct=0;
  if (ttl<=0) voice->autorelease=synth->rate>>2;
  else voice->autorelease=ttl;
  switch (instrumentid) {
  
    case 4: { // lead
        voice->modrate=1.0f;
        ttaq_env_init_constant(&voice->pd,normrate);
        ttaq_env_init_generic(&voice->modrange,synth,1.0f,40,1.5f,100,1.5f,1,400,0.5f);
        ttaq_env_init_level(&voice->level,synth,0x60,15,0x30,1,600);
        voice->update=ttaq_voice_update_relfm;
      } break;
  
    case 5: { // bass
        voice->modrate=0.5f;
        ttaq_env_init_constant(&voice->pd,normrate);
        ttaq_env_init_generic(&voice->modrange,synth,0.5f,40,2.0f,100,1.0f,1,400,0.5f);
        ttaq_env_init_level(&voice->level,synth,0x40,20,0x50,1,200);
        voice->update=ttaq_voice_update_relfm;
      } break;
  
    case 6: { // pad
        voice->modrate=1.0f;
        ttaq_env_init_constant(&voice->pd,normrate);
        ttaq_env_init_constant(&voice->modrange,3.0f);
        ttaq_env_init_level(&voice->level,synth,0x20,80,0x80,1,450);
        voice->update=ttaq_voice_update_relfm;
      } break;
  
    default: { // shouldn't happen
        voice->modrate=1.0f;
        ttaq_env_init_constant(&voice->pd,normrate);
        ttaq_env_init_constant(&voice->modrange,1.0f);
        ttaq_env_init_level(&voice->level,synth,0x20,40,0x50,1,200);
        voice->update=ttaq_voice_update_relfm;
      }
  }
  ttaq_env_trim(&voice->level,level);
}
        

/* Release.
 */
 
void ttaq_voice_release(struct ttaq_voice *voice) {
  ttaq_env_release(&voice->pd);
  ttaq_env_release(&voice->modrange);
  ttaq_env_release(&voice->level);
}
