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

#ifndef SONGQUEUE_H
#define SONGQUEUE_H

#include <QObject>
#include <QList>
#include <QMap>

#include "songqueueitem.h"

// Internal queued song storage element
class SongQueueEntry;


// Represents the enqueued songs, maintains people's position
// and ensures honest rotation
class SongQueue : public QObject
{
    Q_OBJECT

    public:
        SongQueue( QObject * parent );
        ~SongQueue();

        // Initializes the queue, loading it (and emitting the queueChanged) if necessary
        void    init();

        // Returns true if the queue has no more songs and the current song is not valid
        bool    isEmpty() const;

        // Returns the current song from queue. Crashes if you call it and the queue is empty; use isEmpty first!
        SongQueueItem  current() const;

        // Moves queue pointer to a previous song. Returns false if this is the first song (pointer doesn't move then)
        bool    prev();

        // Moves queue forward. Returns false if there is no next song (pointer doesn't move)
        bool    next();

        // Exports the queue and its current song index
        QList< SongQueueItem >   exportQueue() const;
        unsigned int    currentItem() const;

    public slots:
        void    clear();

        // Those slots are called from the collection provider. The id is the queue ID
        void    providerFinished( int id, const QString& error );
        void    providerProgress( int id, int progress );

        // Song status changed (playing/stopped)
        void    statusChanged( int id, bool playstarted );

        // Adds a song into queue, automatically rearranging it. Returns true if the song is added, false otherwise
        void    addSong( const QString& singer, int id );

        // Adds a song into queue, automatically rearranging it. Returns true if the song is added, false otherwise
        void    addSong( const QString& singer, const QString& file );

        // Adds a song into queue, the struct should be filled up already
        void    addSong( SongQueueItem song );

        // Add a song with possible retriever
        void    addSong( SongQueueItem song , SongQueueItemRetriever *retriever );

        // Inserts a song into queue at specific position, without rearranging
        void    insertSong( unsigned int position, const QString& singer, int songid );

        // Replaces a song at specific position
        void    replaceSong( unsigned int position, int songid );

        // Replaces a song at specific position
        void    replaceSong( unsigned int position, const SongQueueItem& song );

        // Removes a song at specific position
        void    removeSong(unsigned int position );

        // Removes a song using specific ID. Returns true if the removed song is current
        bool    removeSongById(int id );

        // Moves a song from position from to position to
        void    moveSong( unsigned int from, unsigned int to );

    private:
        int     addSinger( const QString& singer );
        void    queueUpdated();
        void    save();
        void    load();
        bool    startRetrieve( int idx );

        // Queue of songs. Works as following:
        //
        // - Songs are added into m_queue as they put in by people
        // - Songs are never removed; m_current points to the currently played song
        // - Current singer is set by m_currentSinger
        // - Once the song is done, the next one is chosen by the next singer; if none,
        //   then next singer, and so on (if there is song in the list, there is singer in m_singers)
        // - The current item is swapped with the singer (i.e. list is rearranged)
        // - List may go back into previous entries, then it is not rearranged. m_lastArranged controls this
        QList<SongQueueEntry *>     m_queue;

        // The current song being played; if == m_queue.size(), nothing is played. Index in m_queue
        unsigned int                m_currentSong;

        // keeps track of all singers by name
        QList<QString>              m_singers;

        // Queue identifier increment
        unsigned int                m_nextQueueId;

        // Number of songs currently being retrieved concurrently
        unsigned int                m_beingRetrieved;
};

extern SongQueue * pSongQueue;

#endif // SONGQUEUE_H
