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

#ifndef SONGQUEUEITEM_H
#define SONGQUEUEITEM_H

#include <QFile>
#include <QString>

#include "collectionprovider.h"

class SongQueueItemRetriever;

//
// Represents a single song in the song queue
// This class must be copyable, as used everywhere through player
//
class SongQueueItem
{
    public:
        SongQueueItem();
        ~SongQueueItem();

        enum State
        {
            STATE_NOT_READY,    // Song is not present in the local storage and cannot be played
            STATE_GETTING_READY,// Song is getting ready - converting or downloading
            STATE_READY,        // Song is ready for playing
            STATE_PLAYING,      // Song is being played
        };

        // Returns a text representation of state
        QString     stateText() const;

        int         id;         // unique queue id as order can change anytime so index cannot be used as ID
        int         songid;     // nonzero if the song is in the database, so its status could be updated
        QString     file;       // may be original or cached. If empty when added, the song is to be retrieved
        QString     title;      // Song title
        QString     artist;     // Song artist
        QString     singer;     // Can be empty
        State       state;      // see above
        int         readiness;  // If not yet ready, the percentage of how much of the item is converted/downloaded/unpacked (0-100)
};


// Let Qt know about it
Q_DECLARE_METATYPE(SongQueueItem)


#endif // SONGQUEUEITEM_H
