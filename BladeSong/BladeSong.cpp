#include "BladeSong.h"

/*todo bladesong:

-) alle bilder als ressorce einbinden, dann loadfromresource makro verwenden
-) playlists sortieren
-) threaded scrolling
-) ueberschrift bei playlists
-) switchblade theme support einbaun
-) lautstaerkeregelungs-fenster und knopf
-) songtitel scrollt in control fenster hin/her
-) control fenster huebscher machen

implementierungstips:

state weiterscrollen=true waehrend scrollthread laeuft, er beendet sich wenn es false ist (zb. durch tap event)
laden von songs - array von playlists mit malloc erstellen, dann pointer auf einzelne playlists an ladethread; im array gibt es eine zustandsvariable (nicht vorhanden/ladt/sortiert/fertig geladen) die auf fertigg geladen am ende vom ladethread gesetzt wird. angezeigt wird eine list erst wenn sie status fertig geladen hat
abspielen von playlists? ev eine play() funktion bei playlists?*/

HRESULT connectiTunes() {
	/* start COM connectivity */
	CoInitializeEx(NULL, COINIT_MULTITHREADED | COINIT_SPEED_OVER_MEMORY);
	/* attach to iTunes via COM */
	return (::CoCreateInstance(CLSID_iTunesApp, NULL, CLSCTX_LOCAL_SERVER, IID_IiTunes, (PVOID *)&myITunes));
}

