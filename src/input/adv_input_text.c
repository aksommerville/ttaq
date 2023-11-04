#include "adv_input_internal.h"

/* uint eval
 *****************************************************************************/
 
static inline int adv_hexdigit_eval(char src) {
  if ((src>='0')&&(src<='9')) return src-'0';
  if ((src>='a')&&(src<='f')) return src-'a'+10;
  if ((src>='A')&&(src<='F')) return src-'A'+10;
  return -1;
}
 
int adv_uint_eval(const char *src,int srcc) {
  if ((srcc>0)&&!src) return -1;
  if (srcc<0) { srcc=0; if (src) while (src[srcc]) srcc++; }
  if (srcc<1) return -1;
  int dst=0,srcp=0;
  if ((srcp<srcc-2)&&(src[srcp]=='0')&&((src[srcp+1]=='x')||(src[srcp+1]=='X'))) {
    // hexadecimal
    srcp=2;
    while (srcp<srcc) {
      int digit=adv_hexdigit_eval(src[srcp++]);
      if (digit<0) return -1;
      if (dst&~(INT_MAX>>4)) return -1;
      dst<<=4;
      dst|=digit;
    }
  } else {
    // decimal
    while (srcp<srcc) {
      int digit=src[srcp++]-'0';
      if ((digit<0)||(digit>9)) return -1;
      if (dst>INT_MAX/10) return -1; dst*=10;
      if (dst>INT_MAX-digit) return -1; dst+=digit;
    }
  }
  return dst;
}

/* memcasecmp
 *****************************************************************************/
 
static int adv_memcasecmp(const char *a,const char *b,int c) {
  if (c<1) return 0;
  if (a==b) return 0;
  if (!a) return -1;
  if (!b) return 1;
  while (c--) {
    unsigned char cha=*a++; if ((cha>=0x41)&&(cha<=0x5a)) cha+=0x20;
    unsigned char chb=*b++; if ((chb>=0x41)&&(chb<=0x5a)) chb+=0x20;
    if (cha<chb) return -1;
    if (cha>chb) return 1;
  }
  return 0;
}

/* "KEY_*" symbols
 *****************************************************************************/
 
