/* adv_input.h
 * Our input manager deals with "devices", "controllers", and "players".
 * Devices come from our supplier (linput). These correspond precisely to evdev files (eg /dev/input/event0).
 * Each device has zero or one controllers associated with it.
 * Each controller is associated with one device.
 * Controllers map the device's events to our simple 8-bit input state.
 * A controller may be "detached" in which case it still digests events, but those events are discarded.
 * If attached, a controller is associated uniquely with one player.
 * Our output is sorted by playerid. Think of players as human beings.
 * Normally each player is associated with one hero sprite, but that's none of our business.
 * We forge and break the association between controller and player; the rest is up to you.
 */

#ifndef ADV_INPUT_H
#define ADV_INPUT_H

#define ADV_BTNID_UP         0x01 // required
#define ADV_BTNID_DOWN       0x02 // required
#define ADV_BTNID_LEFT       0x04 // required
#define ADV_BTNID_RIGHT      0x08 // required
#define ADV_BTNID_ACTION     0x10 // required
#define ADV_BTNID_SWITCH     0x20 // optional
#define ADV_BTNID_DETACH     0x40 // optional
#define ADV_BTNID_FOLLOWME   0x80 // optional

#define ADV_USERACTION_QUIT     1
#define ADV_USERACTION_PAUSE    2
#define ADV_USERACTION_SAVE     3

// Read only. [0] is dummy.
extern unsigned char adv_inputs[1+ADV_PLAYER_LIMIT];

int adv_input_init();
void adv_input_quit();
int adv_input_update();

void adv_input_request_quit(); // defers
int adv_input_quit_requested();
int adv_input_get_pause();
int adv_input_set_pause(int pause);

int adv_input_player_exists(int playerid);
int adv_input_remove_player(int playerid);

int adv_input_map_useraction(int keycode,int useraction);
int adv_input_useraction(int useraction);

int adv_keycode_eval(const char *src,int srcc);
int adv_abscode_eval(const char *src,int srcc);
int adv_bus_eval(const char *src,int srcc);
int adv_useraction_eval(const char *src,int srcc);
int adv_btnid_eval(const char *src,int srcc);
int adv_uint_eval(const char *src,int srcc);

/* For drivers.
 *********************************************************/

int adv_input_devid_next();

int adv_input_connect(int devid,int vid,int pid,const char *name,int namec);
void adv_input_disconnect(int devid);
int adv_input_event(int devid,int btnid,int value);

#endif
