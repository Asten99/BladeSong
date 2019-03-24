/* 
 * Copyright (c) 1982, 1986, 1990, 1991, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * This software was written by Christian Fuchs (christian_fuchs@me.com)
 * and is being distributed under the BSD license. Please note that the
 * used libraries (Razer switchblade SDK and Apple iTunes COM interface
 * have their own licenses and thus cannot be distributed in the source
 * package of this product.
 *
 * Many thanks to my dear friend who helped me understand C image functions
 * when I was totally lost and a very gentle being at Razer who supplied
 * me with the SDK when it was no longer in support. Without you all this
 * would never have been possible! Thank you!!
 *
 */

#pragma once

#include <winapifamily.h>

#include <string.h> 
#include <math.h>
#include <TCHAR.h>  

#include <comdef.h>
#include <comutil.h>

#include "SwitchBlade.h"
#include "SwitchBladeSDK_types.h"

#include "iTunesCOMInterface.h"
#include "iTunesCOMInterface_i.c"

#include "resource.h"

#define APPSTATE_STARTUP 0						// default application state (unknown/loading state)
#define APPSTATE_CONTROLS_PLAY 1				// the itunes play controls with play button are displayed by the application
#define APPSTATE_CONTROLS_PAUSE 2				// the itunes play controls with pause button are displayed by the application
#define APPSTATE_PLAYLIST 3						// playlists are displayed by the application
#define APPSTATE_PLAYLIST_START 4				// list of all playlists is to be displayed by the application
#define PL_STATE_UNDEFINED 5					// state of an internal representation of a playlist - unknown
#define PL_STATE_NOT_LOADED 6					// state of an internal representation of a playlist - uninitalized
#define PL_STATE_LOADING 7						// state of an internal representation of a playlist - playlist information set, loading individual tracks
#define PL_STATE_SORTING 8						// state of an internal representation of a playlist - all data loaded, sorting tracks by name
#define PL_STATE_READY 9						// state of an internal representation of a playlist - all data loaded, sorting finished, ready for usage

#define SB_THEME_SWTOR_FGCOL_R 212
#define SB_THEME_SWTOR_FGCOL_G 175
#define SB_THEME_SWTOR_FGCOL_B 55
#define SB_THEME_SWTOR_BGCOL_R 147
#define SB_THEME_SWTOR_BGCOL_G 20
#define SB_THEME_SWTOR_BGCOL_B 0

#define SB_THEME_DSTALKER_FGCOL_R 83
#define SB_THEME_DSTALKER_FGCOL_G 144
#define SB_THEME_DSTALKER_FGCOL_B 83
#define SB_THEME_DSTALKER_BGCOL_R 0
#define SB_THEME_DSTALKER_BGCOL_G 167
#define SB_THEME_DSTALKER_BGCOL_B 0

#define SB_THEME_BLADE_FGCOL_R 60
#define SB_THEME_BLADE_FGCOL_G 60
#define SB_THEME_BLADE_FGCOL_B 73
#define SB_THEME_BLADE_BGCOL_R 45
#define SB_THEME_BLADE_BGCOL_G 45
#define SB_THEME_BLADE_BGCOL_B 55

#define SB_THEME_SWTOR 99
#define SB_THEME_DSTALKER 100
#define SB_THEME_BLADE 101

struct trackData {
	LPTSTR name;
	long TrackID;
	long TrackDatabaseID;
};

struct playlistData {
	LPTSTR name;
	long membercount;
	long PlaylistID;
	long SourceID;
	short loadState;
	long internalID;
	trackData **tracks;
};

HRESULT STDMETHODCALLTYPE AppEventHandler(RZSBSDK_EVENTTYPETYPE, DWORD, DWORD);
HRESULT STDMETHODCALLTYPE TouchPadHandler_Controls(RZSBSDK_GESTURETYPE, DWORD, WORD, WORD, WORD);
HRESULT STDMETHODCALLTYPE TouchPadHandler_Playlist(RZSBSDK_GESTURETYPE, DWORD, WORD, WORD, WORD);
HRESULT STDMETHODCALLTYPE DynamicKeyHandler(RZSBSDK_DKTYPE, RZSBSDK_KEYSTATETYPE);

