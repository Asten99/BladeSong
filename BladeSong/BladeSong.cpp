#include "BladeSong.h"



HRESULT connectiTunes() {
	CoInitialize(NULL);							// start COM connectivity
	/* attach to iTunes via COM */
	return (::CoCreateInstance(CLSID_iTunesApp, NULL, CLSCTX_LOCAL_SERVER, IID_IiTunes, (PVOID *)&myITunes));
}

HRESULT disconnectiTunes() {
	free(allSongs);								// release memory block holding song titles	
	myITunes->Release();						// release iTunes COM object hold by this app
	RzSBStop();									// release switchblade control
	CoUninitialize();							// stop COM connectivity
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
	iITunes->Release();
	errCode = iITunes->get_CurrentTrack(&trackInfo);
	if (errCode == S_OK) {
		errCode = trackInfo->get_Name(&trackName);
		if (errCode == S_OK) {
			return (LPCWSTR)trackName;
		}
	}
	return NULL;
}

HRESULT getPlaylists_iTunes(IiTunes* iITunes, playlistData **allSongs) {
	HRESULT errCode;
	IITSource *iTunesLibrary;
	IITPlaylistCollection *workingPlaylists;
	IITPlaylist *workingPlaylist;
	IITTrackCollection *workingTracks;
	IITTrack *workingTrack;
	BSTR workingPlayListName;
	BSTR workingTrackName;
	num_playlists = 0;
	long num_songs_per_playlist;
	errCode = S_OK;
	iITunes->Release();
	errCode = iITunes->get_LibrarySource(&iTunesLibrary);
	if (errCode != S_OK)
		return errCode;
	errCode = iTunesLibrary->get_Playlists(&workingPlaylists);
	if (errCode != S_OK)
		return errCode;
	errCode = workingPlaylists->get_Count(&num_playlists);
	if (errCode != S_OK)
		return errCode;
	allSongs = (playlistData **) malloc(num_playlists * sizeof(playlistData**));
	for (long i = 0; i < num_playlists; i = i + 1) {
		allSongs[i] = (playlistData *)malloc(sizeof(playlistData));
		errCode = workingPlaylists->get_Item(i + 1, &workingPlaylist);
		if (errCode != S_OK)
			return errCode;
		errCode = workingPlaylist->get_Tracks(&workingTracks);
		if (errCode != S_OK)
			return errCode;
		errCode = workingTracks->get_Count(&num_songs_per_playlist);
		if (errCode != S_OK)
			return errCode;
		errCode = workingPlaylist->get_Name(&workingPlayListName);
		if (errCode != S_OK)
			return errCode;
		allSongs[i]->membercount = num_songs_per_playlist;
		allSongs[i]->name = (LPTSTR)workingPlayListName;
		allSongs[i]->iTunesObjectRef = workingPlaylist;
		allSongs[i]->tracks = (trackData **)malloc(num_songs_per_playlist * sizeof(trackData));
		for (long j = 0; j < num_songs_per_playlist; j = j + 1) { // enum all tracks of this playlist, put it in allsongs[i]->tracks
				errCode = workingTracks->get_Item(j+1, &workingTrack);
				if (errCode != S_OK)
					return errCode;
				errCode = workingTrack->get_Name(&workingTrackName);
				if (errCode != S_OK)
					return errCode;
				allSongs[i]->tracks[j] = (trackData *)malloc(sizeof(trackData));
				allSongs[i]->tracks[j]->name = (LPTSTR)workingTrackName;
				allSongs[i]->tracks[j]->iTunesObjectRef = workingTrack;
		}
	}
	return errCode;
}

