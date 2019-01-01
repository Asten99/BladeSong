#include "BladeSong.h"

HINSTANCE hInst;                                // program instance
IiTunes* myITunes;								// handle to iTunes COM object global variable - needed for touch hanndling callback function 
short applicationstate;

HRESULT connectiTunes() {
	CoInitialize(NULL);								// start COM connectivity
	/* attach to iTunes via COM */
	return (::CoCreateInstance(CLSID_iTunesApp, NULL, CLSCTX_LOCAL_SERVER, IID_IiTunes, (PVOID *)&myITunes));
}

HRESULT disconnectiTunes() {
	myITunes->Release();							// release iTunes COM object hold by this app
	RzSBStop();										// release switchblade control
	CoUninitialize();								// stop COM connectivity
	OutputDebugString((LPCSTR)L"\n\niTunes interface disconnected!\n");
	return S_OK;
}

bool pause_iTunes(IiTunes* iITunes) {			// sends pause command to iTunes COM object
	iITunes->Release();
	return iITunes->Pause() == ERROR_SUCCESS ? true : false;
}

bool start_iTunes(IiTunes* iITunes) {			// sends start command to iTunes COM object
	iITunes->Release();
	return iITunes->Play() == ERROR_SUCCESS ? true : false;
}

bool next_iTunes(IiTunes* iITunes) {			// sends next song command to iTunes COM object
	iITunes->Release();
	return iITunes->NextTrack() == ERROR_SUCCESS ? true : false;
}

bool prev_iTunes(IiTunes* iITunes) {			// sends previous song command to iTunes COM object
	iITunes->Release();
	return iITunes->PreviousTrack() == ERROR_SUCCESS ? true : false;
}

bool iTunes_song_is_playing(IiTunes* iITunes) {	// returns true if a song is currently playing
	ITPlayerState playButtonState;
	HRESULT errCode = S_OK;
	iITunes->Release();
	if (iITunes->get_PlayerState(&playButtonState) == S_OK)
		if (playButtonState == ITPlayerStatePlaying)
			return true;						// song is playing
	return false;								// song either not playing or error communicating with iTunes COM object
}

LPCWSTR getTrack_iTunes(IiTunes* iITunes) {		// return current track from iTunes COM object as a string
	HRESULT errCode;
	IITTrack* trackInfo;
	BSTR trackName;
	_bstr_t bb;
	iITunes->Release();
	errCode = iITunes->get_CurrentTrack(&trackInfo);
	if (errCode == S_OK) {
		errCode = trackInfo->get_Name(&trackName);
		if (errCode == S_OK) {
			bb = trackName;
			return bb;
		}
	}
	return NULL;
}
HRESULT padTap(WORD x, WORD y) {				// handles switchblade touch input for player controls
	HRESULT retval = S_OK;
	if (x > 4 && x < 275) {						// left --> previous song
		myITunes->AddRef();
		prev_iTunes(myITunes);
	}
	if (x > 280 && x < 515) {					// middle --> pause or play
		myITunes->AddRef();
		switch (applicationstate) {
		case APPSTATE_CONTROLS_PLAY:
			start_iTunes(myITunes);
			setAppState(APPSTATE_CONTROLS_PAUSE);
			break;
		case APPSTATE_CONTROLS_PAUSE:
			pause_iTunes(myITunes);
			setAppState(APPSTATE_CONTROLS_PLAY);
			break;
		default:
			break;
		}
	}
	if (x > 520 && x < 690) {					// right --> next song
		myITunes->AddRef();
		next_iTunes(myITunes);
	}
	return retval;
}

