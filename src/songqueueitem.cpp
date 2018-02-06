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

#include "songqueueitem.h"

SongQueueItem::SongQueueItem()
{
    id = 0;
    songid = 0;
    readiness = 0;
}

SongQueueItem::~SongQueueItem()
{
}

QString SongQueueItem::stateText() const
{
    switch ( state )
    {
        case STATE_NOT_READY:
            return QObject::tr("waiting to be downloaded");

        case STATE_GETTING_READY:
            return QObject::tr("downloading %1%").arg( readiness );

        case STATE_READY:
            return QObject::tr("ready");

        case STATE_PLAYING:
            return QObject::tr("playing");

        default:
            return "unknown";
    }
}
