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

#include "BladeSong.h"

/*todo:

-) add an additional window with volume control
-) playlist control also displays the current song title, scrolling with long titles
-) make ui look better
*/

HRESULT connectiTunes() {
	/* start COM connectivity */
	CoInitializeEx(NULL, COINIT_MULTITHREADED | COINIT_SPEED_OVER_MEMORY);
	/* attach to iTunes via COM */
	return (::CoCreateInstance(CLSID_iTunesApp, NULL, CLSCTX_LOCAL_SERVER, IID_IiTunes, (PVOID *)&myITunes));
}

HRESULT disconnectiTunes() {
	continue_scrolling = false;
	for (long i = 0; i < num_playlists; i = i + 1) {
		TerminateThread(threadlist_getplaylists[i], S_OK);
		if (allSongs[i] != NULL)
			for (long j = 0; j < allSongs[i]->membercount; j = j + 1)
				if (allSongs[i]->tracks[j] != NULL)
					free(allSongs[i]->tracks[j]);
		free(allSongs[i]->tracks);
		free(allSongs[i]);
	}
	free(threadlist_getplaylists);
	TerminateThread(scrollthread, S_OK);
	TerminateThread(scrollimmunitytimerthread, S_OK);
	free(allSongs);								// release memory block holding song titles	
	myITunes->Release();						// release iTunes COM object hold by this app
	RzSBStop();									// release switchblade control
	if (o_h_pixbuf != NULL) {
		GlobalUnlock(o_h_pixbuf);
		GlobalFree(o_h_pixbuf);
	}
	DeleteObject(hbutton_controls);
	DeleteObject(hbutton_exit);
	DeleteObject(hbutton_playlist);
	DeleteObject(hcontrols_pause);
	DeleteObject(hcontrols_play);
	DestroyIcon(AppIcon);
	DestroyIcon(AppIconSM);
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

HRESULT getPlaylists_iTunes(IiTunes* iITunes) {
	HRESULT errCode;
	IITSource *iTunesLibrary;
	IITPlaylistCollection *workingPlaylists;
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
	threadlist_getplaylists = (HANDLE *)malloc(num_playlists * sizeof(HANDLE));
	for (long i = 0; i < num_playlists; i = i + 1)
		allSongs[i] = NULL;						// preInit in case User quits early
	for (long i = 0; i < num_playlists; i = i + 1) {
		allSongs[i] = (playlistData *)malloc(sizeof(playlistData));
		allSongs[i]->internalID = i;
		threadlist_getplaylists[i] = CreateThread(NULL, 0, getiTunesPlaylist, &allSongs[i]->internalID, 0,NULL);
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
	for (long j = 0; j < num_songs_per_playlist; j = j + 1)
		allSongs[*playlistno]->tracks[j] = NULL;// preInit in case User quits early
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
	allSongs[*playlistno]->loadState = PL_STATE_SORTING;
	if (allSongs[*playlistno]->membercount > 1)
		qsort((void *)allSongs[*playlistno]->tracks, (size_t)allSongs[*playlistno]->membercount, sizeof(trackData*), trackComp);
	/* Missing: here we need to sort the playlist */
	allSongs[*playlistno]->loadState = PL_STATE_READY;
	workingTracks->Release();
	workingPlaylist->Release();
	workingPlaylists->Release();
	iTunesLibrary->Release();
	return errCode;
}

int trackComp(const void *t1, const void *t2) {
	trackData* tc1 = (trackData *)t1;
	trackData* tc2 = (trackData *)t2;
	return wcscmp(*(wchar_t**)tc1->name, *(wchar_t**)tc2->name);
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
		// get handle to an offscreen device context
		hdcOffscreenDC = CreateCompatibleDC(NULL);
		/* calculate the number of lines the offscreen image needs to hold based on font size, spacing between lines and needed number of lines */
		if (playlist == 0)
			neededlines = (num_playlists + 1) * (fontsize + spacing) + spacing;
		else
			neededlines = allSongs[playlist]->membercount*(fontsize + spacing) + spacing;
		o_image_lines = neededlines;
		// we need the offscreen image to be exactly display size  -  make bigger offscreen image, then BitBlt into s second offscreen image that has 800x480
		// allocate memory (moveable, fill with zeros): screenwidth * bpp, align to next 32 bit block * planes * lines
		if (o_h_pixbuf != NULL)
			GlobalFree(o_h_pixbuf);
		o_h_pixbuf = GlobalAlloc(GHND, (SWITCHBLADE_TOUCHPAD_X_SIZE * 32 + 31) / 32 * 4 * neededlines);
		// lock the memory, get a pointer to it
		o_pixbuf = GlobalLock(o_h_pixbuf);
			// create offscreen memory bitmap that will hold the full list of playlists
		h_offscreen = CreateBitmap(SWITCHBLADE_TOUCHPAD_X_SIZE, neededlines, 1, 32, o_pixbuf);
		// select offscreen image into device context
		SelectObject(hdcOffscreenDC, h_offscreen);
		SetTextColor(hdcOffscreenDC, transl_RGB565(212, 175, 55));
		hFont = CreateFont(fontsize, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_OUTLINE_PRECIS, CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY, DEFAULT_PITCH, TEXT("Courier New"));
		SelectObject(hdcOffscreenDC, hFont);
		SetBkColor(hdcOffscreenDC, transl_RGB565(0, 0, 0));
		if (playlist == 0) {					// copy only names of playlists
			/* lenght of a playlist name name in the memory structutre: _tcslen(allSongs[playlist]->name()) */
			TextOut(hdcOffscreenDC, spacing, spacing, L"Playlists", 9);
			for (long i = 0; i < num_playlists; i = i + 1) {
				chars_per_line = _tcslen(allSongs[i]->name);
				TextOut(hdcOffscreenDC, spacing, (spacing + (fontsize + spacing)*(i + 1)), allSongs[i]->name, chars_per_line > max_chars_per_line ? max_chars_per_line : chars_per_line);
			}
		} else {								// copy names of songs of the specified playlist
					/* check wether songs are loaded and ready for display */
			if (allSongs[playlist]->loadState != PL_STATE_READY) {
				TextOut(hdcOffscreenDC, spacing, (spacing + (fontsize + spacing)), L"loading playlist...", 19);
			}
			else {
				/* lenght of a song name in the memory structutre: _tcslen((allSongs[playlist]->tracks[i]->name()) */
				TextOut(hdcOffscreenDC, spacing, spacing, allSongs[playlist]->name, _tcslen(allSongs[playlist]->name));
				for (long i = 0; i < allSongs[playlist]->membercount; i = i + 1) {
					chars_per_line = _tcslen(allSongs[playlist]->tracks[i]->name);
					TextOut(hdcOffscreenDC, spacing, (spacing + (fontsize + spacing)*(i + 1)), allSongs[playlist]->tracks[i]->name, chars_per_line > max_chars_per_line ? max_chars_per_line : chars_per_line);	//(wchar_t *)  // find a way to cap string lenght correctly - not bigger than strlen, not bigger than screen size
				}
			}
		}
		DeleteObject(hFont);
	}
	GlobalUnlock(o_h_pixbuf);
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
	
	retval = S_OK;
	/* BITBLT to implement scrolling is here;
	   create second image the exact size of the SBUI display to copy scrolled view portion of the offscreen image into */
	neededlines = SWITCHBLADE_TOUCHPAD_Y_SIZE;						// we need to match the SBUI display
	hdcOffscreenviewportDC = CreateCompatibleDC(NULL);
	/* allocate memory for the view portion copy of the offscreen image we drew in drawPlaylistOffscreen(short playlist) and create the corresponding BITMAP object*/
	o_h_pixbuf_viewport = GlobalAlloc(GHND, (SWITCHBLADE_TOUCHPAD_X_SIZE * 32 + 31) / 32 * 4 * neededlines); // should be SWITCHBLADE_TOUCHPAD_SIZE_IMAGEDATA?
	o_pixbuf_viewport = GlobalLock(o_h_pixbuf);
	h_offscreen_viewport = CreateBitmap(SWITCHBLADE_TOUCHPAD_X_SIZE, SWITCHBLADE_TOUCHPAD_Y_SIZE, 1, 32, o_pixbuf_viewport);
	/* select the offscreen view portion buffer into its device context */
	SelectObject(hdcOffscreenviewportDC, h_offscreen_viewport);
	/* copy our viw portion of the offscreen image into our viewport image buffer */
	BitBlt(hdcOffscreenviewportDC, 0, 0, SWITCHBLADE_TOUCHPAD_X_SIZE, SWITCHBLADE_TOUCHPAD_Y_SIZE, hdcOffscreenDC, 0, scroll_offset, SRCCOPY);
	/* prepare the SBUI memory buffer to hold the image data we want to draw on the display */
	memset(&sbuidisplay, 0, sizeof(RZSBSDK_BUFFERPARAMS));
	GetObject(h_offscreen_viewport, sizeof(BITMAP), &offscreen_viewport);
	// Allocate Memory for SBUI pixel Buffer
	HDIB = GlobalAlloc(GHND, ((offscreen_viewport.bmWidth*offscreen_viewport.bmBitsPixel + 31) / 32 * 4 * offscreen_viewport.bmHeight));
	// set up SBUI display structure
	sbuidisplay.PixelType = RGB565;	
	sbuidisplay.DataSize = SWITCHBLADE_TOUCHPAD_SIZE_IMAGEDATA;
	sbuidisplay.pData = (BYTE *)GlobalLock(HDIB);
	// copy the offscreen viewport image buffer into the SBUI display buffer
	GetBitmapBits(h_offscreen_viewport, sbuidisplay.DataSize, sbuidisplay.pData);
	// let the SBUI draw from its image structure
	RzSBRenderBuffer(RZSBSDK_DISPLAY_WIDGET, &sbuidisplay);
	GlobalUnlock(HDIB);
	GlobalFree(HDIB);
	GlobalUnlock(o_h_pixbuf);
	GlobalFree(o_h_pixbuf_viewport);
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
	scrollthread = CreateThread(NULL, 0, scrollUI, &direction, 0, NULL);
	return retval;
}

HRESULT padMove(WORD new_y) {					// as a result of moving the finger on the UI display, we want to scroll to the point the finger was moved
	HRESULT retval;
	retval = S_OK;
	short to_scroll;
	if (moving == false) {
		moving = true;
		to_scroll = (short)new_y - (short)old_y;
		short scroll_increment = 8;
		short scrolled = 0;
		if ((new_y - old_y) < 0) {				// / (o_image_lines - SWITCHBLADE_TOUCHPAD_Y_SIZE / 3) clause stems from some weird scaling factor of the Switchblade display i do not understand
			if ((scroll_offset + scroll_increment) <= (o_image_lines - SWITCHBLADE_TOUCHPAD_Y_SIZE/3)) {
				scroll_offset += scroll_increment;
				renderplaylistUI();
				Sleep(1);
			}
		}
		else {
			if ((scroll_offset - scroll_increment) >= 0) {
				scroll_offset -= scroll_increment;
				renderplaylistUI();
				Sleep(1);
			}
		}
		moving = false;
	}
	return retval;
}

DWORD WINAPI flickTimer(LPVOID pData) {			// when we initiate a flick, we do not want to scroll/move the ui during the flick
	HRESULT errCode;
	errCode = S_OK;
	flick_in_progress = true;
	Sleep(20);
	flick_in_progress = false;
	return errCode;
}

DWORD WINAPI scrollUI(LPVOID pData) {
	/* we assume the direction of the scroll as argument */
	HRESULT errCode;
	WORD* direction = (WORD*)pData;
	continue_scrolling = false;
	errCode = S_OK;
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
	switch (*direction) {						// decide in which direction to scroll
	case RZSBSDK_DIRECTION_UP:					// only scroll down to the end of the list
		continue_scrolling = true;				// we now will start scrolling till we run out of momentum (scrolllength) - this flag is used to signal us to stop scrolling early
		/* the (o_image_lines - SWITCHBLADE_TOUCHPAD_Y_SIZE / 3) stems from some weird scaling factor of the Switchblade display i do not understand */
		while (((scroll_offset + scroll_increment) <= (o_image_lines - SWITCHBLADE_TOUCHPAD_Y_SIZE / 3)) && ((scrolled + scroll_increment) <= scrolllength) && continue_scrolling) {
			scroll_offset = scroll_offset + (long)scroll_increment;
			scrolled = scrolled + (short)scroll_increment;
			/* scroll slower at the end of the scroll - scale the scrolling speed/scroll increments - scrolled should at this point never be 0 */
			scroll_increment = scroll_increment_fas - ((scrolled / scrolllength) * scroll_increment_fas) + 1;
			renderplaylistUI();					// redraw SBUI
			Sleep(1);
		}
		break;
	case RZSBSDK_DIRECTION_DOWN:				// only scroll till the very bottom
		continue_scrolling = true;				// we now will start scrolling till we run out of momentum (scrolllength) - this flag is used to signal us to stop scrolling early
		while ((scroll_offset > 0) && ((scrolled + scroll_increment) <= scrolllength) && continue_scrolling) {
			scroll_offset = scroll_offset - (long)scroll_increment;
			scrolled = scrolled + (short)scroll_increment;
			/* scroll slower at the end of the scroll - scale the scrolling speed/scroll increments */
			scroll_increment = scroll_increment_fas - ((scrolled / scrolllength) * scroll_increment_fas) + 1;
			renderplaylistUI();					// redraw SBUI
			/* scroll slower at the end of the scroll - get slower in the third quarter of the scroll, even slower in the fourth quarter */
			Sleep(1);
		}
		break;
	default:
		break;
	}
	return errCode;
}

HRESULT play_song_on_playlist(long playlist, WORD y_coordinates) {
	HRESULT retval = S_OK;
	long selection;
	double true_y = (double)scroll_offset + (double)y_coordinates / (double)1.98;
	if (true_y >= 0) {
		if (selected_playlist == 0) {
			selection = (long)round((true_y / (double)(fontsize + spacing))) -2;
			if (selection < 0)
				selection = 0;
			selected_playlist = (short)selection;
			setAppState(APPSTATE_PLAYLIST);
		}
		else {
			selection = (long)round( true_y / (double)(fontsize + spacing) )-2;
			if (selection < 0)
				selection = 0;
			myITunes->AddRef();
			play_iTunes_song(myITunes, (long)playlist, selection);
//			wchar_t debugbuf[512];
//			swprintf(debugbuf, 512, L"\ntrue_y: %f scroll_offset: %d y_coordinates: %d selection = %d\n", true_y, scroll_offset, y_coordinates, selection);
//			OutputDebugStringW(debugbuf);
		}
	}
	return retval;
}

HRESULT preloadResources() {
	HRESULT retval = S_OK;
	RZSBSDK_QUERYCAPABILITIES pSBSDKCap;
	retval = RzSBQueryCapabilities(&pSBSDKCap);	// query pSBSDKCap for Razer Switchblade type and load themed resourced accordingly
	switch (hwtheme) {
		case SB_THEME_BLADE:
			hbutton_controls = (HBITMAP)LoadImage(GetModuleHandle(NULL), MAKEINTRESOURCE(IDB_BUTTON_CONTROLS_GREY), IMAGE_BITMAP, SWITCHBLADE_DYNAMIC_KEY_X_SIZE, SWITCHBLADE_DYNAMIC_KEY_Y_SIZE, LR_DEFAULTCOLOR);
			hbutton_exit = (HBITMAP)LoadImage(GetModuleHandle(NULL), MAKEINTRESOURCE(IDB_BUTTON_EXIT_GREY), IMAGE_BITMAP, SWITCHBLADE_DYNAMIC_KEY_X_SIZE, SWITCHBLADE_DYNAMIC_KEY_Y_SIZE, LR_DEFAULTCOLOR);
			hbutton_playlist = (HBITMAP)LoadImage(GetModuleHandle(NULL), MAKEINTRESOURCE(IDB_BUTTON_PLAYLIST_GREY), IMAGE_BITMAP, SWITCHBLADE_DYNAMIC_KEY_X_SIZE, SWITCHBLADE_DYNAMIC_KEY_Y_SIZE, LR_DEFAULTCOLOR);
			hcontrols_pause = (HBITMAP)LoadImage(GetModuleHandle(NULL), MAKEINTRESOURCE(IDB_CONTROLS_PAUSE_GREY), IMAGE_BITMAP, SWITCHBLADE_TOUCHPAD_X_SIZE, SWITCHBLADE_TOUCHPAD_Y_SIZE, LR_DEFAULTCOLOR);
			hcontrols_play = (HBITMAP)LoadImage(GetModuleHandle(NULL), MAKEINTRESOURCE(IDB_CONTROLS_PLAY_GREY), IMAGE_BITMAP, SWITCHBLADE_TOUCHPAD_X_SIZE, SWITCHBLADE_TOUCHPAD_Y_SIZE, LR_DEFAULTCOLOR);
		break;
		case SB_THEME_DSTALKER:
			hbutton_controls = (HBITMAP)LoadImage(GetModuleHandle(NULL), MAKEINTRESOURCE(IDB_BUTTON_CONTROLS_GREEN), IMAGE_BITMAP, SWITCHBLADE_DYNAMIC_KEY_X_SIZE, SWITCHBLADE_DYNAMIC_KEY_Y_SIZE, LR_DEFAULTCOLOR);
			hbutton_exit = (HBITMAP)LoadImage(GetModuleHandle(NULL), MAKEINTRESOURCE(IDB_BUTTON_EXIT_GREEN), IMAGE_BITMAP, SWITCHBLADE_DYNAMIC_KEY_X_SIZE, SWITCHBLADE_DYNAMIC_KEY_Y_SIZE, LR_DEFAULTCOLOR);
			hbutton_playlist = (HBITMAP)LoadImage(GetModuleHandle(NULL), MAKEINTRESOURCE(IDB_BUTTON_PLAYLIST_GREEN), IMAGE_BITMAP, SWITCHBLADE_DYNAMIC_KEY_X_SIZE, SWITCHBLADE_DYNAMIC_KEY_Y_SIZE, LR_DEFAULTCOLOR);
			hcontrols_pause = (HBITMAP)LoadImage(GetModuleHandle(NULL), MAKEINTRESOURCE(IDB_CONTROLS_PAUSE_GREEN), IMAGE_BITMAP, SWITCHBLADE_TOUCHPAD_X_SIZE, SWITCHBLADE_TOUCHPAD_Y_SIZE, LR_DEFAULTCOLOR);
			hcontrols_play = (HBITMAP)LoadImage(GetModuleHandle(NULL), MAKEINTRESOURCE(IDB_CONTROLS_PLAY_GREEN), IMAGE_BITMAP, SWITCHBLADE_TOUCHPAD_X_SIZE, SWITCHBLADE_TOUCHPAD_Y_SIZE, LR_DEFAULTCOLOR);
		break;
		case SB_THEME_SWTOR:
			hbutton_controls = (HBITMAP)LoadImage(GetModuleHandle(NULL), MAKEINTRESOURCE(IDB_BUTTON_CONTROLS_GOLD), IMAGE_BITMAP, SWITCHBLADE_DYNAMIC_KEY_X_SIZE, SWITCHBLADE_DYNAMIC_KEY_Y_SIZE, LR_DEFAULTCOLOR);
			hbutton_exit = (HBITMAP)LoadImage(GetModuleHandle(NULL), MAKEINTRESOURCE(IDB_BUTTON_EXIT_GOLD), IMAGE_BITMAP, SWITCHBLADE_DYNAMIC_KEY_X_SIZE, SWITCHBLADE_DYNAMIC_KEY_Y_SIZE, LR_DEFAULTCOLOR);
			hbutton_playlist = (HBITMAP)LoadImage(GetModuleHandle(NULL), MAKEINTRESOURCE(IDB_BUTTON_PLAYLIST_GOLD), IMAGE_BITMAP, SWITCHBLADE_DYNAMIC_KEY_X_SIZE, SWITCHBLADE_DYNAMIC_KEY_Y_SIZE, LR_DEFAULTCOLOR);
			hcontrols_pause = (HBITMAP)LoadImage(GetModuleHandle(NULL), MAKEINTRESOURCE(IDB_CONTROLS_PAUSE_GOLD), IMAGE_BITMAP, SWITCHBLADE_TOUCHPAD_X_SIZE, SWITCHBLADE_TOUCHPAD_Y_SIZE, LR_DEFAULTCOLOR);
			hcontrols_play = (HBITMAP)LoadImage(GetModuleHandle(NULL), MAKEINTRESOURCE(IDB_CONTROLS_PLAY_GOLD), IMAGE_BITMAP, SWITCHBLADE_TOUCHPAD_X_SIZE, SWITCHBLADE_TOUCHPAD_Y_SIZE, LR_DEFAULTCOLOR);
		break;
		default:
			hbutton_controls = (HBITMAP)LoadImage(GetModuleHandle(NULL), MAKEINTRESOURCE(IDB_BUTTON_CONTROLS_GREY), IMAGE_BITMAP, SWITCHBLADE_DYNAMIC_KEY_X_SIZE, SWITCHBLADE_DYNAMIC_KEY_Y_SIZE, LR_DEFAULTCOLOR);
			hbutton_exit = (HBITMAP)LoadImage(GetModuleHandle(NULL), MAKEINTRESOURCE(IDB_BUTTON_EXIT_GREY), IMAGE_BITMAP, SWITCHBLADE_DYNAMIC_KEY_X_SIZE, SWITCHBLADE_DYNAMIC_KEY_Y_SIZE, LR_DEFAULTCOLOR);
			hbutton_playlist = (HBITMAP)LoadImage(GetModuleHandle(NULL), MAKEINTRESOURCE(IDB_BUTTON_PLAYLIST_GREY), IMAGE_BITMAP, SWITCHBLADE_DYNAMIC_KEY_X_SIZE, SWITCHBLADE_DYNAMIC_KEY_Y_SIZE, LR_DEFAULTCOLOR);
			hcontrols_pause = (HBITMAP)LoadImage(GetModuleHandle(NULL), MAKEINTRESOURCE(IDB_CONTROLS_PAUSE_GREY), IMAGE_BITMAP, SWITCHBLADE_TOUCHPAD_X_SIZE, SWITCHBLADE_TOUCHPAD_Y_SIZE, LR_DEFAULTCOLOR);
			hcontrols_play = (HBITMAP)LoadImage(GetModuleHandle(NULL), MAKEINTRESOURCE(IDB_CONTROLS_PLAY_GREY), IMAGE_BITMAP, SWITCHBLADE_TOUCHPAD_X_SIZE, SWITCHBLADE_TOUCHPAD_Y_SIZE, LR_DEFAULTCOLOR);
		break;
	}
	AppIcon = (HICON)LoadImage(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_APPICON), IMAGE_ICON, GetSystemMetrics(SM_CXICON), GetSystemMetrics(SM_CYICON), LR_DEFAULTCOLOR);
	AppIconSM = (HICON)LoadImage(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_APPICON), IMAGE_ICON, GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), LR_DEFAULTCOLOR);
	return retval;
}

