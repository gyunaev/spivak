# Spivak - free cross-platform Karaoke Player [![Build Status](https://travis-ci.org/gyunaev/spivak.svg?branch=master)](https://travis-ci.org/gyunaev/spivak)

![Screenshot](https://github.com/gyunaev/spivak/raw/master/packaging/screenshot2.png "Screenshot")

Spivak is a free, cross-platform (Linux/Windows/OS X) Karaoke player based on GStreamer and Qt5. It supports a wide range of Karaoke formats, with the goal of playing all more or less widespread Karaoke formats on all popular platforms. It also has strong support for foreign languages, so playing Karaoke in Japanese, Russian or Hindu is fully supported.

Spivak has been designed to be a standalone Karaoke player, which could be used at your parties without a dedicated DJ. It is primarily controlled through its Web interface, so your guests can take care of themselves, searching and adding songs into the queue. The player will ensure fair rotation.

Spivak has been used on many parties for years and has been quite stable.

Spivak is licensed under GNU GPL version 3, and is written by George Yunaev.

## Features

- Supports the most popular Karaoke lyric formats: MIDI/KAR, KaraFun, CDG, LRC (v1 and v2), Encore! Lyric, KOK, Ultrastar (TXT);
- Automatically detects the lyric text encoding, no manual encoding needed;
- Supports music/video media in many different formats, as long as they are supported by your GStreamer installation (and if any format is not supported, you can install GStreamer plugin to add support);
- Supports ZIP archives containing a music file + lyric file, for all supported format (i.e. MP3+LRC is also supported);
- Supports video karaoke, such as in AVI, MKV or Flash formats;
- Supports Karaoke collections, and is able to scan the Karaoke songs into the built-in song database with search capabilities;
- Has built-in Web interface allowing users to search for songs, browse the song database and even queue the songs themselves;
- Has a smart singer queue management, ensuring fair rotation - when the same guest queues multiple songs, they are spread to ensure everyone has opportunity to sing;
- Has a built-in MIDI software syntheser with embedded wavetable, so plays MIDI files even on OS without MIDI support and without any external files such as soundfonts;
- Supports animated images and video background for karaoke songs, including CD+G songs;
- Supports LIRC for remotely controlling the player;
- Supports background music - can play regular music files when no Karaoke is played, will automatically pause it when Karaoke is chosen, and will resume the music automatically once the queue is empty;
- Has comprehensive karaoke queue management interface, as well as background music management interface;

The player has been tested primarily on Linux, but has been briefly tested on Windows and Mac OS X.

## Compiling

Currently the only way to test the player is to build it from source, which should be easy on Linux, but may be cumbersome on other OSes. You would need the following libraries:

- Qt6 (it might work with later versions of Qt5 too);
- GStreamer 1.0 (will NOT work on 0.10) and related libraries, notably Glib;
- uchardet2 (for automatic charset decoding);
- libzip (for handling ZIP and old KFN files);
- libcld2 (to build Karaoke language detector plugin; optional);
- sqlite3 (for karaoke database);

Spivak builds as usual:

    qmake6
    make
    
Apologies for the "make -jX" build being still broken.

The player executable `spivak` is located in `src/` but it requires `libmediaplayer_spivak` to be loadable via dlopen. This means that either all files from `libmediaplayer/libmediaplayer_spivak.so*` must be copied somewhere where ld.so can pick them up (such as /usr/lib64) or `LD_LIBRARY_PATH` must be set to that directory.

You also need to copy all the plugins into either /usr/lib or into Spivak directory.

If you're building it, you can run spivak as following:

'''
cd src
export LD_LIBRARY_PATH=`pwd`/../libmediaplayer
./spivak
'''

## Limitations

- For complex karaoke formats (Ultrastar, KFM) only bare music+lyrics playing functionality is supported. Other features of those formats (such as background video or pitch analysis) are not supported, and there are no plans to support them. If those features are important to you, please use the dedicated players for those formats.
- Newer KFN files use a different encryption and are not supported (yet?).

## Screenshots

![Screenshot](https://github.com/gyunaev/spivak/raw/master/packaging/screenshot1.png "Screenshot")
![Screenshot](https://github.com/gyunaev/spivak/raw/master/packaging/screenshot2.png "Screenshot")
![Screenshot](https://github.com/gyunaev/spivak/raw/master/packaging/screenshot3.png "Screenshot")


## Reporting bugs and requesting features

Please use Github issue tracker for bugs feature requests.