static int _adv_keycode_eval(const char *src,int srcc) {
  switch (srcc) {
    case 1: switch (src[0]) {
        case '1': return KEY_1;
        case '2': return KEY_2;
        case '3': return KEY_3;
        case '4': return KEY_4;
        case '5': return KEY_5;
        case '6': return KEY_6;
        case '7': return KEY_7;
        case '8': return KEY_8;
        case '9': return KEY_9;
        case '0': return KEY_0;
        case 'A': case 'a': return KEY_A;
        case 'B': case 'b': return KEY_B;
        case 'C': case 'c': return KEY_C;
        case 'D': case 'd': return KEY_D;
        case 'E': case 'e': return KEY_E;
        case 'F': case 'f': return KEY_F;
        case 'G': case 'g': return KEY_G;
        case 'H': case 'h': return KEY_H;
        case 'I': case 'i': return KEY_I;
        case 'J': case 'j': return KEY_J;
        case 'K': case 'k': return KEY_K;
        case 'L': case 'l': return KEY_L;
        case 'M': case 'm': return KEY_M;
        case 'N': case 'n': return KEY_N;
        case 'O': case 'o': return KEY_O;
        case 'P': case 'p': return KEY_P;
        case 'Q': case 'q': return KEY_Q;
        case 'R': case 'r': return KEY_R;
        case 'S': case 's': return KEY_S;
        case 'T': case 't': return KEY_T;
        case 'U': case 'u': return KEY_U;
        case 'V': case 'v': return KEY_V;
        case 'W': case 'w': return KEY_W;
        case 'X': case 'x': return KEY_X;
        case 'Y': case 'y': return KEY_Y;
        case 'Z': case 'z': return KEY_Z;
      } break;
    #define K(tag) if (!adv_memcasecmp(src,#tag,srcc)) return KEY_##tag;
    case 2: {
        if ((src[0]=='f')||(src[0]=='F')) switch (src[1]) {
          case 'n': case 'N': return KEY_FN;
          case '1': return KEY_F1;
          case '2': return KEY_F2;
          case '3': return KEY_F3;
          case '4': return KEY_F4;
          case '5': return KEY_F5;
          case '6': return KEY_F6;
          case '7': return KEY_F7;
          case '8': return KEY_F8;
          case '9': return KEY_F9;
        }
        K(RO)
        K(UP)
        K(HP)
        K(OK)
        K(PC)
        K(TV)
        K(AB)
        K(CD)
      } break;
    case 3: {
        K(EPG)
        K(TV2)
        K(RED)
        K(DVD)
        K(AUX)
        K(MP3)
        K(VCR)
        K(PVR)
        K(MHP)
        K(SAT)
        K(UWB)
        K(F10)
        K(ESC)
        K(TAB)
        K(DOT)
        K(KP7)
        K(KP8)
        K(KP9)
        K(KP4)
        K(KP5)
        K(KP6)
        K(KP1)
        K(KP2)
        K(KP3)
        K(KP0)
        K(F11)
        K(F12)
        K(F13)
        K(F14)
        K(F15)
        K(F16)
        K(F17)
        K(F18)
        K(F19)
        K(F20)
        K(F21)
        K(F22)
        K(F23)
        K(F24)
        K(END)
        K(YEN)
        K(CUT)
        K(WWW)
        K(ISO)
        K(NEW)
        if (!adv_memcasecmp(src,"DEL",3)) return KEY_DELETE;
        if (!adv_memcasecmp(src,"INS",3)) return KEY_INSERT;
      } break;
    case 4: {
        K(REDO)
        K(PLAY)
        K(CHAT)
        K(SHOP)
        K(SEND)
        K(SAVE)
        K(WLAN)
        K(NEXT)
        K(SLOW)
        K(TEEN)
        K(TWEN)
        K(NEWS)
        K(EURO)
        K(FN_1)
        K(FN_2)
        K(FN_D)
        K(FN_E)
        K(FN_F)
        K(FN_S)
        K(FN_B)
        K(GOTO)
        K(LIST)
        K(MEMO)
        K(VCR2)
        K(INFO)
        K(TIME)
        K(SAT2)
        K(BLUE)
        K(ZOOM)
        K(TAPE)
        K(LAST)
        K(MODE)
        K(TEXT)
        K(STOP)
        K(UNDO)
        K(EXIT)
        K(MOVE)
        K(EDIT)
        K(COPY)
        K(OPEN)
        K(FIND)
        K(HELP)
        K(MENU)
        K(CALC)
        K(FILE)
        K(XFER)
        K(MAIL)
        K(BACK)
        K(HOME)
        K(LEFT)
        K(DOWN)
        K(MUTE)
      } break;
    case 5: {
        K(MINUS)
        K(EQUAL)
        K(ENTER)
        K(GRAVE)
        K(RIGHT)
        K(COMMA)
        K(SLASH)
        K(MACRO)
        K(POWER)
        K(PAUSE)
        K(SCALE)
        K(HANJA)
        K(SPACE)
        K(SYSRQ)
        K(REPLY)
        K(CLEAR)
        K(TITLE)
        K(ANGLE)
        K(RADIO)
        K(TUNER)
        K(AUDIO)
        K(VIDEO)
        K(GREEN)
        K(FIRST)
        K(FN_F1)
        K(FN_F2)
        K(FN_F3)
        K(FN_F4)
        K(FN_F5)
        K(FN_F6)
        K(FN_F7)
        K(FN_F8)
        K(FN_F9)
        K(GAMES)
        K(BREAK)
        K(PROG3)
        K(PROG4)
        K(CLOSE)
        K(PRINT)
        K(EMAIL)
        K(SOUND)
        K(MEDIA)
        K(SPORT)
        K(WIMAX)
        K(KPDOT)
        K(102ND)
        K(AGAIN)
        K(PROPS)
        K(FRONT)
        K(PASTE)
        K(SETUP)
        K(SLEEP)
        K(PROG1)
        K(PROG2)
        K(MSDOS)
        K(PHONE)
      } break;
    case 6: {
        K(WAKEUP)
        K(COFFEE)
        K(STOPCD)
        K(RECORD)
        K(REWIND)
        K(CONFIG)
        K(KPPLUS)
        K(HENKAN)
        K(INSERT)
        K(DELETE)
        K(PAGEUP)
        K(DIGITS)
        K(ZOOMIN)
        K(EDITOR)
        K(LOGOFF)
        K(IMAGES)
        K(DOLLAR)
        K(FN_F10)
        K(FN_F11)
        K(FN_F12)
        K(FN_ESC)
        K(POWER2)
        K(YELLOW)
        K(OPTION)
        K(SELECT)
        K(VENDOR)
        K(PLAYER)
        K(SCREEN)
        K(RFKILL)
        K(PLAYCD)
        K(CAMERA)
        K(SEARCH)
        K(CANCEL)
        if (!adv_memcasecmp(src,"ESCAPE",6)) return KEY_ESC;
        if (!adv_memcasecmp(src,"RETURN",6)) return KEY_ENTER;
      } break;
    case 7: {
        K(KPEQUAL)
        K(KPCOMMA)
        K(HANGEUL)
        K(HANGUEL)
        K(COMPOSE)
        K(LEFTALT)
        K(NUMLOCK)
        K(KPMINUS)
        K(KPENTER)
        K(KPSLASH)
        K(REFRESH)
        K(FORWARD)
        K(CLOSECD)
        K(EJECTCD)
        K(CONNECT)
        K(FINANCE)
        K(SUSPEND)
        K(PAUSECD)
        K(BATTERY)
        K(MICMUTE)
        K(UNKNOWN)
        K(RESTART)
        K(SHUFFLE)
        K(ZOOMOUT)
        K(DEL_EOL)
        K(DEL_EOS)
        K(ARCHIVE)
        K(PROGRAM)
        K(CHANNEL)
      } break;
    case 8: {
        K(QUESTION)
        K(LANGUAGE)
        K(SUBTITLE)
        K(KEYBOARD)
        K(CALENDAR)
        K(BRL_DOT1)
        K(BRL_DOT2)
        K(BRL_DOT3)
        K(BRL_DOT4)
        K(BRL_DOT5)
        K(BRL_DOT6)
        K(BRL_DOT7)
        K(BRL_DOT8)
        K(BRL_DOT9)
        K(INS_LINE)
        K(DEL_LINE)
        K(PREVIOUS)
        K(DATABASE)
        K(ALTERASE)
        K(SENDFILE)
        K(COMPUTER)
        K(HOMEPAGE)
        K(NEXTSONG)
        K(SCROLLUP)
        K(LEFTCTRL)
        K(CAPSLOCK)
        K(KATAKANA)
        K(LINEFEED)
        K(PAGEDOWN)
        K(VOLUMEUP)
        K(LEFTMETA)
        K(HIRAGANA)
        K(MUHENKAN)
        K(RIGHTALT)
      } break;
    case 9: {
        K(BRL_DOT10)
        K(NUMERIC_0)
        K(NUMERIC_1)
        K(NUMERIC_2)
        K(NUMERIC_3)
        K(NUMERIC_4)
        K(NUMERIC_5)
        K(NUMERIC_6)
        K(NUMERIC_7)
        K(NUMERIC_8)
        K(NUMERIC_9)
        K(CAMERA_UP)
        K(MESSENGER)
        K(ZOOMRESET)
        K(FRAMEBACK)
        K(VOICEMAIL)
        K(FAVORITES)
        K(DIRECTORY)
        K(CHANNELUP)
        K(DOCUMENTS)
        K(DASHBOARD)
        K(BASSBOOST)
        K(BLUETOOTH)
        K(RIGHTCTRL)
        K(BACKSPACE)
        K(LEFTBRACE)
        K(SEMICOLON)
        K(LEFTSHIFT)
        K(BACKSLASH)
        K(KPJPCOMMA)
        K(RIGHTMETA)
        K(DIRECTION)
        K(BOOKMARKS)
        K(PLAYPAUSE)
      } break;
    case 10: {
        K(SPELLCHECK)
        K(VIDEOPHONE)
        K(WPS_BUTTON)
        K(VIDEO_NEXT)
        K(VIDEO_PREV)
        K(KBDILLUMUP)
        K(RIGHTBRACE)
        K(APOSTROPHE)
        K(RIGHTSHIFT)
        K(KPASTERISK)
        K(SCROLLLOCK)
        K(VOLUMEDOWN)
        K(SCROLLDOWN)
        K(DELETEFILE)
        K(SCREENLOCK)
      } break;
    case 11: {
        K(FORWARDMAIL)
        K(FASTFORWARD)
        K(DISPLAY_OFF)
        K(CHANNELDOWN)
        K(TOUCHPAD_ON)
        K(CAMERA_DOWN)
        K(CAMERA_LEFT)
        K(SPREADSHEET)
        K(ADDRESSBOOK)
        K(KPPLUSMINUS)
        K(KPLEFTPAREN)
      } break;
    case 12: {
        K(CYCLEWINDOWS)
        K(EJECTCLOSECD)
        K(PREVIOUSSONG)
        K(KPRIGHTPAREN)
        K(CONTEXT_MENU)
        K(MEDIA_REPEAT)
        K(10CHANNELSUP)
        K(PRESENTATION)
        K(FRAMEFORWARD)
        K(CAMERA_RIGHT)
        K(TOUCHPAD_OFF)
        K(CAMERA_FOCUS)
        K(NUMERIC_STAR)
        K(BRIGHTNESSUP)
        K(KBDILLUMDOWN)
      } break;
    case 13: {
        K(NUMERIC_POUND)
        K(CAMERA_ZOOMIN)
        K(WORDPROCESSOR)
        K(DISPLAYTOGGLE)
      } break;
    case 14: {
        K(ZENKAKUHANKAKU)
        K(BRIGHTNESSDOWN)
        K(KBDILLUMTOGGLE)
        K(GRAPHICSEDITOR)
        K(10CHANNELSDOWN)
        K(CAMERA_ZOOMOUT)
      } break;
    case 15: {
        K(SWITCHVIDEOMODE)
        K(BRIGHTNESS_ZERO)
        K(TOUCHPAD_TOGGLE)
      } break;
    case 16: {
        K(BRIGHTNESS_CYCLE)
        K(KATAKANAHIRAGANA)
      } break;
    #undef K
  }
  return -1;
}

