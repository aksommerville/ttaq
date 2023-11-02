#include "ttaq_synth_internal.h"
#include "res/adv_res.h"

/* Delete.
 */

void ttaq_song_del(struct ttaq_song *song) {
  if (!song) return;
  if (song->eventv) free(song->eventv);
  free(song);
}

/* Add event.
 */
 
static struct ttaq_song_event *ttaq_song_add_event(struct ttaq_song *song) {
  if (song->eventc>=song->eventa) {
    int na=song->eventa+128;
    if (na>INT_MAX/sizeof(struct ttaq_song_event)) return 0;
    void *nv=realloc(song->eventv,sizeof(struct ttaq_song_event)*na);
    if (!nv) return 0;
    song->eventv=nv;
    song->eventa=na;
  }
  struct ttaq_song_event *event=song->eventv+song->eventc++;
  memset(event,0,sizeof(struct ttaq_song_event));
  event->time=song->framec;
  return event;
}

/* Decode "instrument" line.
 * Every song has the same set of 7 instruments.
 * Since I'm not likely to add more songs, just validate that they match.
 * I guess we could just ignore this bit.
 */
 
static int ttaq_song_decode_instrument(struct ttaq_song *song,const char *src,int srcc) {
  int srcp=0;
  while ((srcp<srcc)&&((unsigned char)src[srcp]<=0x20)) srcp++;
  const char *token=src+srcp;
  int tokenc=0;
  while ((srcp<srcc)&&((unsigned char)src[srcp++]>0x20)) tokenc++;
  if ((tokenc!=10)||memcmp(token,"instrument",10)) return -1;
  while ((srcp<srcc)&&((unsigned char)src[srcp]<=0x20)) srcp++;
  token=src+srcp;
  tokenc=0;
  while ((srcp<srcc)&&((unsigned char)src[srcp++]>0x20)) tokenc++;
  if (tokenc!=1) return -1;
  int insid=token[0]-'0';
  while ((srcp<srcc)&&((unsigned char)src[srcp]<=0x20)) srcp++;
  token=src+srcp;
  tokenc=0;
  while ((srcp<srcc)&&((unsigned char)src[srcp++]>0x20)) tokenc++;
  while ((srcp<srcc)&&((unsigned char)src[srcp]<=0x20)) srcp++;
  if (srcp<srcc) return -1;
  switch (insid) {
    case 0: if ((tokenc!=4)||memcmp(token,"kick",4)) return -1; break;
    case 1: if ((tokenc!=5)||memcmp(token,"snare",5)) return -1; break;
    case 2: if ((tokenc!=3)||memcmp(token,"hat",3)) return -1; break;
    case 3: if ((tokenc!=4)||memcmp(token,"ride",4)) return -1; break;
    case 4: if ((tokenc!=4)||memcmp(token,"lead",4)) return -1; break;
    case 5: if ((tokenc!=4)||memcmp(token,"bass",4)) return -1; break;
    case 6: if ((tokenc!=3)||memcmp(token,"pad",3)) return -1; break;
    default: return -1;
  }
  return 0;
}

/* Decode "tempo" line.
 * TODO I forget what the unit is supposed to be.
 * Higher is faster.
 * Existing songs range from 300 to 1200.
 */
 
static int ttaq_song_decode_tempo(struct ttaq_song *song,const char *src,int srcc) {
  int srcp=0;
  while ((srcp<srcc)&&((unsigned char)src[srcp]<=0x20)) srcp++;
  if ((srcp>srcc-5)||memcmp(src+srcp,"tempo",5)) return -1;
  srcp+=5;
  if ((srcp>=srcc)||((unsigned char)src[srcp]>0x20)) return -1;
  while ((srcp<srcc)&&((unsigned char)src[srcp]<=0x20)) srcp++;
  int tempo=0;
  while ((srcp<srcc)&&((unsigned char)src[srcp]>0x20)) {
    int digit=src[srcp++]-'0';
    if ((digit<0)||(digit>9)) return -1;
    tempo*=10;
    tempo+=digit;
    if (tempo>999999) return -1;
  }
  if (!tempo) return -1;
  while ((srcp<srcc)&&((unsigned char)src[srcp]<=0x20)) srcp++;
  if (srcp<srcc) return -1;
  song->tempo=tempo;
  return 0;
}