HRESULT drawPlaylistOffscreen(playlistData **allSongs, short playlist) { // playlist 0 = playlists, playlist 1 = first playlists, etc..
	HRESULT retval;
	long neededlines;
	const short fontsize = 17;
	const short spacing = 5;
	retval = S_OK;
	if (playlist > num_playlists)
		return E_FAIL;
	else {
		neededlines = allSongs[playlist]->membercount*(fontsize + spacing) + spacing;
		// allocate memory (moveable, fill with zeros): screenwidth * bpp, align to next 32 bit block * planes * lines
		o_h_pixbuf = GlobalAlloc(GHND, (800 * 16 + 31) / 32 * 1 * neededlines);
		// lock the memory, get a pointer to it
		o_pixbuf = GlobalLock(o_h_pixbuf);
		// get handle to an offscreen device context
		hdcOffscreenDC = CreateCompatibleDC(NULL);
		// create offscreen memory bitmap that will hold the full playlist
		h_offscreen = CreateBitmap(800, neededlines*(fontsize + spacing), 1, 16, o_pixbuf);
		// select offscreen image into device context
		SelectObject(hdcOffscreenDC, h_offscreen);
		for (long i = 0; i < allSongs[playlist]->membercount; i = i + 1) {
			TextOut(hdcOffscreenDC, spacing, (spacing+(fontsize+spacing)*(i+1)), allSongs[playlist]->tracks[i]->name, _tcslen(allSongs[playlist]->tracks[i]->name));	//(wchar_t *)
		}
	}
	return retval;
}