/* "BTN_*" symbols
 *****************************************************************************/
 
static int _adv_btncode_eval(const char *src,int srcc) {
  switch (srcc) {
    case 1: switch (src[0]) {
        case '0': return BTN_0;
        case '1': return BTN_1;
        case '2': return BTN_2;
        case '3': return BTN_3;
        case '4': return BTN_4;
        case '5': return BTN_5;
        case '6': return BTN_6;
        case '7': return BTN_7;
        case '8': return BTN_8;
        case '9': return BTN_9;
        case 'A': case 'a': return BTN_A;
        case 'B': case 'b': return BTN_B;
        case 'C': case 'c': return BTN_C;
        case 'X': case 'x': return BTN_X;
        case 'Y': case 'y': return BTN_Y;
        case 'Z': case 'z': return BTN_Z;
      } break;
    #define B(tag) if (!adv_memcasecmp(src,#tag,srcc)) return BTN_##tag;
    case 2: {
        B(TL)
        B(TR)
      } break;
    case 3: {
        B(TL2)
        B(TR2)
        B(TOP)
      } break;
    case 4: {
        B(TOP2)
        B(DEAD)
        B(BACK)
        B(TASK)
        B(MODE)
        B(BASE)
        B(SIDE)
        B(LEFT)
      } break;
    case 5: {
        B(RIGHT)
        B(EXTRA)
        B(BASE2)
        B(BASE3)
        B(BASE4)
        B(BASE5)
        B(BASE6)
        B(START)
        B(TOUCH)
        B(THUMB)
      } break;
    case 6: {
        B(THUMB2)
        B(MIDDLE)
        B(PINKIE)
        B(SELECT)
        B(THUMBL)
        B(THUMBR)
        B(STYLUS)
      } break;
    case 7: {
        B(FORWARD)
        B(TRIGGER)
        B(STYLUS2)
        B(GEAR_UP)
      } break;
    case 8: {
        B(TOOL_PEN)
      } break;
    case 9: {
        B(TOOL_LENS)
        B(GEAR_DOWN)
      } break;
    case 10: {
        B(TOOL_BRUSH)
        B(TOOL_MOUSE)
      } break;
    case 11: {
        B(TOOL_RUBBER)
        B(TOOL_PENCIL)
        B(TOOL_FINGER)
      } break;
    case 12: {
        B(TOOL_QUADTAP)
      } break;
    case 13: {
        B(TOOL_AIRBRUSH)
        B(TOOL_QUINTTAP)
      } break;
    case 14: {
        B(TOOL_DOUBLETAP)
        B(TOOL_TRIPLETAP)
      } break;
    #undef B
  }
  // BTN_TRIGGER_HAPPY1 through BTN_TRIGGER_HAPPY40...
  if (((srcc==14)||(srcc==15))&&!adv_memcasecmp(src,"TRIGGER_HAPPY",13)) {
    int hi=0,lo;
    if (srcc==14) lo=src[13]-'0';
    else { hi=src[13]-'0'; lo=src[14]-'0'; }
    if ((hi<0)||(hi>9)||(lo<0)||(lo>9)) return -1;
    int n=hi*10+lo;
    if ((n<1)||(n>40)) return -1;
    return BTN_TRIGGER_HAPPY1+n-1;
  }
  return -1;
}

