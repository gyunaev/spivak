/**************************************************************************
 *  Spivak Karaoke PLayer - a free, cross-platform desktop karaoke player *
 *  Copyright (C) 2015-2016 George Yunaev, support@ulduzsoft.com          *
 *                                                                        *
 *  This program is free software: you can redistribute it and/or modify  *
 *  it under the terms of the GNU General Public License as published by  *
 *  the Free Software Foundation, either version 3 of the License, or     *
 *  (at your option) any later version.                                   *
 *																	      *
 *  This program is distributed in the hope that it will be useful,       *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *  GNU General Public License for more details.                          *
 *                                                                        *
 *  You should have received a copy of the GNU General Public License     *
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>. *
 **************************************************************************/

#ifndef SETTINGS_H
#define SETTINGS_H

#include <QMap>
#include <QFont>
#include <QColor>
#include <QJsonValue>

#include "collectionentry.h"

class Settings
{
    public:
        Settings();

        enum BackgroundType
        {
            BACKGROUND_TYPE_NONE = 2,
            BACKGROUND_TYPE_COLOR = 4,
            BACKGROUND_TYPE_IMAGE = 6,
            BACKGROUND_TYPE_VIDEO = 8
        };

        QString cacheDir;

        // Player parameters
        QColor      playerLyricsTextBeforeColor;
        QColor      playerLyricsTextAfterColor;
        QColor      playerLyricsTextSpotColor;
        QColor      playerLyricsTextBackgroundColor;
        int         playerLyricBackgroundTintPercentage;
        QFont       playerLyricsFont;
        int         playerLyricsFontMaxSize;
        int         playerLyricsFontFitLines;
        bool        playerLyricsTextEachCharacter;
        bool        playerUseBuiltinMidiSynth;
        bool        playerCDGbackgroundTransparent;
        bool        playerIgnoreBackgroundFromFormats;

        int         playerRenderFPS;
        int         playerMusicLyricDelay;      // delay in milliseconds between lyrics and music for this hardware
        int         playerVolumeStep;           // how much change the volume when +/- is pressed

        int         dialogAutoCloseTimer;       // How fast the dialog will automatically close

        //
        // Queue parameters
        //

        // if true, new singers are added right after the current singer; otherwise at the end.
        bool        queueAddNewSingersNext;

        // if defined, we save queue into file
        QString     queueFilename;
        bool        queueSaveOnExit;

        // Notification
        QColor      notificationTopColor;
        QColor      notificationCenterColor;

        // Collection list mapped by collection ID
        QMap<int,CollectionEntry>  collections;

        // Maximum number of songs which could be simultaneously
        // got ready (i.e. downloaded or converted)
        unsigned int    queueMaxConcurrentPrepare;

        // Player background
        BackgroundType  playerBackgroundType;
        QColor          playerBackgroundColor;
        QStringList     playerBackgroundObjects;    // images or videos
        unsigned int    playerBackgroundTransitionDelay; // images only, in seconds. 0 - no transitions/slideshow

        // Music background
        QStringList     musicCollections;
        bool            musicCollectionSortedOrder;
        int             musicCollectionCrossfadeTime;

        // Songs database
        QString         songdbFilename;

        // LIRC path
        bool            lircEnabled;
        QString         lircDevicePath;
        QString         lircMappingFile;

        // HTTP server
        bool            httpEnabled;
        bool            httpEnableAddQueue;
        unsigned int    httpListenPort;
        QString         httpDocumentRoot;
        QString         httpAccessCode;
        QString         httpForceUseHost;

        // If true, we should start in a full screen mode (but this doesn't mean NOW is a fullscreen mode)
        bool            startInFullscreen;

        // If true, first-time wizard has been shown already
        bool            firstTimeWizardShown;

        // Replaces the database path according to rules
        QString         replacePath( const QString& origpath );

    public:
        // Load and save settings
        bool            load();
        bool            save();

        QJsonObject     toJson();
        void            fromJson( const QJsonObject& data );

    private:
        QJsonValue      fromStringList( const QStringList& list );
        QStringList     toStringList( const QJsonValue& value, const QString& defaultval = "" );

        QString         m_appDataPath;
        QString         m_settingsFile;

        // If the song path replacement is set (songPathReplacementFrom is not empty), this would replace '^from' to '^to'
        QString         songPathReplacementFrom;
        QString         songPathReplacementTo;
};

extern Settings * pSettings;


#endif // SETTINGS_H