HRESULT initSwitchbladeControls() {				// paint common user interface
	HRESULT retval = S_OK;
	/* set up the buttons for the control and playlist interface as well as an exit button */
	retval = RzSBSetImageDynamicKey(RZSBSDK_DK_5, RZSBSDK_KEYSTATE_UP, image_button_exit);
	retval = RzSBSetImageDynamicKey(RZSBSDK_DK_6, RZSBSDK_KEYSTATE_UP, image_button_controls);
	retval = RzSBSetImageDynamicKey(RZSBSDK_DK_7, RZSBSDK_KEYSTATE_UP, image_button_list);
	/* disable handing over touchpad gestures to OS - disables "mouse" functionality */
	retval = RzSBEnableOSGesture(RZSBSDK_GESTURE_ALL, false);
	/* register callback function to handle touch input to application */
	retval = RzSBGestureSetCallback(reinterpret_cast<TouchpadGestureCallbackFunctionType>(TouchPadHandler));
	/* register callback function to handle button presses */
	retval = RzSBDynamicKeySetCallback(reinterpret_cast<DynamicKeyCallbackFunctionType>(DynamicKeyHandler));
	/* register callback function to handle application lifecycle events */
	retval = RzSBAppEventSetCallback(reinterpret_cast<AppEventCallbackType>(AppEventHandler));
	return retval;
}

HRESULT setAppState(short appstate) {
	HRESULT retval = S_OK;
	switch (appstate) {
	case APPSTATE_STARTUP:
		applicationstate = APPSTATE_STARTUP;
		initSwitchbladeControls();			// paint button interface
		showiTunesControlInterface();		// paint interface for itunes controls
		break;
	case APPSTATE_CONTROLS_PLAY:
		applicationstate = APPSTATE_CONTROLS_PLAY;
		retval = RzSBSetImageTouchpad(image_play_controls);
		break;
	case APPSTATE_CONTROLS_PAUSE:
		applicationstate = APPSTATE_CONTROLS_PAUSE;
		retval = RzSBSetImageTouchpad(image_pause_controls);
		break;
	case APPSTATE_PLAYLIST:
		applicationstate = APPSTATE_PLAYLIST;
		retval = RzSBSetImageTouchpad(image_playlist_songs);
	default:
		break;
	}
	return retval;
}

/* displays the interface to control iTunes playback on the switchblade touch screen */

HRESULT showiTunesControlInterface() {
	HRESULT retval;
	myITunes->AddRef();
	if (iTunes_song_is_playing(myITunes))		// check if iTunes is playing a song currently - then start up with controls UI with either play or pause showing, depending on iTunes state
		retval = setAppState(APPSTATE_CONTROLS_PAUSE);
	else
		retval = setAppState(APPSTATE_CONTROLS_PLAY);
	return retval;
}

/* displays the interface for playlist control on the switchblade touchscreen */

HRESULT showiTunesPlaylistInterface() {
	HRESULT retval;
	retval = S_OK;
	setAppState(APPSTATE_PLAYLIST);
	return retval;
}

/* Handles quit/deactive/close events sent by the switchblade environment */

HRESULT STDMETHODCALLTYPE AppEventHandler(RZSBSDK_EVENTTYPETYPE rzEvent, DWORD wParam, DWORD lParam) {
	HRESULT retval = S_OK;
	switch (rzEvent) {
	case RZSBSDK_EVENT_ACTIVATED:
		connectiTunes();
		setAppState(APPSTATE_STARTUP);
		break;
	case RZSBSDK_EVENT_DEACTIVATED:
		disconnectiTunes();
		break;
	case RZSBSDK_EVENT_CLOSE:
		PostQuitMessage(0);
		break;
	case RZSBSDK_EVENT_EXIT:
		PostQuitMessage(0);
		break;
	}
	return retval;
}

/* Button handling callback function */