/* "ABS_*" symbols
 *****************************************************************************/
 
static int _adv_abscode_eval(const char *src,int srcc) {
  switch (srcc) {
    case 1: switch (src[0]) {
        case 'x': case 'X': return ABS_X;
        case 'y': case 'Y': return ABS_Y;
        case 'z': case 'Z': return ABS_Z;
      } break;
    #define A(tag) if (!adv_memcasecmp(src,#tag,srcc)) return ABS_##tag;
    case 2: {
        A(RX)
        A(RY)
        A(RZ)
      } break;
    case 3: {
        A(GAS)
      } break;
    case 4: {
        A(MISC)
      } break;
    case 5: {
        A(WHEEL)
        A(BRAKE)
        A(HAT0X)
        A(HAT0Y)
        A(HAT1X)
        A(HAT1Y)
        A(HAT2X)
        A(HAT2Y)
        A(HAT3X)
        A(HAT3Y)
      } break;
    case 6: {
        A(RUDDER)
        A(TILT_X)
        A(TILT_Y)
        A(VOLUME)
      } break;
    case 7: {
        A(MT_SLOT)
      } break;
    case 8: {
        A(THROTTLE)
        A(PRESSURE)
        A(DISTANCE)
      } break;
    case 10: {
        A(TOOL_WIDTH)
        A(MT_BLOB_ID)
      } break;
    case 11: {
        A(MT_PRESSURE)
        A(MT_DISTANCE)
      } break;
    case 12: {
        A(MT_TOOL_TYPE)
      } break;
    case 13: {
        A(MT_POSITION_X)
        A(MT_POSITION_Y)
      } break;
    case 14: {
        A(MT_TOUCH_MAJOR)
        A(MT_TOUCH_MINOR)
        A(MT_WIDTH_MAJOR)
        A(MT_WIDTH_MINOR)
        A(MT_ORIENTATION)
        A(MT_TRACKING_ID)
      } break;
    #undef A
  }
  // Linux headers only declare "ABS_MISC", but let's fake eg "ABS_MISC0", "ABS_MISC1"...
  if ((srcc==5)&&!adv_memcasecmp(src,"MISC",4)&&(src[4]>='0')&&(src[4]<='6')) return ABS_MISC+src[4]-'0';
  return -1;
}

