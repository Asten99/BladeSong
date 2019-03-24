# BladeSong
Razer SwitchBlade remote control application for Apple iTunes running on Microsoft Windows distributed under the BSD open license.

To build this solution yourself, you need the Apple iTunes COM interface (from Apple) as well as the Switchblade 2 SDK (from Razer).

To download a precompiled version, see the zip file in this directory.
This is BladeSong, an iTunes playlist visualizer and control program for
you Razer Switchblade devices.

First of all, many thanks to to my dear friend who helped me with C coding
that I failed to grasp and a very gentle being at Razer who supplied
me with the SDK even tough it was no longer in support.

Software Requirements:

-) iTunes for Windows
-) Synapse 2 for Windows

Hardware Requirements:

-) Any PC running Windows Vista+
-) Switchblade device (correctly installed with full Synapse drivers)

Developer requirements (not needed to start the software):

This software was built using Visual Studio 2017 Community Edition as
well as the following two additional components:

-) Apple iTunes COM interface v. 9.1.0.80
-) Razer Switchblade UI SDK v. 2.0


User documentation:

Just copy the BladeSong.exe anywhere you like and start it.
iTunes will start by itself if it is not already running.
You can use the buttons on top to switch between displaying playlists
and controlling iTunes playback. Please note depending on the number of
songs in your iTunes library, it may take some time to load the songs for
SongBlade to display. Songblade will notify you if it has not finished
loading a particular playlist content. You can select a playlist by
tapping on it. To get back, just press the playlist button on the
Switchblade dynamic keys again. To play back a song, tap on it after
selecting the playlist it is in. To scroll the list of playlists or
within a playlist, just flick through the list.
To control iTunes playback, press the playback control button on
the dynamic key range.
You can use iTunes normally during the time BaldeSong runs.
To exit the Application, press the exit button. This relinquishes
the connection it has to iTunes. Please note you get a warning from
iTunes if you try to quit iTunes before exiting BladeSong first.

Developer notes:

In order for flicking to work when compiling the code for release,
turn of "complete optimization of program code" in the common project
properties page. If you have any suggestions, comments or want to expand
upon this project, feel free to. Especially UI artwork help would be
greatly appreciated.