/* Decode one track header in chart block.
 */
 
struct ttaq_track {
  int instrument;
  int level;
  int pan;
  int sustainp; // index in song->eventv, or <0 if nothing is sustaining right now
};
 
static int ttaq_song_decode_track_header(struct ttaq_track *track,const char *src,int srcc) {
  memset(track,0,sizeof(struct ttaq_track));
  track->sustainp=-1;
  int srcp=0;
  
  // Instrument zero is a real thing and should need to be distinguished from empty.
  // But luckily, all our songs have "kick" as instrument zero, which would never be declared in the header.
  while ((srcp<srcc)&&(src[srcp]!=':')) {
    if ((unsigned char)src[srcp]>0x20) {
      int digit=src[srcp]-'0';
      if ((digit<0)||(digit>9)) return -1;
      track->instrument*=10;
      track->instrument+=digit;
    }
    srcp++;
  }
  if (srcp>=srcc) return 0;
  if (src[srcp++]!=':') return -1;
  while ((srcp<srcc)&&((unsigned char)src[srcp]<=0x20)) srcp++;
  
  while ((srcp<srcc)&&(src[srcp]!='@')) {
    if ((unsigned char)src[srcp]>0x20) {
      int digit=src[srcp]-'0';
      if ((digit<0)||(digit>9)) return -1;
      track->level*=10;
      track->level+=digit;
    }
    srcp++;
  }
  if (srcp>=srcc) return 0;
  if (src[srcp++]!='@') return -1;
  while ((srcp<srcc)&&((unsigned char)src[srcp]<=0x20)) srcp++;
  
  int panpositive=1;
  while (srcp<srcc) {
    if ((unsigned char)src[srcp]>0x20) {
      if (src[srcp]=='-') {
        panpositive=0;
      } else {
        int digit=src[srcp]-'0';
        if ((digit<0)||(digit>9)) return -1;
        track->pan*=10;
        if (panpositive) track->pan+=digit; else track->pan-=digit;
      }
    }
    srcp++;
  }
  
  return 0;
}

/* Advance clock after decoding one beat from the chart.
 */
 
static int ttaq_song_next_beat(struct ttaq_song *song) {
  if (!song->frames_per_beat) {
    if ((song->tempo<1)||(song->mainrate<1)) return -1;
    song->frames_per_beat=(song->mainrate*60)/song->tempo;
    if (song->frames_per_beat<1) song->frames_per_beat=1;
  }
  song->framec+=song->frames_per_beat;
  return 0;
}

/* Release a pending note.
 */
 
static void ttaq_song_release_event(struct ttaq_song *song,struct ttaq_song_event *event) {
  event->ttl=song->framec-event->time;
}

/* Decode one note from the chart.
 */
 
