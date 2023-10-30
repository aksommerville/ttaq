/* adv_game_progress.h
 * All the data that needs to be stored for game saving.
 */
 
#ifndef ADV_GAME_PROGRESS_H
#define ADV_GAME_PROGRESS_H

/* "Globals" are variables preserved in a saved game, and accessible to sprite classes.
 * The state of each princess is a global, ditto for switches and general "accomplishments".
 */
#define ADV_GLOBAL_DUMMY              0 // Since global refs may default to zero, leave this slot unused.
#define ADV_GLOBAL_PRINCESS_1         1
#define ADV_GLOBAL_PRINCESS_2         2
#define ADV_GLOBAL_PRINCESS_3         3
#define ADV_GLOBAL_PRINCESS_4         4
#define ADV_GLOBAL_PRINCESS_5         5
#define ADV_GLOBAL_PRINCESS_6         6
#define ADV_GLOBAL_PRINCESS_7         7
#define ADV_GLOBAL_MISC               8 // Anonymous switches from here on...
#define ADV_GLOBAL_COUNT             32

// Princess state.
#define ADV_PRINCESS_AFIELD    0
#define ADV_PRINCESS_RESCUED   1
#define ADV_PRINCESS_DEAD      2

struct adv_game_progress {
  unsigned char globals[ADV_GLOBAL_COUNT];
};

extern struct adv_game_progress adv_game_progress; // current game
extern char *adv_save_game_path; // path to current encoded game progress, or NULL

// Dispatch point for responding to changed globals.
int adv_global_changed(int globalid);

/* Safe global accessors
 *****************************************************************************/

static inline unsigned char adv_global_get(int globalid) {
  if ((globalid<0)||(globalid>=ADV_GLOBAL_COUNT)) return 0;
  return adv_game_progress.globals[globalid];
}

static inline int adv_global_set(int globalid,int value) {
  if ((globalid<0)||(globalid>=ADV_GLOBAL_COUNT)) return -1;
  if ((value<0)||(value>255)) return -1;
  if (value==adv_game_progress.globals[globalid]) return 0;
  adv_game_progress.globals[globalid]=value;
  return adv_global_changed(globalid);
}

/* Encode/decode
 *****************************************************************************/

/* Decoding may clobber any data in (dst) even when it fails.
 * It is unwise to decode directly into the global 'adv_game_progress'.
 */
int adv_game_progress_encode(void *dst,int dsta,const struct adv_game_progress *src);
int adv_game_progress_decode(struct adv_game_progress *dst,const void *src,int srcc);

/* Use NULL path to use whatever was previously requested.
 */
int adv_game_save(const char *path);
int adv_game_load(const char *path);

#endif