/* keycode eval, dispatch
 *****************************************************************************/
 
int adv_keycode_eval(const char *src,int srcc) {
  if ((srcc>0)&&!src) return -1;
  if (srcc<0) { srcc=0; if (src) while (src[srcc]) srcc++; }
  
  /* Try to treat single characters as their name.
   * This is helpful for punctuation and such, like the user can say "[" instead of "LEFTBRACE".
   * Anything where the macro is 1 character need not bother here.
   */
  if (srcc==1) switch (src[0]) {
    case '-': return KEY_MINUS;
    case '=': return KEY_EQUAL;
    case '[': return KEY_LEFTBRACE;
    case ']': return KEY_RIGHTBRACE;
    case ';': return KEY_SEMICOLON;
    case '\'': return KEY_APOSTROPHE;
    case '`': return KEY_GRAVE;
    case '\\': return KEY_BACKSLASH;
    case ',': return KEY_COMMA;
    case '.': return KEY_DOT;
    case '/': return KEY_SLASH;
    // Shifted values. This is for the US keyboard layout.
    case '_': return KEY_MINUS;
    case '+': return KEY_EQUAL;
    case '{': return KEY_LEFTBRACE;
    case '}': return KEY_RIGHTBRACE;
    case ':': return KEY_SEMICOLON;
    case '"': return KEY_APOSTROPHE;
    case '~': return KEY_GRAVE;
    case '|': return KEY_BACKSLASH;
    case '<': return KEY_COMMA;
    case '>': return KEY_DOT;
    case '?': return KEY_SLASH;
    case '!': return KEY_1;
    case '@': return KEY_2;
    case '#': return KEY_3;
    case '$': return KEY_4;
    case '%': return KEY_5;
    case '^': return KEY_6;
    case '&': return KEY_7;
    case '*': return KEY_8;
    case '(': return KEY_9;
    case ')': return KEY_0;
  }
  
  /* If it begins with "KEY_" or "BTN_", look up symbols. */
  if ((srcc>=4)&&!adv_memcasecmp(src,"KEY_",4)) return _adv_keycode_eval(src+4,srcc-4);
  if ((srcc>=4)&&!adv_memcasecmp(src,"BTN_",4)) return _adv_btncode_eval(src+4,srcc-4);
  
  /* If it begins with "KEY:" or "BTN:", user is unambiguously asking for integer evaluation. */
  if ((srcc>=4)&&(!adv_memcasecmp(src,"KEY:",4)||!adv_memcasecmp(src,"BTN:",4))) {
    return adv_uint_eval(src+4,srcc-4);
  }
  
  /* Try unprefixed KEY_ or BTN_ codes. */
  int code;
  if ((code=_adv_keycode_eval(src,srcc))>=0) return code;
  if ((code=_adv_btncode_eval(src,srcc))>=0) return code;
  
  /* Evaluate as straight integer. */
  return adv_uint_eval(src,srcc);
}