bool pause_iTunes(IiTunes*);
bool start_iTunes(IiTunes*);
bool next_iTunes(IiTunes*);
bool prev_iTunes(IiTunes*);
bool iTunes_song_is_playing(IiTunes*);
LPCWSTR getTrack_iTunes(IiTunes*);
HRESULT getPlaylists_iTunes(IiTunes*);

HRESULT padTap(WORD, WORD);						// tapping on UI (playing a song if touched/clicked)
HRESULT padFlick(WORD);							// scrolling the UI on flicking it
HRESULT padMove(WORD);							// scrolling the UI on moving your finger on it for a very short duration 
HRESULT play_song_on_playlist(long, WORD);		// calculates wether user tapped a playlist or song (and which one) and either makes calls to display the selected playlist or play the song
HRESULT initSwitchbladeControls();
HRESULT setAppState(short);
HRESULT showiTunesControlInterface();
HRESULT showiTunesPlaylistInterface();
HRESULT refreshiTunesPlayList();
DWORD WINAPI getiTunesPlaylist(LPVOID);			// playlist iTunes COM enumeration thread
DWORD WINAPI scrollUI(LPVOID);					// playlist browser scroll thread
DWORD WINAPI flickTimer(LPVOID);				// immunity timer during flick for move gestures
int trackComp(const void *t1, const void *t2);	// compares two tracks along their names and returns wich is to be ordered first for sorting (qsort)
HRESULT preloadResources();						// preloads Resources according to selected theme
HRESULT storeLastUITapCoord(WORD, WORD);		// stores the last point of touch contact on the display to calculate moving on the display
												// generic bitmap handle function for SBUI output
HRESULT drawSBImage(RZSBSDK_DISPLAY, HBITMAP, RZSBSDK_BUFFERPARAMS);



HANDLE *threadlist_getplaylists;				// thread handles for iTunes playlist retrieval
HANDLE scrollthread;							// thread handle for display scrolling
HANDLE scrollimmunitytimerthread;				// thread for scroll handling

const short fontsize = 23;						// playlist font size
const short spacing = 4;						// playlist padding
const size_t max_chars_per_line = 32;			// char line length of the sbui display in the specified font / size

HINSTANCE hInst;                                // program instance
IiTunes* myITunes;								// handle to iTunes COM object global variable - needed for touch hanndling callback function 
playlistData **allSongs;						// internal structure to hold all playlists and titles
short applicationstate;							// what the application is to display to the user (ie control mode, playlist mode)
short selected_playlist;						// which playlist has been selected to display by the user; to be conform to Apple, this start with 1. 0 is the list of all playlists
long scroll_offset;								// offset used to determine the scrolled state to calculate the correct view portion of larger images to show
long o_image_lines;							// offscreen ui image lines number
long num_playlists;								// how many playlists we import form iTunes (all in the library)
HDC hdcOffscreenDC;								// device context handle for offscreen rendering of the whole playlist
HBITMAP h_offscreen;							// bitmap handle for the offscreen image
void *o_pixbuf;									// memory for the actual offscreen image
HANDLE o_h_pixbuf;								// handle to said memory
bool continue_scrolling;						// flag to tell the scrolling thread to stop
bool moving;									// flag to tell the ui is scrolling
HICON AppIcon;									// full-size App Icon
HICON AppIconSM;								// small-size App Icon
HBITMAP hbutton_controls;						// show controls button image
HBITMAP hbutton_exit;							// exit button image
HBITMAP hbutton_playlist;						// show playlists button image
HBITMAP hcontrols_pause;						// controls UI image - playback paused
HBITMAP hcontrols_play;							// controls UI image - playback commencing
WORD old_x;										// previous x axis coordinate of last touch point of user interaction on UI
WORD old_y;										// previous y axis coordinate of last touch point of user interaction on UI
BITMAPINFOHEADER bmi_offscreen;					// Handle to said image
bool flick_in_progress;							// to check wether user initated a flick - we do not want to move the UI during that
RZSBSDK_BUFFERPARAMS sbuidisplay;				// structure to implement SBUI drawing
byte hwtheme;									// we parse this from command line, decides which theme to load