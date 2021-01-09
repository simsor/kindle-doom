/* Emacs style mode select   -*- C++ -*-
 *-----------------------------------------------------------------------------
 *
 *
 *  PrBoom: a Doom port merged with LxDoom and LSDLDoom
 *  based on BOOM, a modified and improved DOOM engine
 *  Copyright (C) 1999 by
 *  id Software, Chi Hoang, Lee Killough, Jim Flynn, Rand Phares, Ty Halderman
 *  Copyright (C) 1999-2006 by
 *  Jess Haas, Nicolas Kalkhof, Colin Phipps, Florian Schulze
 *  Copyright 2005, 2006 by
 *  Florian Schulze, Colin Phipps, Neil Stevens, Andrey Budko
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 *  02111-1307, USA.
 *
 * DESCRIPTION:
 *  DOOM graphics stuff for SDL
 *
 *-----------------------------------------------------------------------------
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#include <errno.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#endif


#include "m_argv.h"
#include "doomstat.h"
#include "doomdef.h"
#include "doomtype.h"
#include "v_video.h"
#include "r_draw.h"
#include "d_main.h"
#include "d_event.h"
#include "i_joy.h"
#include "i_video.h"
#include "z_zone.h"
#include "s_sound.h"
#include "sounds.h"
#include "w_wad.h"
#include "st_stuff.h"
#include "lprintf.h"

int gl_colorbuffer_bits=16;
int gl_depthbuffer_bits=16;

extern void M_QuitDOOM(int choice);
#ifdef DISABLE_DOUBLEBUFFER
int use_doublebuffer = 0;
#else
int use_doublebuffer = 1; // Included not to break m_misc, but not relevant to SDL
#endif
int use_fullscreen;
int desired_fullscreen;
static byte *screen;
static int fbFd;
static int kindleKeysFd;
static int kindleKeysFd2;

typedef struct {
  int truc1;
	int truc2;
	unsigned short truc3;
  unsigned short keyCode;
  int status;
} kindle_key_t;

static kindle_key_t kkey;

#define KINDLE_WIDTH 800
#define KINDLE_HEIGHT 600

/////////// FRAMEBUFFER STUFF
static void init_framebuffer() {
  fbFd = open("/dev/fb0", O_RDWR);
  if (fbFd == -1) {
    perror("open(/dev/fb0)");
    exit(1);
  }
}

static void update_framebuffer(int full) {
  if (full) {
    ioctl(fbFd, 0x46db, 1);
  } else {
    ioctl(fbFd, 0x46db, 0);
  }
}

static void clear_framebuffer() {
  ioctl(fbFd, 0x46e1, 0);  
}


////// KINDLE EVENTS

static void init_kindle_keys() {
  kindleKeysFd = open("/dev/input/event1", O_RDONLY|O_NONBLOCK);
  if (kindleKeysFd == -1) {
    perror("open(/dev/input/event1)");
    exit(1);
  }

  kindleKeysFd2 = open("/dev/input/event0", O_RDONLY|O_NONBLOCK);
  if (kindleKeysFd2 == -1) {
    perror("open(/dev/input/event0)");
    exit(1);
  }
}

static int kindle_poll_keys(kindle_key_t *k) {
    int retval = read(kindleKeysFd, k, sizeof(kindle_key_t));
    if (retval == -1) {
      if (errno != EAGAIN) {
        perror("read()");
        exit(1);
      }
    } else if (retval > 0)  {
      return 1;
    }

  // This SHOULD NOT WORK: /dev/input/event0 doesn't have the same structure as event1
  // For some reason it matches for the HOME key, but it's a kludge and doesn't allow us to detect the other buttons
    retval = read(kindleKeysFd2, k, sizeof(kindle_key_t));
    if (retval == -1) {
      if (errno != EAGAIN) {
        perror("read()");
        exit(1);
      }
    } else if (retval > 0)  {
      return 1;
    }

    return 0;
}

////////////////////////////////////////////////////////////////////////////
// Input code
int             leds_always_off = 0; // Expected by m_misc, not relevant

// Mouse handling
extern int     usemouse;        // config file var
static boolean mouse_enabled; // usemouse, but can be overriden by -nomouse
static boolean mouse_currently_grabbed;

/////////////////////////////////////////////////////////////////////////////////
// Keyboard handling

//
//  Translates the key currently in key
//

#define KINDLE_LEFT 105
#define KINDLE_RIGHT 106
#define KINDLE_UP 103
#define KINDLE_DOWN 108
#define KINDE_OK 194
#define KINDLE_HOME 102
#define KINDLE_MENU 139
#define KINDLE_BACK 158

static int I_TranslateKey(unsigned short code)
{
  int rc = 0;

  switch (code) {
    case KINDLE_LEFT: rc = KEYD_DOWNARROW; break;
    case KINDLE_RIGHT: rc = KEYD_UPARROW; break;
    case KINDLE_UP: rc = KEYD_LEFTARROW; break;
    case KINDLE_DOWN: rc = KEYD_RIGHTARROW; break;
    case KINDE_OK: rc = KEYD_RCTRL; break;
    default: break;
  }


  return rc;

}

/////////////////////////////////////////////////////////////////////////////////
// Main input code

/* cph - pulled out common button code logic */
static int I_SDLtoDoomMouseState(int buttonstate)
{
  return 0;
}

