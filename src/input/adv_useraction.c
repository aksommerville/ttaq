#include "adv_input_internal.h"
#include "game/adv_game_progress.h"
#include "video/adv_video.h"

/* trigger useraction, main dispatch
 *****************************************************************************/
 
int adv_input_useraction(int useraction) {
  switch (useraction) {
    case ADV_USERACTION_QUIT: adv_input.quit_requested=1; break;
    case ADV_USERACTION_PAUSE: adv_input.pause^=1; break;
    case ADV_USERACTION_SAVE: adv_game_save(0); break;
    case ADV_USERACTION_FULLSCREEN: adv_video_toggle_fullscreen(); break;
  }
  return 0;
}
