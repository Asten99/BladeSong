#pragma once

#include <winapifamily.h>

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

const LPWSTR image_play_controls = (LPWSTR)L"..\\res\\controls_play.png";
const LPWSTR image_pause_controls = (LPWSTR)L"..\\res\\controls_pause.png";
const LPWSTR image_button_exit = (LPWSTR)L"..\\res\\button_exit.png";
const LPWSTR image_button_controls = (LPWSTR)L"..\\res\\button_controls.png";
const LPWSTR image_button_list = (LPWSTR)L"..\\res\\button_list.png";
const LPWSTR image_playlist_songs = (LPWSTR)L"..\\res\\feature_missing.png";

struct trackData {
	LPTSTR name;
	IITTrack *iTunesObjectRef;
};

struct playlistData {
	LPTSTR name;
	long membercount;
	IITPlaylist *iTunesObjectRef;
	trackData **tracks;
};

HRESULT STDMETHODCALLTYPE AppEventHandler(RZSBSDK_EVENTTYPETYPE, DWORD, DWORD);
HRESULT STDMETHODCALLTYPE TouchPadHandler(RZSBSDK_GESTURETYPE, DWORD, WORD, WORD, WORD);
HRESULT STDMETHODCALLTYPE DynamicKeyHandler(RZSBSDK_DKTYPE, RZSBSDK_KEYSTATETYPE);

bool pause_iTunes(IiTunes*);
bool start_iTunes(IiTunes*);
bool next_iTunes(IiTunes*);
bool prev_iTunes(IiTunes*);
bool iTunes_song_is_playing(IiTunes*);
LPCWSTR getTrack_iTunes(IiTunes*);
HRESULT getPlaylists_iTunes(IiTunes* , playlistData **);

HRESULT padTap(WORD, WORD);
HRESULT initSwitchbladeControls();
HRESULT setAppState(short);
HRESULT showiTunesControlInterface();
HRESULT showiTunesPlaylistInterface();