/* abscode eval
 *****************************************************************************/
 
int adv_abscode_eval(const char *src,int srcc) {
  if ((srcc>0)&&!src) return -1;
  if (srcc<0) { srcc=0; if (src) while (src[srcc]) srcc++; }
  
  /* If it begins with "ABS_", look up symbols. */
  if ((srcc>=4)&&!adv_memcasecmp(src,"ABS_",4)) return _adv_abscode_eval(src+4,srcc-4);
  
  /* If it begins with "ABS:", user is unambiguously asking for integer evaluation. */
  if ((srcc>=4)&&!adv_memcasecmp(src,"ABS:",4)) return adv_uint_eval(src+4,srcc-4);
  
  /* Unprefixed ABS_ code? */
  int code=_adv_abscode_eval(src,srcc);
  if (code>=0) return code;
  
  /* Straight integer. */
  return adv_uint_eval(src,srcc);
}

/* bus eval
 *****************************************************************************/
 
int adv_bus_eval(const char *src,int srcc) {
  if ((srcc>0)&&!src) return -1;
  if (srcc<0) { srcc=0; if (src) while (src[srcc]) srcc++; }
  switch (srcc) {
    case 3: {
        if (!memcmp(src,"PCI",3)) return BUS_PCI;
        if (!memcmp(src,"USB",3)) return BUS_USB;
        if (!memcmp(src,"HIL",3)) return BUS_HIL;
        if (!memcmp(src,"ISA",3)) return BUS_ISA;
        if (!memcmp(src,"ADB",3)) return BUS_ADB;
        if (!memcmp(src,"I2C",3)) return BUS_I2C;
        if (!memcmp(src,"GSC",3)) return BUS_GSC;
        if (!memcmp(src,"SPI",3)) return BUS_SPI;
      } break;
    case 4: {
        if (!memcmp(src,"HOST",4)) return BUS_HOST;
      } break;
    case 5: {
        if (!memcmp(src,"AMIGA",5)) return BUS_AMIGA;
        if (!memcmp(src,"ATARI",5)) return BUS_ATARI;
        if (!memcmp(src,"I8042",5)) return BUS_I8042;
        if (!memcmp(src,"XTKBD",5)) return BUS_XTKBD;
        if (!memcmp(src,"RS232",5)) return BUS_RS232;
      } break;
    case 6: {
        if (!memcmp(src,"ISAPNP",6)) return BUS_ISAPNP;
      } break;
    case 7: {
        if (!memcmp(src,"PARPORT",7)) return BUS_PARPORT;
        if (!memcmp(src,"VIRTUAL",7)) return BUS_VIRTUAL;
      } break;
    case 8: {
        if (!memcmp(src,"GAMEPORT",8)) return BUS_GAMEPORT;
      } break;
    case 9: {
        if (!memcmp(src,"BLUETOOTH",9)) return BUS_BLUETOOTH;
      } break;
  }
  return adv_uint_eval(src,srcc);
}

