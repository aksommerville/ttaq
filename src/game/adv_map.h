/* adv_map.h
 * Single screen of background data and associated header data.
 * The map set is managed by resource manager.
 *
 * ----- Map Resource Format -----
 * XXX This text format is being replaced by a tighter binary format, see below.
 *
 * These are text files.
 * Formatting is stricter than most text formats, for the decoder's convenience.
 * You must use a single LF for newline, and you must finish the file with a newline.
 * '#' line comments are permitted in the header section.
 *
 * Header may contain:
 *   KEY VALUE         # Arbitrary header fields, see below.
 *   tile 'XX' BG FG   # Use values BG and FG for string XX in tile data, BG and FG are hexadecimal integers.
 *   sprite ID COL ROW # Arguments are decimal integers.
 *   switch COL ROW TILEOFF TILEON GLOBAL METHOD
 *
 * "tile" lines must be exactly 10 characters from the first quote: 'XX' XX XX
 *
 * At the bottom of the file is tile data.
 * This is (ADV_MAP_H) lines, each consisting of (ADV_MAP_W*2+1) bytes.
 * Text from the tile data is read 2 bytes at a time and looked up against declared tile values.
 * We arbitrarily limit you to 256 defined tiles. Easy enough to add more if we ever need to.
 *
 * Header keys:
 *   bg INTEGER           # Background tilesheet ID.
 *   fg INTEGER           # Foreground tilesheet ID.
 *   spr INTEGER          # Sprite tilesheet ID.
 *   north INTEGER        # Neighbor map ID.
 *   south INTEGER        # Neighbor map ID.
 *   west INTEGER         # Neighbor map ID.
 *   east INTEGER         # Neighbor map ID.
 *   lights_out BOOL      # If nonzero, use the lights-out effect.
 *   song INTEGER         # Background music song ID.
 *
 * ----- Map Resource Format, Binary ----
 *
 * Integers are big-endian unsigned unless noted.
 *
 * Begins with 4-byte signature: "\0MP\xff"
 * Followed by map dimensions, one byte each, for validation only.
 * We only permit maps at the fixed size (ADV_MAP_W,ADV_MAP_H).
 * Followed by cell data, one layer only, one byte per cell. ie, (ADV_MAP_W*ADV_MAP_H) bytes.
 *
 * Followed by arbitrary fields described by their first byte:
 *   0x00 TERMINATOR
 *          End of data. Stop processing here. Additional data is permitted after, but never used.
 *   0x01 BGTILESHEETID
 *   0x02 SPRTILESHEETID
 *   0x03 NORTHMAPID
 *   0x04 SOUTHMAPID
 *   0x05 EASTMAPID
 *   0x06 WESTMAPID
 *   0x07 SONGID
 *          All resource references are 2 bytes unsigned.
 *   0x08 LIGHTS_OUT
 *          One byte boolean.
 *   0x09 SPRITE
 *          1  Column
 *          1  Row
 *          2  Sprite ID
 *   0x0a DOOR
 *          1  Column
 *          1  Row
 *          2  Map ID
 *          1  Destination column
 *          1  Destination row
 *   0x0b SWITCH
 *          1  Column
 *          1  Row
 *          1  Tile ID (off)
 *          1  Tile ID (on). Method THREEWAY ignores this.
 *          1  Global ID
 *          1  Method
 *
 */

#ifndef adv_map_h
#define adv_map_h

#define ADV_SWITCH_METHOD_OUTPUT             0 // read-only
#define ADV_SWITCH_METHOD_SWORDSCREW         1 // sword can turn it; holds value
#define ADV_SWITCH_METHOD_DEADMAN            2 // enabled while a living hero stands on it
#define ADV_SWITCH_METHOD_STOMPBOX           3 // toggles state when a living hero passes over it
#define ADV_SWITCH_METHOD_THREEWAY           4 // three-state output

struct adv_spawn {
  int sprdefid;
  int col,row;
};

struct adv_door {
  int dstmapid;
  int srccol,srcrow;
  int dstcol,dstrow;
};

struct adv_switch {
  int col,row;
  int tileoff,tileon;
  int globalid;
  int method;
};

struct adv_map {
  int mapid;
  int bg,spr;
  int north,south,west,east;
  int lights_out;
  int song;
  struct adv_spawn *spawnv; int spawnc,spawna;
  struct adv_door *doorv; int doorc,doora;
  struct adv_switch *switchv; int switchc,switcha;
  unsigned char cellv[ADV_MAP_W*ADV_MAP_H]; // LRTB
  int suspend_xfer; // transient
  const unsigned char *propv; // points to a shared tsprop resource; may be NULL
};

// Only resource manager need concern itself with these.
// "decode" functions should only be performed on a brand-new map.
// They do not wipe existing data.
// Failed decodes may leave the map partially populated.
// Encoding does not bother to confirm that values fit in the target field.
int adv_map_new(struct adv_map **dst);
void adv_map_del(struct adv_map *map);
int adv_map_decode_file(struct adv_map *map,const char *path); // text or binary
int adv_map_decode_text(struct adv_map *map,const void *src,int srcc,const char *refname);
int adv_map_decode_binary(struct adv_map *map,const void *src,int srcc,const char *refname);
int adv_map_encode_binary(void *dst,int dsta,const struct adv_map *map);

// Read tile properties. Don't be a hero and try to do this yourself.
int adv_map_cell_is_solid(struct adv_map *map,int col,int row,int check_positive,int check_negative);
int adv_map_cell_is_heal(struct adv_map *map,int col,int row);

// Examine globals and update tiles to match.
int adv_map_update_switches(struct adv_map *map,int draw);

#endif