unsigned short __inline ARGB2RGB565(int x)
{
	unsigned short r = (unsigned short)((x & 0x00F80000) >> 8);
	unsigned short g = (unsigned short)((x & 0x0000FC00) >> 5);
	unsigned short b = (unsigned short)((x & 0x000000F8) >> 3);
	unsigned short rgb565 = 0;

	rgb565 = r | g | b;

	return rgb565;
}

HRESULT drawSBImage(RZSBSDK_DISPLAY sbui_display_handle, HBITMAP buttonimage, RZSBSDK_BUFFERPARAMS sbuidisplay)
{
	HRESULT retval = S_OK;
	unsigned short* rgb565buffer;
	DWORD* rgb888buffer;
	unsigned int in, out;
	unsigned short output_x, output_y;
	BITMAPINFO bitmap_cfg;
	size_t targetDisplaySize;
	HDC resbitmapDC = NULL;
	in = 0;
	out = 0;
	/* set target memory buffer size, target rows and lines depending on desired output display */
	if (sbui_display_handle == RZSBSDK_DISPLAY_WIDGET)
	{
		targetDisplaySize = SWITCHBLADE_TOUCHPAD_SIZE_IMAGEDATA;
		output_x = SWITCHBLADE_TOUCHPAD_X_SIZE;
		output_y = SWITCHBLADE_TOUCHPAD_Y_SIZE;
	}
	else										// assume we draw on a dynamic key if not main widged is specified
	{
		targetDisplaySize = SWITCHBLADE_DK_SIZE_IMAGEDATA;
		output_x = SWITCHBLADE_DYNAMIC_KEY_X_SIZE;
		output_y = SWITCHBLADE_DYNAMIC_KEY_Y_SIZE;
	}
	resbitmapDC = CreateCompatibleDC(NULL);
	memset(&bitmap_cfg, 0, sizeof(BITMAPINFO));
	/* allocate the converted rgb565 image to hold the pixels to be outputted to screen */
	rgb565buffer = (unsigned short*)malloc(targetDisplaySize);
	/* prepare the output structure to draw on the SBUI interface*/
	memset(&sbuidisplay, 0, sizeof(RZSBSDK_BUFFERPARAMS));
	sbuidisplay.PixelType = RGB565;
	sbuidisplay.DataSize = targetDisplaySize;
	sbuidisplay.pData = (BYTE*)rgb565buffer;	
	/* retrieve bitmap header, set for RGB conversion as bitmaps are 8 bit color palette */
	bitmap_cfg.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	retval = GetDIBits(resbitmapDC, buttonimage, 0, output_y, NULL, &bitmap_cfg, DIB_RGB_COLORS);
	bitmap_cfg.bmiHeader.biCompression = BI_RGB;// sonst steht da 3L drinnen und dann wird beim 2. Aufruf aufgrund BI_BITFILES drinnens tehen hat schreibt er bitmaske in COLORQUAD feld dahinter - ist aber nur fuer 1 bitmaske platz [1] in definition - muesste dann mehr sein, brauchen wir ja nicht
	/* init RGB88 Buffer to hold image pixels from HBITMAP structure*/
	rgb888buffer = (DWORD*)_aligned_malloc(bitmap_cfg.bmiHeader.biSizeImage, 4);
	/* remove striding image is padded with black pixels only for dynamic key images (uneven rows in image) */
	if (sbui_display_handle == RZSBSDK_DISPLAY_WIDGET) {
		retval = GetDIBits(resbitmapDC, buttonimage, 0, output_y, rgb888buffer, &bitmap_cfg, DIB_RGB_COLORS);
		for (short row = 0; row < output_y; row++)
		{
			for (short col = 0; col < output_x; col++)
			{	
				rgb565buffer[out] = ARGB2RGB565(rgb888buffer[in]);
				out = out + 1;
				in = in + 1;
			}
		}
	}
	else {										// assume dynamic key rendering, remove padding to prevent image striding
		/* get the image */
		retval = GetDIBits(resbitmapDC, buttonimage, 0, output_y, rgb888buffer, &bitmap_cfg, DIB_RGB_COLORS);
		for (short row = 0; row < output_y; row++)
		{
			for (short col = 0; col < output_x; col++)
			{
				if (col == 0 && row != 0)		// remove padding to prevent image striding
					in--;
				rgb565buffer[out] = ARGB2RGB565(rgb888buffer[in]);
				out = out + 1;
				in = in + 1;
			}
		}
	}

	/* Draw the Image */
	RzSBRenderBuffer(sbui_display_handle, &sbuidisplay);
	/* clean up */
	_aligned_free(rgb888buffer);
	free(rgb565buffer);
	return retval;
}