static int ttaq_song_decode_note(struct ttaq_song *song,struct ttaq_track *track,const char *src,int srclimit) {
  int srcc=0;
  while ((srcc<srclimit)&&((unsigned char)src[srcc]>0x20)) srcc++;
  if (srcc<1) return 0; // blanks are normal, do nothing.
  
  // Release whatever's sustaining.
  if (track->sustainp>=0) {
    ttaq_song_release_event(song,song->eventv+track->sustainp);
    track->sustainp=-1;
  }
  
  // Exclamation mark only means release -- cool, done.
  if ((srcc==1)&&(src[0]=='!')) return 0;
  
  // Create the event.
  track->sustainp=song->eventc;
  struct ttaq_song_event *event=ttaq_song_add_event(song);
  if (!event) return -1;
  
  // Tracks with instrument zero are percussion. Event must be a simple decimal integer.
  if (!track->instrument) {
    for (;srcc-->0;src++) {
      if ((*src<'0')||(*src>'9')) return -1;
      event->soundid*=10;
      event->soundid+=(*src)-'0';
    }
    switch (event->soundid) {
      case 0: event->soundid=ADV_SOUND_KICK; break;
      case 1: event->soundid=ADV_SOUND_SNARE; break;
      case 2: event->soundid=ADV_SOUND_HAT; break;
      case 3: event->soundid=ADV_SOUND_RIDE; break;
      default: event->soundid=-1; break;
    }
    return 0;
  }
  
  /* Others are: TONE [ADJUST] OCTAVE
   * And I probably started octaves at A instead of C because hey, alphabet, what a concept.
   * (it's supposed to be C, because it was invented by musicians).
   */
  int tone;
  switch (src[0]) {
    case 'a': tone=0; break;
    case 'b': tone=2; break;
    case 'c': tone=3; break;
    case 'd': tone=5; break;
    case 'e': tone=7; break;
    case 'f': tone=8; break;
    case 'g': tone=10; break;
  }
  src++; srcc--;
  if (!srcc) return -1;
  if (*src=='#') { tone++; src++; srcc--; }
  else if (*src=='b') { tone--; src++; srcc--; }
  if (!srcc) return -1;
  int octave=0;
  for (;srcc-->0;src++) {
    if ((*src<'0')||(*src>'9')) return -1;
    octave*=10;
    octave+=(*src)-'0';
  }
  int noteid=octave*12+tone;
  
  /* Arbitrarily put noteid 0x10 at 440 hz, which is probably somewhere close to right.
   * Our tones don't need to match anything else, so just season to taste.
   */
  event->rate=(440.0f*powf(2.0f,(float)(noteid-0x10)/12.0f))/(float)song->mainrate;
  
  event->soundid=track->instrument;
  
  return 0;
}

/* Decode end of chart block.
 * Any notes we've placed that we haven't terminated yet, do that now.
 */
 
static int ttaq_song_decode_end_of_block(struct ttaq_song *song,struct ttaq_track *trackv,int trackc) {
  int i=1; for (;i<trackc;i++) {
    if (trackv[i].sustainp<0) continue;
    ttaq_song_release_event(song,song->eventv+trackv[i].sustainp);
    trackv[i].sustainp=-1;
  }
  return 0;
}

/* Decode a block of chart lines.
 */
 
struct ttaq_chart {
  const char *v;
  int c;
};
 
static int ttaq_song_add_chart_block(struct ttaq_song *song,const struct ttaq_chart *chartv,int chartc) {
  if (chartc<1) return 0;
  if (!song->tempo) {
    fprintf(stderr,"Song must declare tempo before first chart block.\n");
    return -1;
  }
  
  /* Blocks look like this:
   *             >   >   >   >   >   >   >   >   >   >   >   >   >   >   >   >   >   >   >   >   >   >   >   >
   * 6: 50@-30   c3  !   eb3     g#3     c4      g3      eb3     d3      eb3     g3      c4      g3      eb3
   *  : 80@20    3                                               3                       3
   *  : 50@0     0                                               0                       0       1       1   1
   *
   * First line establishes timing, each '>' is a beat.
   * Subsequent lines start with: 
   *   [INSTRUMENT] : LEVEL @ PAN
   * When INSTRUMENT is omitted, the chart contains fire-and-forget drum notes.
   * Otherwise, it's musical note names and '!' for explicit release.
   */
   
  #define charta 16
  if (chartc>charta) return -1;
  struct ttaq_track headerv[charta]; // First entry is blank, it's the timing track. Indices match (chartv).
  
  int srcp=0; // position in all strings of (chartv).
  while ((srcp<chartv[0].c)&&((unsigned char)chartv[0].v[srcp]<=0x20)) srcp++;
  int i=1; for (;i<chartc;i++) {
    int len=srcp;
    if (len>chartv[i].c) len=chartv[i].c;
    if (ttaq_song_decode_track_header(headerv+i,chartv[i].v,len)<0) {
      return -1;
    }
  }
  
  /* Read along all chart lines in sync, and expect a note at each '>' in the top line.
   */
  while (srcp<chartv[0].c) {
    if ((unsigned char)chartv[0].v[srcp]<=0x20) { srcp++; continue; }
    if (chartv[0].v[srcp]!='>') return -1;
    for (i=1;i<chartc;i++) {
      if (ttaq_song_decode_note(song,headerv+i,chartv[i].v+srcp,chartv[i].c-srcp)<0) return -1;
    }
    if (ttaq_song_next_beat(song)<0) return -1;
    srcp++;
  }
  
  if (ttaq_song_decode_end_of_block(song,headerv,chartc)<0) return -1;
  
  #undef charta
  return 0;
}

