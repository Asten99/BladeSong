#pragma once

#include <winapifamily.h>

#include <string.h>  
#include <TCHAR.h>  

#include <comdef.h>
#include <comutil.h>

#include "SwitchBlade.h"
#include "SwitchBladeSDK_types.h"

#include "iTunesCOMInterface.h"
#include "iTunesCOMInterface_i.c"

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

const LPWSTR image_play_controls = (LPWSTR)L"..\\res\\controls_play.png";
const LPWSTR image_pause_controls = (LPWSTR)L"..\\res\\controls_pause.png";
const LPWSTR image_button_exit = (LPWSTR)L"..\\res\\button_exit.png";
const LPWSTR image_button_controls = (LPWSTR)L"..\\res\\button_controls.png";
const LPWSTR image_button_list = (LPWSTR)L"..\\res\\button_list.png";
const LPWSTR image_playlist_songs = (LPWSTR)L"..\\res\\feature_missing.png";

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

HRESULT padTap(WORD, WORD);
HRESULT padFlick(WORD);
HRESULT play_song_on_playlist(long, WORD);
HRESULT initSwitchbladeControls();
HRESULT setAppState(short);
HRESULT showiTunesControlInterface();
HRESULT showiTunesPlaylistInterface();
HRESULT refreshiTunesPlayList();
DWORD WINAPI getiTunesPlaylist(LPVOID);			// playlist iTunes COM enumeration thread
DWORD WINAPI scrollUI(LPVOID);					// playlist browser scroll thread
int trackComp(const void *t1, const void *t2);	// compares two tracks along their names and returns wich is to be ordered first for sorting (qsort)


const short fontsize = 17;						// playlist font size
const short spacing = 5;						// playlist padding
const size_t max_chars_per_line = 32;			// char line length of the sbui display in the specified font / size

HINSTANCE hInst;                                // program instance
IiTunes* myITunes;								// handle to iTunes COM object global variable - needed for touch hanndling callback function 
playlistData **allSongs;						// internal structure to hold all playlists and titles
short applicationstate;							// what the application is to display to the user (ie control mode, playlist mode)
short selected_playlist;						// which playlist has been selected to display by the user; to be conform to Apple, this start with 1. 0 is the list of all playlists
long scroll_offset;								// offset used to determine the scrolled state to calculate the correct view portion of larger images to show
long num_playlists;								// how many playlists we import form iTunes (all in the library)
HDC hdcOffscreenDC;								// device context handle for offscreen rendering of the whole playlist
HBITMAP h_offscreen;							// bitmap handle for the offscreen image
void *o_pixbuf;									// memory for the actual offscreen image
HANDLE o_h_pixbuf;								// handle to said memory
bool continue_scrolling;						// flag to tell the scrolling thread to stop

BITMAPINFOHEADER bmi_offscreen;				// Handle to said image

RZSBSDK_BUFFERPARAMS sbuidisplay;			// structure to implement SBUI drawing