HRESULT disconnectiTunes() {
	free(allSongs);								// release memory block holding song titles	
	myITunes->Release();						// release iTunes COM object hold by this app
	RzSBStop();									// release switchblade control
	CoUninitialize();							// stop COM connectivity
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

bool play_iTunes_song(IiTunes* iITunes, long playlist, long track) {
	IITObject *track_to_play_temp;				// this is the IITObject we get back from GetITObjectByID using the stored ID's from the selected track
	IITTrack *track_to_play;					// we need to get a pointer to this interface in order to use play() on it
	HRESULT errCode = S_OK;
	iITunes->Release();
	/* get the song using the previously stored id's of the track */
	errCode = iITunes->GetITObjectByID(allSongs[playlist]->SourceID, allSongs[playlist]->PlaylistID, allSongs[playlist]->tracks[track]->TrackID, allSongs[playlist]->tracks[track]->TrackDatabaseID, &track_to_play_temp);
	/* get an IITTrack interface to the track we got */
	errCode = track_to_play_temp->QueryInterface(IID_IITTrack, (void**)&track_to_play);
	/* play the track */
	errCode = track_to_play->Play();
	/* release handles */
	track_to_play->Release();
	track_to_play_temp->Release();
	if (errCode == S_OK)
		return true;
	else
		return false;
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
/* Old Code: Single-Threaded Song Loading mechanism:
HRESULT getPlaylists_iTunes(IiTunes* iITunes) {
	HRESULT errCode;
	IITSource *iTunesLibrary;
	IITPlaylistCollection *workingPlaylists;
	IITPlaylist *workingPlaylist;
	IITTrackCollection *workingTracks;
	IITTrack *workingTrack;
	BSTR workingPlayListName;
	BSTR workingTrackName;
	long num_songs_per_playlist;
	long workingplaylistID;
	long workingtrackID;
	long workingtrackDatabaseID;
	long workingSourceID;
	workingplaylistID = 0;
	workingtrackID = 0;
	workingtrackDatabaseID = 0;
	workingSourceID = 0;
	num_playlists = 0;
	errCode = S_OK;
	iITunes->Release();
	errCode = iITunes->get_LibrarySource(&iTunesLibrary);
	if (errCode != S_OK)
		return errCode;
	errCode = iTunesLibrary->get_SourceID(&workingSourceID);
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
		errCode = workingPlaylist->get_PlaylistID(&workingplaylistID);
		if (errCode != S_OK)
			return errCode;
		allSongs[i]->membercount = num_songs_per_playlist;
		allSongs[i]->name = (LPTSTR)workingPlayListName;
		allSongs[i]->PlaylistID = workingplaylistID;
		allSongs[i]->SourceID = workingSourceID;
		allSongs[i]->tracks = (trackData **)malloc(num_songs_per_playlist * sizeof(trackData));
		for (long j = 0; j < num_songs_per_playlist; j = j + 1) { 
			// enumerate all tracks of this playlist, put it in allsongs[i]->tracks
				errCode = workingTracks->get_Item(j+1, &workingTrack);
				if (errCode != S_OK)
					return errCode;
				errCode = workingTrack->get_Name(&workingTrackName);
				if (errCode != S_OK)
					return errCode;
				errCode = workingTrack->get_TrackID(&workingtrackID);
				if (errCode != S_OK)
					return errCode;
				errCode = workingTrack->get_TrackDatabaseID(&workingtrackDatabaseID);
				allSongs[i]->tracks[j] = (trackData *)malloc(sizeof(trackData));
				allSongs[i]->tracks[j]->name = (LPTSTR)workingTrackName;
				allSongs[i]->tracks[j]->TrackID = workingtrackID;
				allSongs[i]->tracks[j]->TrackDatabaseID = workingtrackDatabaseID;
				workingTrack->Release();
		}
		workingTracks->Release();
		workingPlaylist->Release();
	}
	workingPlaylists->Release();
	iTunesLibrary->Release();
	return errCode;
} */

HRESULT getPlaylists_iTunes(IiTunes* iITunes) {
	HRESULT errCode;
	IITSource *iTunesLibrary;
	IITPlaylistCollection *workingPlaylists;
	HANDLE *threadlist;
	long workingSourceID;
	workingSourceID = 0;
	num_playlists = 0;
	errCode = S_OK;
	errCode = iITunes->get_LibrarySource(&iTunesLibrary);
	if (errCode != S_OK)
		return errCode;
	errCode = iTunesLibrary->get_SourceID(&workingSourceID);
	if (errCode != S_OK)
		return errCode;
	errCode = iTunesLibrary->get_Playlists(&workingPlaylists);
	if (errCode != S_OK)
		return errCode;
	errCode = workingPlaylists->get_Count(&num_playlists);
	if (errCode != S_OK)
		return errCode;
	allSongs = (playlistData **)malloc(num_playlists * sizeof(playlistData**));
	threadlist = (HANDLE *)malloc(num_playlists * sizeof(HANDLE));
	for (long i = 0; i < num_playlists; i = i + 1) {
		allSongs[i] = (playlistData *)malloc(sizeof(playlistData));
		allSongs[i]->internalID = i;
		threadlist[i] = CreateThread(NULL, 0, getiTunesPlaylist, &allSongs[i]->internalID, 0,NULL);
	}
	workingPlaylists->Release();
	iTunesLibrary->Release();
	iITunes->Release();
	return errCode;
}
DWORD WINAPI getiTunesPlaylist(LPVOID pData) {
	/* we get numer which is the allocated Playlist we should im port */
	HRESULT errCode;
	long* playlistno = (long*)pData;
	IITSource *iTunesLibrary;
	IITPlaylistCollection *workingPlaylists;
	IITPlaylist *workingPlaylist;
	IITTrackCollection *workingTracks;
	IITTrack *workingTrack;
	BSTR workingPlayListName;
	BSTR workingTrackName;
	long num_songs_per_playlist;
	long workingplaylistID;
	long workingtrackID;
	long workingtrackDatabaseID;
	long workingSourceID;
	workingplaylistID = 0;
	workingtrackID = 0;
	workingtrackDatabaseID = 0;
	workingSourceID = 0;
	errCode = S_OK;
	//OutputDebugStringW(LPCWSTR("\n\nThreadID:"));
	//OutputDebugStringW(LPCWSTR(*playlistno));
	allSongs[*playlistno] = (playlistData *)malloc(sizeof(playlistData));
	allSongs[*playlistno]->loadState = PL_STATE_NOT_LOADED;
	/* Traverste the iTunes COM object to get to our desired playlist */
	errCode = myITunes->get_LibrarySource(&iTunesLibrary);
	if (errCode != S_OK)
		return errCode;
	errCode = iTunesLibrary->get_SourceID(&workingSourceID);
	if (errCode != S_OK)
		return errCode;
	errCode = iTunesLibrary->get_Playlists(&workingPlaylists);
	if (errCode != S_OK)
		return errCode;
	/* Here we get the representation of the playlist we are interested in */
	errCode = workingPlaylists->get_Item(*playlistno + 1, &workingPlaylist);
	if (errCode != S_OK)
		return errCode;
	/* Here we get a Collection of the Tracks of the playlist we are looking for */
	errCode = workingPlaylist->get_Tracks(&workingTracks);
	if (errCode != S_OK)
		return errCode;
	errCode = workingTracks->get_Count(&num_songs_per_playlist);
	if (errCode != S_OK)
		return errCode;
	errCode = workingPlaylist->get_Name(&workingPlayListName);
	if (errCode != S_OK)
		return errCode;
	errCode = workingPlaylist->get_PlaylistID(&workingplaylistID);
	if (errCode != S_OK)
		return errCode;
	/* We fill up our representation of the playlist with the information we have */
	allSongs[*playlistno]->membercount = num_songs_per_playlist;
	allSongs[*playlistno]->name = (LPTSTR)workingPlayListName;
	allSongs[*playlistno]->PlaylistID = workingplaylistID;
	allSongs[*playlistno]->SourceID = workingSourceID;
	allSongs[*playlistno]->tracks = (trackData **)malloc(num_songs_per_playlist * sizeof(trackData));
	allSongs[*playlistno]->loadState = PL_STATE_LOADING;
	for (long j = 0; j < num_songs_per_playlist; j = j + 1) {
		/* enumerate all tracks of this playlist, put it in allsongs[i]->tracks */
		errCode = workingTracks->get_Item(j + 1, &workingTrack);
		if (errCode != S_OK)
			return errCode;
		errCode = workingTrack->get_Name(&workingTrackName);
		if (errCode != S_OK)
			return errCode;
		errCode = workingTrack->get_TrackID(&workingtrackID);
		if (errCode != S_OK)
			return errCode;
		errCode = workingTrack->get_TrackDatabaseID(&workingtrackDatabaseID);
		allSongs[*playlistno]->tracks[j] = (trackData *)malloc(sizeof(trackData));
		allSongs[*playlistno]->tracks[j]->name = (LPTSTR)workingTrackName;
		allSongs[*playlistno]->tracks[j]->TrackID = workingtrackID;
		allSongs[*playlistno]->tracks[j]->TrackDatabaseID = workingtrackDatabaseID;
		workingTrack->Release();
	}
	/* Missing: here we need to sort the playlist */
	allSongs[*playlistno]->loadState = PL_STATE_READY;
	workingTracks->Release();
	workingPlaylist->Release();
	workingPlaylists->Release();
	iTunesLibrary->Release();
	return errCode;
}


COLORREF transl_RGB565(byte r, byte g, byte b) {// we expect color values in rgb 0..255 range each
	return ((r/8) << 11) | ((g/4) << 5) | (b/8);// normalize to 0..31/0..63/0..31 range, then bitshift into WORD for RGB565 conversion, then reurn in COLORREF datatype for compatibility with existing windows draw functions
}

HRESULT drawPlaylistOffscreen(short playlist) { // playlist 0 = list of playlists, playlist 1 = first playlists, etc..
	HRESULT retval;
	long neededlines;
	HFONT hFont;
	size_t chars_per_line = 32;
	retval = S_OK;
	if (playlist > num_playlists)
		return E_FAIL;
	else {
		/* calculate the number of lines the offscreen image needs to hold based on font size, spacing between lines and needed number of lines */
		if (playlist == 0)
			neededlines = num_playlists * (fontsize + spacing) + spacing;
		else
			neededlines = allSongs[playlist]->membercount*(fontsize + spacing) + spacing;
		if (neededlines < 800)					// todo: this should not be necessary
			neededlines = 800;
		// we need the offscreen image to be exactly display size  -  make bigger offscreen image, then BitBlt into s second offscreen image that has 800x480
		// allocate memory (moveable, fill with zeros): screenwidth * bpp, align to next 32 bit block * planes * lines
		o_h_pixbuf = GlobalAlloc(GHND, (800 * 32 + 31) / 32 * 4 * neededlines);
		// lock the memory, get a pointer to it
		o_pixbuf = GlobalLock(o_h_pixbuf);
		// get handle to an offscreen device context
		hdcOffscreenDC = CreateCompatibleDC(NULL);
		// create offscreen memory bitmap that will hold the full list of playlists
		h_offscreen = CreateBitmap(800, neededlines, 1, 32, o_pixbuf);
		// select offscreen image into device context
		SelectObject(hdcOffscreenDC, h_offscreen);
		SetTextColor(hdcOffscreenDC, transl_RGB565(212, 175, 55));
		hFont = CreateFont(25, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_OUTLINE_PRECIS, CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY, DEFAULT_PITCH, TEXT("razer regular"));
		SelectObject(hdcOffscreenDC, hFont);
		SetBkColor(hdcOffscreenDC, transl_RGB565(0, 0, 0));
		if (playlist == 0) {					// copy only names of playlists
			/* lenght of a playlist name name in the memory structutre: _tcslen(allSongs[playlist]->name()) */
			for (long i = 0; i < num_playlists; i = i + 1) {
				chars_per_line = _tcslen(allSongs[i]->name);
				TextOut(hdcOffscreenDC, spacing, (spacing + (fontsize + spacing)*(i + 1)), allSongs[i]->name, chars_per_line > max_chars_per_line ? max_chars_per_line : chars_per_line);
			}
		} else {								// copy names of songs of the specified playlist
					/* check wether songs are loaded and ready for display */
			if (allSongs[playlist]->loadState != PL_STATE_READY) {
				TextOut(hdcOffscreenDC, spacing, (spacing + (fontsize + spacing)), L"loading playlist...\0", chars_per_line > max_chars_per_line ? max_chars_per_line : chars_per_line);
			}
			else {
				/* lenght of a song name in the memory structutre: _tcslen((allSongs[playlist]->tracks[i]->name()) */
				for (long i = 0; i < allSongs[playlist]->membercount; i = i + 1) {
					chars_per_line = _tcslen(allSongs[playlist]->tracks[i]->name);
					TextOut(hdcOffscreenDC, spacing, (spacing + (fontsize + spacing)*(i + 1)), allSongs[playlist]->tracks[i]->name, chars_per_line > max_chars_per_line ? max_chars_per_line : chars_per_line);	//(wchar_t *)  // find a way to cap string lenght correctly - not bigger than strlen, not bigger than screen size
				}
			}
		}
		DeleteObject(hFont);
	}
	return retval;
}

HRESULT renderplaylistUI() {
	HRESULT retval;								// for error handling
	HANDLE o_h_pixbuf_viewport;					// handle to the copy of the view portion of the offscreen image we want to draw onto the SwitchBlade display
	void *o_pixbuf_viewport;					// pointer to memory region of the image view portion copy to later draw onto the SwitchBlade display
	HBITMAP h_offscreen_viewport;				// Bitmap Handle to the view portion copy image
	BITMAP offscreen_viewport;					// Image to be drawn onto SBUI
	long neededlines;
	HANDLE HDIB;								// handle for SBUI RGB565 image buffer
	HDC hdcOffscreenviewportDC;					// Create device context handle for the viewport image
	
	/* BITBLT to implement scrolling is here;
	   create second image the exact size of the SBUI display to copy scrolled view portion of the offscreen image into */
	neededlines = 480;						// we need to match the SBUI display
	hdcOffscreenviewportDC = CreateCompatibleDC(NULL);
	/* allocate memory for the view portion copy of the offscreen image we drew in drawPlaylistOffscreen(short playlist) and create the corresponding BITMAP object*/
	o_h_pixbuf_viewport = GlobalAlloc(GHND, (800 * 32 + 31) / 32 * 4 * neededlines);
	o_pixbuf_viewport = GlobalLock(o_h_pixbuf);
	h_offscreen_viewport = CreateBitmap(800, 480, 1, 32, o_pixbuf_viewport);
	/* select the offscreen view portion buffer into its device context */
	SelectObject(hdcOffscreenviewportDC, h_offscreen_viewport);
	/* copy our viw portion of the offscreen image into our viewport image buffer */
	BitBlt(hdcOffscreenviewportDC, 0, 0, 800, 480, hdcOffscreenDC, 0, scroll_offset, SRCCOPY);
	/* prepare the SBUI memory buffer to hold the image data we want to draw on the display */
	memset(&sbuidisplay, 0, sizeof(RZSBSDK_BUFFERPARAMS));
	GetObject(h_offscreen_viewport, sizeof(BITMAP), &offscreen_viewport);
	bmi_offscreen.biSize = sizeof(BITMAPINFOHEADER);
	bmi_offscreen.biWidth = 800;
	bmi_offscreen.biHeight = 480;
	bmi_offscreen.biPlanes = 1;
	bmi_offscreen.biBitCount = 32;
	bmi_offscreen.biCompression = BI_RGB;
	bmi_offscreen.biSizeImage = 0;
	bmi_offscreen.biXPelsPerMeter = 0;
	bmi_offscreen.biYPelsPerMeter = 0;
	bmi_offscreen.biClrUsed = 0;
	bmi_offscreen.biClrImportant = 0;
	// Allocate Memory for SBUI pixel Buffer
	HDIB = GlobalAlloc(GHND, ((offscreen_viewport.bmWidth*bmi_offscreen.biBitCount + 31) / 32 * 4 * offscreen_viewport.bmHeight));
	// set up SBUI display structure
	sbuidisplay.PixelType = RGB565;	
	sbuidisplay.DataSize = 800 * 480 * sizeof(WORD);
	sbuidisplay.pData = (BYTE *)GlobalLock(HDIB);
	// copy the offscreen viewport image buffer into the SBUI display buffer
	GetBitmapBits(h_offscreen_viewport, sbuidisplay.DataSize, sbuidisplay.pData);
	// let the SBUI draw from its image structure
	RzSBRenderBuffer(RZSBSDK_DISPLAY_WIDGET, &sbuidisplay);
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

HRESULT padFlick(WORD direction) {				// handles switchblade input for scrolling in the playlists
	HRESULT retval;
	retval = S_OK;
	short scrolled = 0;
	float scrolllength = 300;					// how much to scroll with one flick
	short SBUI_length = 480;
	float scroll_increment;
	float scroll_increment_fas = 12;
	long offscreen_imagelength;
	BITMAP offscreen;
	GetObject(h_offscreen, sizeof(BITMAP), &offscreen);
	offscreen_imagelength = offscreen.bmHeight;
	scroll_increment = scroll_increment_fas;
	switch (direction) {						// decide in which direction to scroll
	case RZSBSDK_DIRECTION_UP:					// only scroll down to the end of the list
		while (((scroll_offset + scroll_increment) <= (offscreen_imagelength - SBUI_length)) && ((scrolled + scroll_increment) <= scrolllength)) {
			scroll_offset = scroll_offset + (long)scroll_increment;
			scrolled = scrolled + (short)scroll_increment;
			/* scroll slower at the end of the scroll - scale the scrolling speed/scroll increments - scrolled should at this point never be 0 */
			scroll_increment = scroll_increment_fas - ((scrolled / scrolllength) * scroll_increment_fas)+1;
			renderplaylistUI();					// redraw SBUI
			Sleep(1);
		}
		break;
	case RZSBSDK_DIRECTION_DOWN:				// only scroll till the very top
		while ((scroll_offset > 0) && ((scrolled + scroll_increment) <= scrolllength)) {
			scroll_offset = scroll_offset - (long)scroll_increment;
			scrolled = scrolled + (short)scroll_increment;
			/* scroll slower at the end of the scroll - scale the scrolling speed/scroll increments */
			scroll_increment = scroll_increment_fas - ((scrolled / scrolllength) * scroll_increment_fas)+1;
			renderplaylistUI();					// redraw SBUI
			/* scroll slower at the end of the scroll - get slower in the third quarter of the scroll, even slower in the fourth quarter */
			Sleep(1);
		}
		break;
	default:
		break;
	}
	return retval;
}

HRESULT play_song_on_playlist(long playlist, WORD y_coordinates) {
	HRESULT retval = S_OK;
	long selection;
	long true_y = scroll_offset + y_coordinates - spacing - 72;
	if (true_y >= 0) {
		if (selected_playlist == 0) {
			selection = true_y / (45);
			selected_playlist = (short)selection;
			setAppState(APPSTATE_PLAYLIST);
		}
		else {
			selection = true_y / (45);
			myITunes->AddRef();
			play_iTunes_song(myITunes, (long)playlist, selection);
			// OutputDebugStringW(L"\n");
			// OutputDebugStringW((LPWSTR)allSongs[playlist]->tracks[selection]->name);
		}
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
	retval = RzSBGestureSetCallback(reinterpret_cast<TouchpadGestureCallbackFunctionType>(TouchPadHandler_Controls));
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
		showiTunesControlInterface();		// paint interface for iTunes controls
		refreshiTunesPlayList();			// get songlists from iTunes
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
		scroll_offset = 0;
		showiTunesPlaylistInterface();
		break;
	case APPSTATE_PLAYLIST:
		applicationstate = APPSTATE_PLAYLIST;
		scroll_offset = 0;
		showiTunesPlaylistInterface();
		break;
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
	retval = RzSBGestureSetCallback(reinterpret_cast<TouchpadGestureCallbackFunctionType>(TouchPadHandler_Controls));
	return retval;
}

/* displays the interface for playlist control on the switchblade touchscreen */

HRESULT refreshiTunesPlayList() {
	HRESULT retval;
	retval = S_OK;
	myITunes->AddRef();
	getPlaylists_iTunes(myITunes);
	return retval;
}

HRESULT showiTunesPlaylistInterface() {
	HRESULT retval;
	retval = S_OK;
	drawPlaylistOffscreen(selected_playlist);
	renderplaylistUI();
	retval = RzSBGestureSetCallback(reinterpret_cast<TouchpadGestureCallbackFunctionType>(TouchPadHandler_Playlist));
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
			setAppState(APPSTATE_CONTROLS_PLAY);
			break;
		case RZSBSDK_DK_7:						// show playlist button
			setAppState(APPSTATE_PLAYLIST_START);
			break;
		default:
			break;
		}
		return retval;
}

/* Generic function to handle touchpad inpuit on the switchblade ui. */

HRESULT STDMETHODCALLTYPE TouchPadHandler_Controls(RZSBSDK_GESTURETYPE gesturetype, DWORD touchpoints, WORD x, WORD y, WORD z) {
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

HRESULT STDMETHODCALLTYPE TouchPadHandler_Playlist(RZSBSDK_GESTURETYPE gesturetype, DWORD touchpoints, WORD x, WORD y, WORD z) {
	HRESULT retval = S_OK;
	switch (gesturetype) {
	case RZSBSDK_GESTURE_FLICK:
		retval = padFlick(z);					// call scrolling function
		break;
	case RZSBSDK_GESTURE_PRESS:
												// stop scrolling upon pad press
		break;
	case RZSBSDK_GESTURE_TAP:					// play song upon tapping on it
		retval = play_song_on_playlist(selected_playlist, y);
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
			buttonretval = MessageBox(NULL, (LPCWSTR)L"Razer Switchblade hardware initalization error!", (LPCWSTR)L"Switchblade error", MB_OK | MB_ICONERROR | MB_DEFBUTTON1 | MB_APPLMODAL);
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
			buttonretval = MessageBox(NULL, (LPCWSTR)L"Apple iTunes is not installed!", (LPCWSTR)L"iTunes error", MB_OK | MB_ICONERROR | MB_DEFBUTTON1 | MB_APPLMODAL);
			break;
		case CLASS_E_NOAGGREGATION:
		case E_NOINTERFACE:
		case E_POINTER:
		default:
			buttonretval = MessageBox(NULL, (LPCWSTR)L"Unknown error connecting to iTunes!", (LPCWSTR)L"iTunes error", MB_OK | MB_ICONERROR | MB_DEFBUTTON1 | MB_APPLMODAL);
		}
	}
	return (hRes);
}