/* Finish decode: Validate what we got.
 */
 
static int ttaq_song_decode_finish(struct ttaq_song *song,int songid) {
  if (song->eventc<1) {
    fprintf(stderr,"song %d is empty!\n",songid);
    return -1;
  }
  // Add one more no-op event, to ensure we capture the end of time correctly.
  struct ttaq_song_event *event=ttaq_song_add_event(song);
  if (!event) return -1;
  event->soundid=-1;
  return 0;
}

/* Decode new song.
 */
 
static int ttaq_song_decode(struct ttaq_song *song,const char *src,int srcc,int songid) {
  
  #define charta 16 /* arbitrary track limit */
  struct ttaq_chart chartv[charta];
  int chartc=0;
  
  int srcp=0,lineno=0;
  while (srcp<srcc) {
    lineno++;
    const char *line=src+srcp;
    int linec=0,comment=0;
    while (srcp<srcc) {
      if (src[srcp]==';') comment=1;
      if (src[srcp++]==0x0a) break;
      if (!comment) linec++;
    }
    while (linec&&((unsigned char)line[linec-1]<=0x20)) linec--;
    
    if ((linec>=10)&&!memcmp(line,"instrument",10)) {
      if (ttaq_song_decode_instrument(song,line,linec)<0) {
        fprintf(stderr,"song %d, line %d: Error processing instrument line.\n",songid,lineno);
        return -1;
      }
      continue;
    }
    
    if ((linec>=5)&&!memcmp(line,"tempo",5)) {
      if (ttaq_song_decode_tempo(song,line,linec)<0) {
        fprintf(stderr,"song %d, line %d: Error processing tempo line.\n",songid,lineno);
        return -1;
      }
      continue;
    }
    
    if (linec) {
      if (chartc>=charta) {
        fprintf(stderr,"song %d, line %d: Too many tracks in chart block.\n",songid,lineno);
        return -1;
      }
      struct ttaq_chart *chart=chartv+chartc++;
      chart->v=line;
      chart->c=linec;
    } else {
      if (ttaq_song_add_chart_block(song,chartv,chartc)<0) {
        fprintf(stderr,"song %d, line %d: Error adding chart block.\n",songid,lineno);
        return -1;
      }
      chartc=0;
    }
  }
  if (chartc&&(ttaq_song_add_chart_block(song,chartv,chartc)<0)) {
    fprintf(stderr,"song %d, line %d: Error adding chart block.\n",songid,lineno);
    return -1;
  }
  #undef charta
  return ttaq_song_decode_finish(song,songid);
}

/* New.
 */
 
struct ttaq_song *ttaq_song_new(const void *v,int c,int mainrate,int songid) {
  struct ttaq_song *song=calloc(1,sizeof(struct ttaq_song));
  if (!song) return 0;
  song->mainrate=mainrate;
  if (ttaq_song_decode(song,v,c,songid)<0) {
    ttaq_song_del(song);
    return 0;
  }
  return song;
}

/* Reset.
 */

void ttaq_song_reset(struct ttaq_song *song) {
  song->eventp=0;
  song->time=0;
}

/* Update.
 */

int ttaq_song_update(struct ttaq_song_event *event,struct ttaq_song *song) {
  if (song->eventp>=song->eventc) {
    song->eventp=0;
    song->time=0;
    return 1; // never zero, just in case the song ended up empty or something
  }
  int delay=song->eventv[song->eventp].time-song->time;
  if (delay>0) return delay;
  *event=song->eventv[song->eventp];
  song->eventp++;
  return 0;
}

/* Advance clock.
 */

void ttaq_song_advance(struct ttaq_song *song,int framec) {
  song->time+=framec;
}