HRESULT initSwitchbladeControls() {				// paint common user interface
	HRESULT retval = S_OK;

	preloadResources();							// preload image handles depending upon selected sbui theme
	/* set up the buttons for the control and playlist interface as well as an exit button */
	/* set touchscreen coordinates of last top to zero */
	old_x = 0;
	old_y = 0;
	/* paint exit button */
	drawSBImage(RZSBSDK_DISPLAY_DK_5, hbutton_exit, sbuidisplay);
	/* paint playlist button */
	drawSBImage(RZSBSDK_DISPLAY_DK_7, hbutton_playlist, sbuidisplay);
	/* paint music controls button */
	drawSBImage(RZSBSDK_DISPLAY_DK_6, hbutton_controls, sbuidisplay);
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
		refreshiTunesPlayList();			// get songlists from iTunes
		showiTunesControlInterface();		// paint interface for iTunes controls
		break;
	case APPSTATE_CONTROLS_PLAY:
		applicationstate = APPSTATE_CONTROLS_PLAY;
		retval = drawSBImage(RZSBSDK_DISPLAY_WIDGET, hcontrols_play, sbuidisplay);
		retval = RzSBGestureSetCallback(reinterpret_cast<TouchpadGestureCallbackFunctionType>(TouchPadHandler_Controls));
		break;
	case APPSTATE_CONTROLS_PAUSE:
		applicationstate = APPSTATE_CONTROLS_PAUSE;
		retval = drawSBImage(RZSBSDK_DISPLAY_WIDGET, hcontrols_pause, sbuidisplay);
		retval = RzSBGestureSetCallback(reinterpret_cast<TouchpadGestureCallbackFunctionType>(TouchPadHandler_Controls));
		break;
	case APPSTATE_PLAYLIST_START:
		applicationstate = APPSTATE_PLAYLIST_START;
		continue_scrolling = false;
		selected_playlist = 0;
		scroll_offset = 0;
		showiTunesPlaylistInterface();
		break;
	case APPSTATE_PLAYLIST:
		applicationstate = APPSTATE_PLAYLIST;
		continue_scrolling = false;
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
	continue_scrolling = false;
	drawPlaylistOffscreen(selected_playlist);
	renderplaylistUI();
	retval = RzSBGestureSetCallback(reinterpret_cast<TouchpadGestureCallbackFunctionType>(TouchPadHandler_Playlist));
	return retval;
}