HRESULT STDMETHODCALLTYPE DynamicKeyHandler(RZSBSDK_DKTYPE DynamicKey, RZSBSDK_KEYSTATETYPE DynamicKeystate) {
	HRESULT retval = S_OK;
	if (DynamicKeystate == RZSBSDK_KEYSTATE_DOWN)
		switch (DynamicKey) {
		case RZSBSDK_DK_5:						// exit button
			PostQuitMessage(0);
			break;
		case RZSBSDK_DK_6:						// show iTunes controls button
			showiTunesControlInterface();
			break;
		case RZSBSDK_DK_7:						// show playlist button
			showiTunesPlaylistInterface();
			break;
		default:
			break;
		}
		return retval;
}

/* Generic function to handle touchpad inpuit on the switchblade ui. */

HRESULT STDMETHODCALLTYPE TouchPadHandler(RZSBSDK_GESTURETYPE gesturetype, DWORD touchpoints, WORD x, WORD y, WORD z) {
	HRESULT retval = S_OK;
	switch (gesturetype) {
	case RZSBSDK_GESTURE_TAP:					// since we are only interested in taps by the user, we only check for the tap gesture
		retval = padTap(x, y);					// actual implementation of what happens when the user taps is done here, depening on the application state
		break;
	default:
		break;
	}
	return retval;
}

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPWSTR    lpCmdLine,
	_In_ int       nCmdShow)
{
	HRESULT hRes;
	MSG msg;

	hRes = S_OK;
	hRes = connectiTunes();
	if (hRes == S_OK) {
		if (RzSBStart() == RZSB_OK) {				// start up razer switchblade connectivity
			setAppState(APPSTATE_STARTUP);
		}
		else {
			int buttonretval;
			buttonretval = MessageBox(NULL, "Razer Switchblade hardware initalization error!", "Switchblade error", MB_OK | MB_ICONERROR | MB_DEFBUTTON1 | MB_APPLMODAL);
			OutputDebugStringW(L"Razer Switchblade hardware initalization error!\n");
			PostQuitMessage(0);							// since we cannot connect to switchblade, we quit
		}
		while (GetMessage(&msg, nullptr, 0, 0))			// windows application loop
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		disconnectiTunes();
	}
	else {
		int buttonretval;
		switch (hRes) {
		case REGDB_E_CLASSNOTREG:
			buttonretval = MessageBox(NULL, "Apple iTunes is not installed!", "iTunes error", MB_OK | MB_ICONERROR | MB_DEFBUTTON1 | MB_APPLMODAL);
			break;
		case CLASS_E_NOAGGREGATION:
		case E_NOINTERFACE:
		case E_POINTER:
		default:
			buttonretval = MessageBox(NULL, "Unknown error connecting to iTunes!", "iTunes error", MB_OK | MB_ICONERROR | MB_DEFBUTTON1 | MB_APPLMODAL);
		}
	}
	return (hRes);
}

/* Test function to output to the switchblade touchpad using RzSBRenderBuffer() - i fail to access the bitmap structure, however..*/

void printTextListeRefence(HWND hwnd, long songcount) {
	HDC hdc;
	HBITMAP uiimage;
	RECT bounds;
	bounds.left = 1L;
	bounds.top = 1L;
	bounds.right = 800L;
	bounds.bottom = 1600L;
	LPCSTR SongTitles = "Hello World";
	hdc = GetDC(hwnd);
	uiimage = CreateCompatibleBitmap(hdc, 800, 1600);
	FillRect(hdc, &bounds, (HBRUSH)(COLOR_BACKGROUND));
	DrawText(hdc, SongTitles, 11, &bounds, DT_LEFT & DT_TOP);
	size_t displayPixels = 800 * 480;
	RZSBSDK_BUFFERPARAMS display;
	memset(&display, 0, sizeof(RZSBSDK_BUFFERPARAMS));
	display.PixelType = RGB565;
	display.DataSize = (WORD)displayPixels * sizeof(WORD);
	BitBlt(hdc, 0, 0, 800, 480, hdc, 0, 0, DSTINVERT);
	RzSBRenderBuffer(RZSBSDK_DISPLAY_WIDGET, &display);
	DeleteObject(uiimage);
}