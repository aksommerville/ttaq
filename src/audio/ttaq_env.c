#include "ttaq_synth_internal.h"

/* Init a noop envelope that emits a constant.
 */
 
void ttaq_env_init_constant(struct ttaq_env *env,float v) {
  env->hold=1;
  env->stage=3;
  env->level=v;
  env->level_fin=v;
}

/* Init special for level envelopes.
 */

void ttaq_env_init_level(
  struct ttaq_env *env,
  const struct ttaq_synth *synth,
  uint8_t atkl,
  uint8_t atkt_ms,
  uint8_t rel_decl,
  uint8_t sustain,
  uint16_t rlst_ms
) {
  ttaq_env_init_generic(env,synth,0.0f,atkt_ms,atkl/256.0f,atkt_ms<<1,(atkl*rel_decl)/65536.0f,sustain,rlst_ms,0.0f);
}

/* Init special for simple ramps.
 */
 
void ttaq_env_init_ramp(
  struct ttaq_env *env,
  const struct ttaq_synth *synth,
  float levela,
  uint16_t dur_ms,
  float levelz
) {
  return ttaq_env_init_generic(env,synth,levela,0,levela,0,levela,0,dur_ms,levelz);
}
 
void ttaq_env_init_ramp_hz(
  struct ttaq_env *env,
  const struct ttaq_synth *synth,
  float levela,
  uint16_t dur_ms,
  float levelz
) {
  levela=(levela*M_PI*2.0f)/synth->rate;
  levelz=(levelz*M_PI*2.0f)/synth->rate;
  return ttaq_env_init_generic(env,synth,levela,0,levela,0,levela,0,dur_ms,levelz);
}

/* Init for generic envelopes.
 */

void ttaq_env_init_generic(
  struct ttaq_env *env,
  const struct ttaq_synth *synth,
  float initlevel,
  uint16_t atkt_ms,
  float atklevel,
  uint16_t dect_ms,
  float declevel,
  uint8_t sustain,
  uint16_t rlst_ms,
  float rlslevel
) {
  env->hold=0;
  env->stage=0;
  env->sustain=sustain;
  env->released=0;
  env->level=initlevel;
  env->level_dec=atklevel;
  env->level_rls=declevel;
  env->level_fin=rlslevel;
  
  float timescale=(float)synth->rate/1000.0f;
  if ((env->levelc=(int)(timescale*atkt_ms))<1) env->levelc=1;
  if ((env->levelc_dec=(int)(timescale*dect_ms))<1) env->levelc_dec=1;
  if ((env->levelc_rls=(int)(timescale*rlst_ms))<1) env->levelc_rls=1;
  
  env->leveld=(env->level_dec-env->level)/env->levelc;
  env->leveld_dec=(env->level_rls-env->level_dec)/env->levelc_dec;
  env->leveld_rls=(env->level_fin-env->level_rls)/env->levelc_rls;
}

/* Release.
 */

void ttaq_env_release(struct ttaq_env *env) {
  env->released=1;
  if (env->hold&&(env->stage==1)) { // currently sustaining
    ttaq_env_advance_stage(env);
  }
}

/* Advance to next stage.
 */
 
void ttaq_env_advance_stage(struct ttaq_env *env) {
  switch (env->stage) {
    case 0: {
        env->stage=1;
        env->level=env->level_dec;
        env->leveld=env->leveld_dec;
        env->levelc=env->levelc_dec;
      } break;
    case 1: {
        if (env->sustain&&!env->released) { // begin sustain
          env->hold=1;
          env->level=env->level_rls;
        } else {
          env->stage=2;
          env->level=env->level_rls;
          env->leveld=env->leveld_rls;
          env->levelc=env->levelc_rls;
        }
      } break;
    default: {
        env->stage=3;
        env->level=env->level_fin;
        env->leveld=0.0f;
        env->hold=1;
      }
  }
}