static void I_GetEvent(kindle_key_t k)
{
  event_t event;

  if (k.keyCode == 102 && k.status == 1) {
    // Press HOME
    exit(0);
  }

  switch (k.status) {
  case 1: // SDL_KEYDOWN
    event.type = ev_keydown;
    event.data1 = I_TranslateKey(k.keyCode);
    D_PostEvent(&event);
    break;

  case 0: // SDL_KEYUP
  {
    event.type = ev_keyup;
    event.data1 = I_TranslateKey(k.keyCode);
    D_PostEvent(&event);
  }
  break;    

  default:
    break;
  }
}


//
// I_StartTic
//

void I_StartTic (void)
{
  while ( kindle_poll_keys(&kkey)) {
    //printf("Keycode: %d // Status: %d\n", kkey.keyCode, kkey.status);
    I_GetEvent(kkey);
  }
}

//
// I_StartFrame
//
void I_StartFrame (void)
{
}

//
// I_InitInputs
//

static void I_InitInputs(void)
{
  init_kindle_keys();
}
/////////////////////////////////////////////////////////////////////////////

// I_SkipFrame
//
// Returns true if it thinks we can afford to skip this frame

inline static boolean I_SkipFrame(void)
{
  static int frameno;

  frameno++;
  switch (gamestate) {
  case GS_LEVEL:
    if (!paused)
      return false;
  default:
    // Skip odd frames
    return (frameno & 1) ? true : false;
  }
}

///////////////////////////////////////////////////////////
// Palette stuff.
//
static void I_UploadNewPalette(int pal)
{
}

//////////////////////////////////////////////////////////////////////////////
// Graphics API

void I_ShutdownGraphics(void)
{
}

//
// I_UpdateNoBlit
//
void I_UpdateNoBlit (void)
{
}

//
// I_FinishUpdate
//
static int newpal = 0;
#define NO_PALETTE_CHANGE 1000

void I_FinishUpdate (void)
{
  if (I_SkipFrame()) return;

  for (int x=0; x < SCREENWIDTH; x++) {
    for (int y=0; y < SCREENHEIGHT; y++) {
      screen[x*600+(599-y)] = ~screens[0].data[y*SCREENWIDTH+x];
    }
  }

  update_framebuffer(0);
}

//
// I_ScreenShot - moved to i_sshot.c
//

//
// I_SetPalette
//
void I_SetPalette (int pal)
{
  newpal = pal;
}

// I_PreInitGraphics

static void I_ShutdownSDL(void)
{
  clear_framebuffer();
  close(fbFd);
  return;
}

void I_PreInitGraphics(void)
{
  init_framebuffer();

 atexit(I_ShutdownSDL);
}

// e6y
// GLBoom use this function for trying to set the closest supported resolution if the requested mode can't be set correctly.
// For example glboom.exe -geom 1025x768 -nowindow will set 1024x768.
// It should be used only for fullscreen modes.
static void I_ClosestResolution (int *width, int *height, int flags)
{
  *width=KINDLE_WIDTH;
  *height=KINDLE_HEIGHT;
}  