/* useraction eval
 *****************************************************************************/
 
int adv_useraction_eval(const char *src,int srcc) {
  if ((srcc>0)&&!src) return -1;
  if (srcc<0) { srcc=0; if (src) while (src[srcc]) srcc++; }
  if ((srcc==4)&&!adv_memcasecmp(src,"QUIT",4)) return ADV_USERACTION_QUIT;
  if ((srcc==4)&&!adv_memcasecmp(src,"SAVE",4)) return ADV_USERACTION_SAVE;
  if ((srcc==5)&&!adv_memcasecmp(src,"PAUSE",5)) return ADV_USERACTION_PAUSE;
  if ((srcc==10)&&!adv_memcasecmp(src,"FULLSCREEN",10)) return ADV_USERACTION_FULLSCREEN;
  return -1;
}

/* btnid eval
 *****************************************************************************/
 
int adv_btnid_eval(const char *src,int srcc) {
  if ((srcc>0)&&!src) return -1;
  if (srcc<0) { srcc=0; if (src) while (src[srcc]) srcc++; }
  if ((srcc==2)&&!adv_memcasecmp(src,"UP",2)) return ADV_BTNID_UP;
  if ((srcc==4)&&!adv_memcasecmp(src,"DOWN",4)) return ADV_BTNID_DOWN;
  if ((srcc==4)&&!adv_memcasecmp(src,"LEFT",4)) return ADV_BTNID_LEFT;
  if ((srcc==5)&&!adv_memcasecmp(src,"RIGHT",5)) return ADV_BTNID_RIGHT;
  if ((srcc==6)&&!adv_memcasecmp(src,"ACTION",6)) return ADV_BTNID_ACTION;
  if ((srcc==6)&&!adv_memcasecmp(src,"SWITCH",6)) return ADV_BTNID_SWITCH;
  if ((srcc==6)&&!adv_memcasecmp(src,"DETACH",6)) return ADV_BTNID_DETACH;
  if ((srcc==8)&&!adv_memcasecmp(src,"FOLLOWME",8)) return ADV_BTNID_FOLLOWME;
  return -1;
}
