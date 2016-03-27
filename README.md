# spivak
Spivak is a cross-platform Karaoke player written by George Yunaev and licensed under GNU GPL version 3. It supports a wide range of Karaoke formats, with the goal of playing all more or less widespread Karaoke formats on all popular platforms. 

Currently Spivak is in alpha stage, but already has an good enough set of features, and is stable enough to survive a party without a single crash or hang.

## Features

- Supports for the following Karaoke lyric formats: MIDI/KAR, KaraFun, CDG, LRCv1, LRCv2, Encore! Lyric, KOK, Ultrastar (TXT);
- Autodetects the lyric text encoding for text formats, no manual encoding needed;
- Supports media files in all formats supported by your GStreamer installation (and if any format is not supported, you can install GStreamer plugin to add support);
- Supports ZIP archives containing a music file + lyric file, for all supported format (i.e. MP3+LRC is also supported);
- Supports video karaoke;
- Has a built-in song database with scan and search capabilities;
- Has Web interface allowing users to search for songs, browse the song database and even add the songs into queue themselves;
- Smart singer queue management, 
- Has built-in MIDI software syntheser, so plays MIDI files even on OS without MIDI support;
- Supports animated images and video background for karaoke songs;
- Supports LIRC for remote controlling the player;
- Can play music files when no Karaoke is played, will pause it when Karaoke is chosen, and will resume automatically; 
- Has comprehensive karaoke queue management interface, as well as background music management interface;

The player has been tested primarily on Linux, but has been briefly tested on Windows and Mac OS X.

## Building

Currently the only way to test the player is to build it from source, which should be easy on Linux, but not so easy on other OSes. You would need the following libraries:

- Qt 5.5 (might work on prior versions too, but will NOT work on Qt4);
- GStreamer 1.0 (will NOT work on 0.10) and related libraries, notably Glib;
- uchardet2 (for automatic charset decoding);
- libzip (for handling ZIP and old KFN files);
- libsonivox (to render MIDI in software; you can get it from here: https://github.com/gyunaev/libsonivox);
- libcld2 (to build Karaoke language detector plugin; optional);
- sqlite3 (for karaoke database);

Please do not build with google breakpad, as this would enable automatic crash reporting which is useless for me because I do not have your symbols file.

To build, please comment out the line "CONFIG += GOOGLEBREAKPAD" in src/src.pro, and then build as usual:

    qmake (or qmake-qt5)
    make

Copy the player executable from src/spivak somewhere, and copy all the plugins into the "plugins" subdirectory where spivak executable is located.

## Contacts

At this moment please use gyunaev@ulduzsoft.com to send the bugs and feature requests. Please only do so if you're willing to actively participate in bugfixing, such as testing proposed patches, and are able to compile the player yourself from the source code. If you cannot, please wait until the first beta is available, when the official builds would be available.