// CPhipps -
// I_CalculateRes
// Calculates the screen resolution, possibly using the supplied guide
void I_CalculateRes(unsigned int width, unsigned int height)
{
  // e6y: how about 1680x1050?
  /*
  SCREENWIDTH = (width+3) & ~3;
  SCREENHEIGHT = (height+3) & ~3;
  */

// e6y
// GLBoom will try to set the closest supported resolution 
// if the requested mode can't be set correctly.
// For example glboom.exe -geom 1025x768 -nowindow will set 1024x768.
// It affects only fullscreen modes.
  if (V_GetMode() == VID_MODEGL) {
    if ( desired_fullscreen )
    {
      I_ClosestResolution(&width, &height, 0);
    }
    SCREENWIDTH = width;
    SCREENHEIGHT = height;
    SCREENPITCH = SCREENWIDTH;
  } else {
    SCREENWIDTH = KINDLE_WIDTH;
    SCREENHEIGHT = KINDLE_HEIGHT;
    if (!(SCREENWIDTH % 1024)) {
      SCREENPITCH = SCREENWIDTH*V_GetPixelDepth()+32;
    } else {
      SCREENPITCH = SCREENWIDTH*V_GetPixelDepth();
    }
  }
}

// CPhipps -
// I_SetRes
// Sets the screen resolution
void I_SetRes(void)
{
  int i;

  I_CalculateRes(SCREENWIDTH, SCREENHEIGHT);

  // set first three to standard values
  for (i=0; i<3; i++) {
    screens[i].width = SCREENWIDTH;
    screens[i].height = SCREENHEIGHT;
    screens[i].byte_pitch = SCREENPITCH;
    screens[i].short_pitch = SCREENPITCH / V_GetModePixelDepth(VID_MODE16);
    screens[i].int_pitch = SCREENPITCH / V_GetModePixelDepth(VID_MODE32);
  }

  // statusbar
  screens[4].width = SCREENWIDTH;
  screens[4].height = (ST_SCALED_HEIGHT+1);
  screens[4].byte_pitch = SCREENPITCH;
  screens[4].short_pitch = SCREENPITCH / V_GetModePixelDepth(VID_MODE16);
  screens[4].int_pitch = SCREENPITCH / V_GetModePixelDepth(VID_MODE32);

  lprintf(LO_INFO,"I_SetRes: Using resolution %dx%d\n", SCREENWIDTH, SCREENHEIGHT);
}

void I_InitGraphics(void)
{
  char titlebuffer[2048];
  static int    firsttime=1;

  if (firsttime)
  {
    firsttime = 0;

   atexit(I_ShutdownGraphics);
    lprintf(LO_INFO, "I_InitGraphics: %dx%d\n", SCREENWIDTH, SCREENHEIGHT);

    /* Set the video mode */
    I_UpdateVideoMode();

    /* Setup the window title */
    strcpy(titlebuffer,PACKAGE);
    strcat(titlebuffer," ");
    strcat(titlebuffer,VERSION);
    printf("Title: %s\n", titlebuffer);

    /* Initialize the input system */
    I_InitInputs();
  }
}

int I_GetModeFromString(const char *modestr)
{
  video_mode_t mode;


  mode = VID_MODE8;

  return mode;
}

void I_UpdateVideoMode(void)
{
  int init_flags;
  int i;
  video_mode_t mode;

  lprintf(LO_INFO, "I_UpdateVideoMode: %dx%d (%s)\n", SCREENWIDTH, SCREENHEIGHT, desired_fullscreen ? "fullscreen" : "nofullscreen");

  mode = I_GetModeFromString(default_videomode);
  if ((i=M_CheckParm("-vidmode")) && i<myargc-1) {
    mode = I_GetModeFromString(myargv[i+1]);
  }

  V_InitMode(mode);
  V_DestroyUnusedTrueColorPalettes();
  V_FreeScreens();

  I_SetRes();


  screen = mmap(NULL, 600 * 800, PROT_READ|PROT_WRITE, MAP_SHARED|MAP_LOCKED, fbFd, 0);

  if(screen == NULL) {
    I_Error("Couldn't set %dx%d video mode", SCREENWIDTH, SCREENHEIGHT);
  }

  lprintf(LO_INFO, "I_UpdateVideoMode: 0x%x, %s, %s\n", init_flags, "", "");

  mouse_currently_grabbed = false;

  // Get the info needed to render to the display

    byte* screen_data = (byte*)malloc(800*600);
    screens[0].not_on_heap = true;
    screens[0].data = (unsigned char *) (screen_data);
    screens[0].byte_pitch = KINDLE_WIDTH;
    screens[0].short_pitch = KINDLE_WIDTH; // screen->pitch / V_GetModePixelDepth(VID_MODE16);
    screens[0].int_pitch = KINDLE_WIDTH; // screen->pitch / V_GetModePixelDepth(VID_MODE32);


  V_AllocScreens();

  // Hide pointer while over this window
  //SDL_ShowCursor(0);

  R_InitBuffer(SCREENWIDTH, SCREENHEIGHT);

}
