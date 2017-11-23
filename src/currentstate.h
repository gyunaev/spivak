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

#ifndef CURRENTSTATE_H
#define CURRENTSTATE_H

#include <QSize>
#include <QObject>
#include <QAtomicInt>
#include <QStringList>

#include "songqueue.h"
#include "mediaplayer.h"

//
// Represents current state of a player.
//
// This is a storage of relatively long-living state (which song is playing, what is current player position, is Web server running and on which address, etc)
// which are supplied by relevant modules (queue, player), and are available to all modules.
//
//
class CurrentState : public QObject
{
    Q_OBJECT

    public:
        enum
        {
            // Many checks assume that if playback is not PLAYERSTATE_STOPPED, we're playing.
            // Hence please do not add here something like PLAYERSTATE_QUEUE_EMPTY
            PLAYERSTATE_STOPPED,
            PLAYERSTATE_PLAYING,
            PLAYERSTATE_PAUSED
        };

        explicit CurrentState(QObject *parent = 0);

        // mseconds per frame
        unsigned int            msecPerFrame;

        // Current database info (0 if not initialized)
        // May be accessed from another thread - move to concurrent storage when fixing this!
        unsigned int            m_databaseSongs;
        unsigned int            m_databaseArtists;
        QString                 m_databaseUpdatedDateTime;

        // Current player state
        QAtomicInt              playerState;

        // Current song parameters (not valid if playerState is STOPPED) - updated from another thread!
        MediaPlayer::Capabilities playerCapabilities;
        QAtomicInt              playerPosition;
        QAtomicInt              playerDuration;
        int                     playerVolume;
        int                     playerPitch;
        int                     playerTempo;
        bool                    playerVoiceRemovalEnabled;
        int                     playerLyricsDelay;

        // For video only, stores last position
        qint64                  playerBackgroundLastVideoPosition;

        // For music player
        int                     musicVolume;

        // For main window
        QSize                   windowSizeMain;

        // For Karaoke queue window
        QSize                   windowSizeKarQueue;
        QList<int>              splitterSizeKarQueue;

        // For Music queue window
        QSize                   windowSizeMusicQueue;


    public slots:
        void        loadBackgroundObjects();
        void        saveTempData();
        QString     currentBackgroundObject();
        void        nextBackgroundObject();
        void        applySettingsBackground();

    private:
        // Background objects (files and files from directories)
        QStringList             m_playerBackgroundObjects;

        // Last shown image, or last played video (from playerBackgroundObjects)
        unsigned int            m_playerBackgroundLastObject;
};

extern CurrentState * pCurrentState;

#endif // CURRENTSTATE_H
