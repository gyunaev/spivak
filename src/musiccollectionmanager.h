/**************************************************************************
 *  Spivak Karaoke PLayer - a free, cross-platform desktop karaoke player *
 *  Copyright (C) 2015-2018 George Yunaev, support@ulduzsoft.com          *
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

#ifndef MUSICCOLLECTIONMANAGER_H
#define MUSICCOLLECTIONMANAGER_H

#include <QObject>

#include "songqueue.h"
#include "mediaplayer.h"


//
// This class handles the music collection - scans, queue and playing.
// It is always created even if the collection is empty.
class MusicCollectionManager : public QObject
{
    Q_OBJECT

    public:
        explicit MusicCollectionManager( QObject *parent = 0 );
        ~MusicCollectionManager();

        // Collection access/modification
        void    exportCollection( QStringList & musicFiles );

        // Inserts music at specific position, -1 means at the end
        void    insertMusic( int pos, const QString& file );
        void    removeMusic( unsigned int position );
        void    moveMusicItem( unsigned int from, unsigned int to );

        // Supported file extensions
        const QStringList& supportedExtensions() const;

        // If player is playing, returns the position and duration
        void    currentProgress( qint64& position, qint64& duration );

    public slots:
        void    initCollection();

        // Management slots (changing the manager state based on actions)
        void    start();
        void    stop();
        void    pause();
        void    nextMusic();
        void    prevMusic();
        void    volumeDown();
        void    volumeUp();
        void    volumeSet( int percentage );

        // Notification slots (so we can turn off the player when song is started, and then turn it back)
        void    karaokeStarted(SongQueueItem song );
        void    karaokeStopped();
        void    playerCurrentSongFinished();

        // Requestor
        bool    isPlaying() const;

    private slots:
        // Handles fade-out and fade-in
        void    fadeVolume();

        // Player loaded the song
        void    songLoaded();

    private:
        // Volume setter
        void    setVolume( int percentage, bool notify = true );

        // Player is only created when the collection is used
        MediaPlayer *   m_player;

        // Original music directories from settings
        QStringList     m_musicDirectories;

        // Songs (files and files from directories) sorted according to settings
        QStringList     m_musicFiles;

        // Currently played file
        unsigned int    m_currentFile;

        // List of supported music file extensions
        QStringList     m_supportedExtensions;

        // For fading out/in: volume decrease/increase step; if zero we're not crossfading
        int             m_crossfadeVolumeStep;
        int             m_crossfadeCurrentVolume;
};


extern MusicCollectionManager * pMusicCollectionMgr;


#endif // MUSICCOLLECTIONMANAGER_H