HRESULT renderplaylistUI() {
	HRESULT retval;
	BITMAP offscreen;

	retval = S_OK;
	return retval;
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
	case APPSTATE_PLAYLIST_START:
		applicationstate = APPSTATE_PLAYLIST_START;
		selected_playlist = 0;
		showiTunesPlaylistInterface(selected_playlist);
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

HRESULT showiTunesPlaylistInterface(short playlist) {
	HRESULT retval;
	retval = S_OK;
	setAppState(APPSTATE_PLAYLIST);
	selected_playlist = playlist;
	myITunes->AddRef();
	getPlaylists_iTunes(myITunes, allSongs);
	drawPlaylistOffscreen(allSongs, selected_playlist);
	retval = RzSBSetImageTouchpad(image_playlist_songs);
	return retval;
}

/* Handles quit/deactive/close events sent by the switchblade environment */

HRESULT STDMETHODCALLTYPE AppEventHandler(RZSBSDK_EVENTTYPETYPE rzEvent, DWORD wParam, DWORD lParam) {
	HRESULT retval = S_OK;
	switch (rzEvent) {
	case RZSBSDK_EVENT_ACTIVATED:				// reconnect to iTunes COM object when we are reawakened
		connectiTunes();
		setAppState(APPSTATE_STARTUP);
		break;
	case RZSBSDK_EVENT_DEACTIVATED:				// disconnect from iTunes when SBUI sends us to sleep
		disconnectiTunes();
		break;
	case RZSBSDK_EVENT_CLOSE:					// we disconnect from iTunes when the application quits in wWinMain
		PostQuitMessage(0);
		break;
	case RZSBSDK_EVENT_EXIT:
		PostQuitMessage(0);						// we disconnect from iTunes when the application quits in wWinMain
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

/* Test function to output to the switchblade touchpad using RzSBRenderBuffer() - i fail to access the bitmap structure, however..

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

However, this works:
HDC hdc;
	HBITMAP uiimage;
	RECT bounds;
	bounds.left = 1L;
	bounds.top = 1L;
	bounds.right = 800L;
	bounds.bottom = 1600L;
	LPCWSTR SongTitles = L"Hello World";
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

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
					 _In_opt_ HINSTANCE hPrevInstance,
					 _In_ LPWSTR    lpCmdLine,
					 _In_ int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	// TODO: Hier Code einfügen.
	LPWSTR imagepath;
	char *sbuibuf;
	BITMAPINFOHEADER offscreen_bmi;
	int errcode;
	imagepath = (LPWSTR)(L"C:\\Users\\asten\\Documents\\Development\\iTunesControl\\iTunesControl\\Background.bmp");
	CoInitialize(NULL);
	HRESULT hRes;
	LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadStringW(hInstance, IDC_ITUNESCONTROL, szWindowClass, MAX_LOADSTRING);
	MyRegisterClass(hInstance);
	// Anwendungsinitialisierung ausführen:
	hWnd = InitInstance(hInstance, nCmdShow);
	HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_ITUNESCONTROL));
	MSG msg;
	int linenum = 20;
	hRes = ::CoCreateInstance(CLSID_iTunesApp, NULL, CLSCTX_LOCAL_SERVER, IID_IiTunes, (PVOID *)&myITunes);
	//myITunes->AddRef();
	if (RzSBStart() == RZSB_OK) {
		//RzSBSetImageTouchpad(imagepath);
		hRes = RzSBEnableOSGesture(RZSBSDK_GESTURE_ALL, false);
		hRes = RzSBGestureSetCallback(reinterpret_cast<TouchpadGestureCallbackFunctionType>(TouchPadHandler));
		hRes = RzSBAppEventSetCallback(reinterpret_cast<AppEventCallbackType>(AppEventHandler));
		size_t displayPixels = 800 * 480;
		void* pixbuf;
		HANDLE draw_HDIB = GlobalAlloc(GHND, (800*32+31) / 32 * 4 * 480);
		pixbuf = GlobalLock(draw_HDIB);
		RZSBSDK_BUFFERPARAMS display;
		struct {
			BITMAPINFOHEADER bmiHeader;
			RGBQUAD bmiColors[256];
		} bmi;
		memset(&bmi, 0, sizeof(bmi));
		bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
		memset(&display, 0, sizeof(RZSBSDK_BUFFERPARAMS));
		display.PixelType = RGB565;
		display.DataSize = displayPixels * sizeof(WORD);
		display.pData = NULL; // set later
		WNDCLASSEXW sbwcex = {};
		const wchar_t sbuiwinclassname[] = L"SB UI Window";
		sbwcex.lpfnWndProc = SBWndProc;
		sbwcex.hInstance = hInstance;
		sbwcex.lpszClassName = sbuiwinclassname;
		RegisterClassExW(&sbwcex);
		HWND sbhwnd = CreateWindowEx(0, sbuiwinclassname, L"SBUI BladeSong Window", WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT, 800, 480, NULL, NULL, hInstance, NULL);
		ShowWindow(sbhwnd, nCmdShow);
		//UpdateWindow(sbhwnd);
		HDC hdcMemDC;
		HBITMAP h_offscreen;
		BITMAP offscreen;

		hdcMemDC = CreateCompatibleDC(NULL);
		h_offscreen = CreateBitmap(800, 480, 1, 32, pixbuf);
		SelectObject(hdcMemDC, h_offscreen);
		TextOut(hdcMemDC, 1, 1, L"Anders", 6);
		BitBlt(GetDC(hWnd), 0, 0, 800, 480, hdcMemDC, 0, 0, SRCCOPY);  // Test Copy to WinScreen see if works
		GetObject(h_offscreen, sizeof(BITMAP), &offscreen);
		offscreen_bmi.biSize = sizeof(BITMAPINFOHEADER);
		offscreen_bmi.biWidth = offscreen.bmWidth;
		offscreen_bmi.biHeight = offscreen.bmHeight;
		offscreen_bmi.biPlanes = 1;
		offscreen_bmi.biBitCount = 32;
		offscreen_bmi.biCompression = BI_RGB;
		offscreen_bmi.biSizeImage = 0;
		offscreen_bmi.biXPelsPerMeter = 0;
		offscreen_bmi.biYPelsPerMeter = 0;
		offscreen_bmi.biClrUsed = 0;
		offscreen_bmi.biClrImportant = 0;
		HANDLE HDIB = GlobalAlloc(GHND, (offscreen.bmWidth*(offscreen_bmi.biBitCount) + 31) / 32 * 4 * offscreen.bmHeight);
		display.pData = (BYTE *)GlobalLock(HDIB);

		//errcode = GetDIBits(hdcMemDC, h_offscreen, 1, 479, display.pData, (BITMAPINFO *)&offscreen_bmi, DIB_RGB_COLORS);
		GetBitmapBits(h_offscreen, display.DataSize, display.pData);
		BitBlt(GetDC(hWnd), 0, 0, 800, 480, hdcMemDC, 0, 0, SRCCOPY);  // Test Copy to WinScreen see if works
		//GetBitmapBits(offscreen, display.DataSize, display.pData);
		//pColor = ((LPSTR)pBitmapInfo + (WORD)(pBitmapInfo->bmiHeader.biSize)); ?? reicht auch??
		//GetDIBits(hdcMemDC, offscreen, 0, 480, display.pData, (BITMAPINFO *)&bmi, DIB_RGB_COLORS);
		//display.pData = (BYTE *)sbuibuf;
		RzSBRenderBuffer(RZSBSDK_DISPLAY_WIDGET, &display);
	}


*/