/* Stores the last point of contact on the UI for later scrolling calculations */

HRESULT storeLastUITapCoord(WORD x, WORD y) {
	HRESULT retval;
	retval = S_OK;
	old_x = x;
	old_y = y;
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
	if (ValidGesture(gesturetype)) {
		switch (gesturetype) {
		case RZSBSDK_GESTURE_FLICK:
			continue_scrolling = false;			// stop scrolling upon pad press
			scrollimmunitytimerthread = CreateThread(NULL, 0, flickTimer, NULL, 0, NULL);
			retval = padFlick(z);				// call scrolling function
			break;
		case RZSBSDK_GESTURE_PRESS:
			continue_scrolling = false;			// stop scrolling upon pad press
			storeLastUITapCoord(x, y);
			break;
/* As long as we do not have a good solutiuon to distinguis between the user moving his finger on the touchscreen and flicking the finger,
this stays commented out: 
		case RZSBSDK_GESTURE_MOVE:
			continue_scrolling = false;			// stop scrolling upon pad press
			if (!flick_in_progress && SingleGesture(gesturetype))
				retval = padMove(y);			// do not interpret move gesture while flicking
			storeLastUITapCoord(x, y);
			break;
*/
		case RZSBSDK_GESTURE_TAP:				// play song upon tapping on it
			continue_scrolling = false;			// stop scrolling upon pad press
			if (!flick_in_progress)				// stop scrolling on pad press, but do not select song for playback
				retval = play_song_on_playlist(selected_playlist, y);
			break;
		default:
			break;
		}
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
	/* check for theme argument switch when called from Razer Switchblade System */
	/* default theme is the grey blade theme */
	hwtheme = SB_THEME_SWTOR;
	if (wcsstr(lpCmdLine, L"-skin:Blade") != NULL)
		hwtheme = SB_THEME_SWTOR;
	if (wcsstr(lpCmdLine, L"-skin:DeathStalker") != NULL)
		hwtheme = SB_THEME_DSTALKER;
	hRes = S_OK;
	o_h_pixbuf = NULL;
	moving = false;
	hRes = connectiTunes();
	Sleep(5);
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